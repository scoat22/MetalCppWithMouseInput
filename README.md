![Preview Image](/Documentation/Image.png)

# Metal++ & Mouse Input
MetalCpp sample project, with mouse input directly from Mac operating system (Objective-C & Cocoa). No 3rd party libraries.

## How it works.
Objective-C calls C++ game/metal code using extern "C" function call. Since C, C++ and Objective-C all share the same memory layout for structs, there is a common struct caled "PlatformMemory". And you can put whatever code you want there. Then the Objective-C and C/C++ code can both access the struct. I put a Metal View there (MTKView). There are more comments within the code that explain things.

## Stats
main.m is ~100 lines of code of Objective-C. <br>
game.cpp is ~250 lines of C++ (mostly boilerplate Metal calls to display some cubes).
