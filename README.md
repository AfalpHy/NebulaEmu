# NebulaEmu
An NES emulator for entertainment.  

After learning about a project called [SimpleNES](https://github.com/amhndu/SimpleNES.git) by chance, I got the idea of writing my own NES emulator because I think it's really cool. I spent a lot of time reading SimpleNES's source code and NES documentation to understand the datails of how the NES operates. Finally, I figured out all the details and completed my own NES emulator(A lot of the code is similar to SimpleNES. What's different is that I modified some of the architecture and added controller support. Besides, I am trying to implement the APU, but it is not yet complete). Anyway, I would like to thank SimpleNES, which allowed me to learn about the NES emulator and I gain a lot of fun from it.

# Build
~~~sh
sudo apt install libsdl2-dev
mkdir build
cd build
cmake ..
make -j `nproc`
~~~
