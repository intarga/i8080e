# 8080-emulator
A simple emulator for the Intel 8080. It's only tested for the space invaders arcade ROM, because that's the only software I have for it.

## Building and running

### Supported systems

Only Linux??

### Dependencies

- gcc <version>

- OpenGL?? <version>

### Building

Run `$ make release` in the project directory to build, it spits out two binaries: 8080-disassembler, and 8080-emulator, which both do what they say on the tin.

### Getting the ROM

I can't distribute the ROM here for obvious copyright reasons, but it's not particularly hard to find.

Once you have it, extract it to `<project_directory>/rom`, and then concatenate the files together like so:

`cat invaders.h invaders.g invaders.f invaders.e > invaders`

### Running

Simply execute the binaries in the project directory to run them, no arguments or flags are needed they'll pick up and load the ROM from its directory.
