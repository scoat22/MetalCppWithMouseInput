// (We can compress these values later).
// Filled out by an Objective-C class in main.m
struct KeyboardInput
{
    int MouseX;
    int MouseY;
    int MouseDeltaX;
    int MouseDeltaY;
};

enum CursorState
{
    UnlockedShown, LockedHidden
};

// sizeof(void*) is exactly the size of a pointer on the target architecture (4 bytes on 32-bit, 8 bytes on 64-bit).

// Basically this is the only struct visible to both C/C++ and Objective-C code.
struct PlatformMemory
{
    void* view; // (MTKView*) A pointer so that we can work with Metal graphics.
    void* gameMemory; // Allocate 1 GB for the C/C++ game to work with.
    enum CursorState cursorState;
    struct KeyboardInput input;
};
