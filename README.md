# NebulaEmu
An NES emulator for entertainment.  

After learning about a project called [SimpleNES](https://github.com/amhndu/SimpleNES.git) by chance, I got the idea of writing my own NES emulator because I think it's really cool. I spent a lot of time reading SimpleNES's source code and NES documentation to understand the datails of how the NES operates. Finally, I figured out all the details and completed my own NES emulator(A lot of the code is similar to SimpleNES. What's different is that I modified some of the architecture and added controller support. Furthermore, I am trying to implement the APU. So far, the pulse, triangle, and noise channels have been implemented, but the DMC channel is still incomplete. Nonetheless, it is sufficient for playing game sounds). Anyway, I would like to thank SimpleNES, which allowed me to learn about the NES emulator and I gain a lot of fun from it.

# Build
## Ubuntu22.04/WSL
~~~sh
sudo apt install libsdl2-dev
mkdir build
cd build
cmake ..
make -j `nproc`
~~~

## Windows
- Download SDL2-devel-2.26.5-mingw.zip from [SDL2](https://github.com/libsdl-org/SDL/releases/tag/release-2.26.5), and then extract it
- Create an environment variable named `SDL2_INCLUDE` with the value `{path to SDL2}\x86_64-w64-mingw32\include`
- Create an environment variable named `SDL2_LIB` with the value `{path to SDL2}\x86_64-w64-mingw32\bin`
- Add the value `{path to SDL2}\x86_64-w64-mingw32\bin` to the environment variable PATH
- Install MinGW64 and CMake
~~~sh

mkdir build
cd build
cmake -G "Unix Makefiles" ..
make -j `nproc`
~~~