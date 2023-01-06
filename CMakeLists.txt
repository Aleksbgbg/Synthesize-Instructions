cmake_minimum_required(VERSION 3.10)
project(SynthesizeInstructions)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS -m32) # Compile 32-bit executable, since we are synthesizing 32-bit assembly. Running in 64-bit mode
                          # would require additional inputs in the instruction stream to ensure our instructions are
                          # interpreted in 32-bit mode. Compiling in 32-bit mode also ensures memory addresses are
                          # 32-bit.

add_executable(${PROJECT_NAME} main.cpp)
