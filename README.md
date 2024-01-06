# Light Up

A Flipper Zero application for controlling addressable LEDs, requires uFBT to build.

## Screenshots

![Alt text](/.flipcorg/gallery/screenshot-1.png?raw=true "Banner")

## Dependencies

This project depends on [uFBT](https://github.com/flipperdevices/flipperzero-ufbt) for building. It is also recommended to install VSCode for ease of development, as well as [minicom](https://en.wikipedia.org/wiki/Minicom) for debugging (`brew install minicom`).

## App Source File Structure

The application is structured such that each scene can be self contained, with the sharing of data between scenes done via the app context. All of the scenes can be found with their corresponding source and header files in the `src/scenes` directory. The scenes do as follows:

- **Starting Scene**: The scene where the application starts. Simply displays "Hello World" right now.

Note as well that the app context file is generic, and designed in such as way that it should not need to be updated for things specific to the application. This allows for an easier time to allow scenes to self manage, insteaed of having somewhere else that centrally manages everything.

## Helpful Commands

- `ufbt`: Builds the project
- `ufbt launch`: Launches the project on a device. Make sure no other applications (including qFlipper) are connected to the device.
- `minicom -D /dev/tty.X`: Replace `X` with the name of your flipper device when connected and then use this to start a command line interface to your flipper device. From there, you can run `log debug` to see debug logs from the app while it is running.
