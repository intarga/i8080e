# i8080e

A simple emulator for the Intel 8080, along with other parts of the Space Invaders arcade system. Tested with the CPUDIAG routine, and space invaders.

This is a work in progress. CPU emulation is complete, but I/O remains to be implemented.

## Building and running

### Supported systems

Only tested with x86_64 Linux, but probably anything with a C compiler and SDL backend will probably work fine.

### Dependencies

- glibc (Other C standard library implementations would probably work too) <version>

- LibSDL 2 <version>

### Building

Run `$ make release` in the project directory to build, it puts the executable at `bin/i8080e`

### Getting the ROM

I can't distribute the ROM here for obvious copyright reasons, but it's not particularly hard to find.

Once you have it, extract it to `<project_directory>/rom`, and then concatenate the files together like so:

`$ cat invaders.h invaders.g invaders.f invaders.e > invaders`

### Running

Simply execute the binary from the project directory to run.

`$ bin/i8080e`

no arguments or flags are needed and it'll pick up and load the ROM from its directory.
