#include <stm32wbxx_ll_dma.h>
#include <furi_hal.h>

#include "gpio_helper.h"
#include "../main.h"

// We store the HIGH/LOW durations (2 values) for each color bit (24 bits per LED)
#define LED_DRIVER_BUFFER_SIZE (MAX_LED_COUNT * 2 * 24)
// We use a setinel value to figure out when the timer is complete.
#define LED_DRIVER_TIMER_SETINEL 0xFFFFU

// 64 transitions per us @ 64MHz.  Our timing is in NANO_SECONDS
#define LED_DRIVER_TIMER_NANOSECOND (1000U / (SystemCoreClock / 1000000U))
// Timings for WS2812B
#define LED_DRIVER_T0H 400U
#define LED_DRIVER_T1H 800U
#define LED_DRIVER_T0L 850U
#define LED_DRIVER_T1L 450U
#define LED_DRIVER_TRESETL 55 * 1000U

// Wait for 35ms for the DMA to complete. NOTE: 1000 leds*(850ns+450ns)*24 = 32ms
#define LED_DRIVER_SETINEL_WAIT_MS 35

void setGpioPin(const GpioPin* gpioPin, bool state) {
    FURI_LOG_I(TAG, "Updating pin state to %s", state ? "On" : "Off");
    if(state) {
        furi_hal_gpio_init_simple(gpioPin, GpioModeOutputPushPull);
    } else {
        furi_hal_gpio_init(gpioPin, GpioModeOutputOpenDrain, GpioPullNo, GpioSpeedLow);
    }
    furi_hal_gpio_write(gpioPin, state);
}

static void setupDMAGPIOUpdate(
    LL_DMA_InitTypeDef* dma_gpio_update,
    const GpioPin* gpioPin,
    const uint32_t gpioBuf[2]) {
    // Memory to Peripheral
    dma_gpio_update->Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    // Peripheral (GPIO - We populate GPIO port's BSRR register)
    dma_gpio_update->PeriphOrM2MSrcAddress = (uint32_t)&gpioPin->port->BSRR;
    dma_gpio_update->PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    dma_gpio_update->PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_WORD;
    // Memory (State to set GPIO)
    dma_gpio_update->MemoryOrM2MDstAddress = (uint32_t)gpioBuf;
    dma_gpio_update->MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    dma_gpio_update->MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_WORD;
    // Data
    dma_gpio_update->Mode = LL_DMA_MODE_CIRCULAR;
    dma_gpio_update->NbData = 2; // We cycle between two (HIGH/LOW)values
    // When to perform data exchange
    dma_gpio_update->PeriphRequest = LL_DMAMUX_REQ_TIM2_UP;
    dma_gpio_update->Priority = LL_DMA_PRIORITY_VERYHIGH;
}

static void
    setupDMATransitionTimer(LL_DMA_InitTypeDef* dma_transition_timer, uint16_t* timerBuffer) {
    // Timer that triggers based on user data.
    dma_transition_timer->Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    // Peripheral (Timer - We populate TIM2's ARR register)
    dma_transition_timer->PeriphOrM2MSrcAddress = (uint32_t)&TIM2->ARR;
    dma_transition_timer->PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    dma_transition_timer->PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_WORD;
    // Memory (Timings)
    dma_transition_timer->MemoryOrM2MDstAddress = (uint32_t)timerBuffer;
    dma_transition_timer->MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    dma_transition_timer->MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_HALFWORD;
    // Data
    dma_transition_timer->Mode = LL_DMA_MODE_NORMAL;
    dma_transition_timer->NbData = LED_DRIVER_BUFFER_SIZE;
    // When to perform data exchange
    dma_transition_timer->PeriphRequest = LL_DMAMUX_REQ_TIM2_UP;
    dma_transition_timer->Priority = LL_DMA_PRIORITY_HIGH;
}

static void led_driver_start_dma(
    LL_DMA_InitTypeDef* dma_gpio_update,
    LL_DMA_InitTypeDef* dma_transition_timer) {
    LL_DMA_Init(DMA1, LL_DMA_CHANNEL_1, dma_gpio_update);
    LL_DMA_Init(DMA1, LL_DMA_CHANNEL_2, dma_transition_timer);

    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
}

static void led_driver_start_timer() {
    furi_hal_bus_enable(FuriHalBusTIM2);

    LL_TIM_SetCounterMode(TIM2, LL_TIM_COUNTERMODE_UP);
    LL_TIM_SetClockDivision(TIM2, LL_TIM_CLOCKDIVISION_DIV1);
    LL_TIM_SetPrescaler(TIM2, 0);
    // Updated by led_driver->dma_led_transition_timer.PeriphOrM2MSrcAddress
    LL_TIM_SetAutoReload(TIM2, LED_DRIVER_TIMER_SETINEL);
    LL_TIM_SetCounter(TIM2, 0);

    LL_TIM_EnableCounter(TIM2);
    LL_TIM_EnableUpdateEvent(TIM2);
    LL_TIM_EnableDMAReq_UPDATE(TIM2);
    LL_TIM_GenerateEvent_UPDATE(TIM2);
}

static void led_driver_stop_timer() {
    LL_TIM_DisableCounter(TIM2);
    LL_TIM_DisableUpdateEvent(TIM2);
    LL_TIM_DisableDMAReq_UPDATE(TIM2);
    furi_hal_bus_disable(FuriHalBusTIM2);
}

static void led_driver_stop_dma() {
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
    LL_DMA_ClearFlag_TC1(DMA1);
    LL_DMA_ClearFlag_TC2(DMA1);
}

static void led_driver_spin_lock(uint32_t* write_pos) {
    const uint32_t prev_timer = DWT->CYCCNT;
    const uint32_t wait_time = LED_DRIVER_SETINEL_WAIT_MS * SystemCoreClock / 1000;
    uint32_t read_pos = 0;

    do {
        /* Make sure it's started (allow 100 ticks), but then check for sentinel value. */
        if(TIM2->ARR == LED_DRIVER_TIMER_SETINEL && DWT->CYCCNT - prev_timer > 100) {
            break;
        }

        // 0xFF is fairly quick, make sure we didn't miss it.
        if((DWT->CYCCNT - prev_timer > wait_time)) {
            FURI_LOG_D("Demo", "0xFF not found (ARR 0x%08lx, read %lu)", TIM2->ARR, read_pos);
            read_pos = *write_pos - 1;
            break;
        }
    } while(true);
}

static void
    led_driver_add_period_length(uint16_t* timer_buffer, uint32_t* write_pos, uint32_t length) {
    timer_buffer[(*write_pos)++] = length;
    timer_buffer[*write_pos] = LED_DRIVER_TIMER_SETINEL;
}

static void
    led_driver_add_period(uint16_t* timer_buffer, uint32_t* write_pos, uint16_t duration_ns) {
    uint32_t reload_value = duration_ns / LED_DRIVER_TIMER_NANOSECOND;

    if(reload_value > 255) {
        FURI_LOG_E("Demo", "reload_value: %ld", reload_value);
    }
    furi_check(reload_value > 0);
    furi_check(reload_value < 256 * 256);

    led_driver_add_period_length(timer_buffer, write_pos, reload_value - 1);
}

void sendRgbToWS2812B(const GpioPin* gpioPin, uint32_t rgb) {
    FURI_LOG_I(TAG, "Setting WS2812B color to %04x", (unsigned int)rgb);

    LL_DMA_InitTypeDef dma_gpio_update;
    LL_DMA_InitTypeDef dma_transition_timer;

    // Setup the GPIO update first
    const uint32_t bit_set = gpioPin->pin << GPIO_BSRR_BS0_Pos;
    const uint32_t bit_reset = gpioPin->pin << GPIO_BSRR_BR0_Pos;
    uint32_t gpio_buf[2] = {bit_reset, bit_set};
    FURI_LOG_I(TAG, "Setting up GPIO update");
    setupDMAGPIOUpdate(&dma_gpio_update, gpioPin, gpio_buf);

    uint16_t timer_buffer[LED_DRIVER_BUFFER_SIZE + 2];
    FURI_LOG_I(TAG, "Setting up transition timer");
    setupDMATransitionTimer(&dma_transition_timer, timer_buffer);

    furi_hal_gpio_init(gpioPin, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(gpioPin, false);

    // Setup the transition timer
    FURI_LOG_I(TAG, "Populating timer buffer with empty");
    for(size_t i = 0; i < LED_DRIVER_BUFFER_SIZE; i++) {
        timer_buffer[i] = LED_DRIVER_TIMER_SETINEL;
    }
    FURI_LOG_I(TAG, "Done");

    // Add colors here
    uint32_t write_pos = 0;
    FURI_LOG_I(TAG, "Actually setting color");
    for(int j = 0; j < MAX_LED_COUNT; j++) {
        uint32_t ggrrbb = (rgb & 0xFF) | ((rgb & 0xFF00) << 8) | ((rgb & 0xFF0000) >> 8);
        for(int i = 23; i >= 0; i--) {
            FURI_LOG_I(TAG, "Color %d", i);
            if(ggrrbb & (1 << i)) {
                led_driver_add_period(timer_buffer, &write_pos, LED_DRIVER_T0H);
                led_driver_add_period(timer_buffer, &write_pos, LED_DRIVER_T1H);
            } else {
                led_driver_add_period(timer_buffer, &write_pos, LED_DRIVER_T0L);
                led_driver_add_period(timer_buffer, &write_pos, LED_DRIVER_T1L);
            }
        }
    }
    FURI_LOG_I(TAG, "Done");

    // Number of bits written
    dma_transition_timer.NbData = write_pos + 1;

    FURI_CRITICAL_ENTER();

    FURI_LOG_I(TAG, "Starting the DMA");
    led_driver_start_dma(&dma_gpio_update, &dma_transition_timer);
    FURI_LOG_I(TAG, "Starting the timers");
    led_driver_start_timer();

    FURI_LOG_I(TAG, "Entering spin lock");
    led_driver_spin_lock(&write_pos);
    FURI_LOG_I(TAG, "Exiting spin lock");

    FURI_LOG_I(TAG, "Stopping timer");
    led_driver_stop_timer();
    FURI_LOG_I(TAG, "Stopping DMA");
    led_driver_stop_dma();

    FURI_CRITICAL_EXIT();

    FURI_LOG_I(TAG, "Clearing timer buffer");
    memset(timer_buffer, LED_DRIVER_TIMER_SETINEL, LED_DRIVER_BUFFER_SIZE);
}
