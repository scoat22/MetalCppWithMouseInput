#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>
#include "PlatformMemory.h"

extern void StartGame(struct PlatformMemory* platformMemory);
extern void UpdateGame(struct PlatformMemory* platformMemory);

// The MTKView class calls the Draw() function at a regular cadence, which passes it to Renderer, which calls UpdateGame() for us.
@interface Renderer : NSObject <MTKViewDelegate>
@property struct PlatformMemory* platformMemory;
@end

@implementation Renderer
- (void) drawInMTKView: (MTKView*)view {
    UpdateGame(_platformMemory);
    
    // Enforce cursor state
    if(_platformMemory->cursorState == LockedHidden)
    {
        CGAssociateMouseAndMouseCursorPosition(false);
        [NSCursor hide];
    }
    else
    {
        CGAssociateMouseAndMouseCursorPosition(true);
        [NSCursor unhide];
    }
}
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle resizing if needed
}
@end

// The view class handles capturing input.
@interface MyView : MTKView
@property struct PlatformMemory* platformMemory;
@end

@implementation MyView

- (BOOL)acceptsFirstResponder { return YES; }
// mouse button events
- (void)mouseDown:(NSEvent *)event {
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    //NSLog(@"mouseDown at (%.1f, %.1f) button=%lu", p.x, p.y, event.buttonNumber);
}
- (void)rightMouseDown:(NSEvent *)event {
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    //NSLog(@"rightMouseDown at (%.1f, %.1f) button=%lu", p.x, p.y, event.buttonNumber);
}
- (void)otherMouseDown:(NSEvent *)event {
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    //NSLog(@"otherMouseDown at (%.1f, %.1f) button=%lu", p.x, p.y, event.buttonNumber);
}
// mouse up
- (void)mouseUp:(NSEvent *)event {
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    //NSLog(@"mouseUp at (%.1f, %.1f) button=%lu", p.x, p.y, event.buttonNumber);
}
// dragging (mouse held)
- (void)mouseDragged:(NSEvent *)event {
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    //NSLog(@"mouseDragged at (%.1f, %.1f) delta=(%.1f, %.1f)", p.x, p.y, event.deltaX, event.deltaY);
}
// simple mouse moved (no buttons)
- (void)mouseMoved:(NSEvent *)event {
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    _platformMemory->input.MouseX = p.x;
    _platformMemory->input.MouseY = p.y;
    _platformMemory->input.MouseDeltaX = event.deltaX;
    _platformMemory->input.MouseDeltaY = event.deltaY;
}
// scroll wheel
- (void)scrollWheel:(NSEvent *)event {
    //NSLog(@"scrollWheel deltaY=%.2f deltaX=%.2f", event.scrollingDeltaY, event.scrollingDeltaX);
}
@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        NSRect windowRect = NSMakeRect(200, 300, 512, 512);
        NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
        //NSWindow *window = [[NSWindow alloc] initWithContentRect:windowRect styleMask:style backing:NSBackingStoreBuffered defer:NO];
        NSWindow *window = [[NSWindow alloc] initWithContentRect:windowRect styleMask:style backing:NSBackingStoreNonretained defer:NO];
        [window setTitle: @"Game"];
        [window center];

        // Create content view.
        MyView* view = [[MyView alloc] initWithFrame:[[window contentView] bounds]];
        view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        view.device = MTLCreateSystemDefaultDevice(); // Set metal device
        [window setContentView: view];
        
        struct PlatformMemory platformMemory = {};  // Zeroes all fields (allocated on stack).
        platformMemory.view = (__bridge void*)view; // We simply need "__bridge" to cast to void*.
        
        Renderer* renderer = [Renderer alloc];      // Create Renderer.
        renderer.platformMemory = &platformMemory;  // Set PlatformMemory reference.
        view.platformMemory = &platformMemory;      // Set PlatformMemory reference.
        view.delegate = renderer;                   // Set the view's delegate (make it recieve start/draw updates).
        
        // Allocate 1GB game memory.
        platformMemory.gameMemory = malloc(1024L * 1024L * 1024L);
        
        StartGame(&platformMemory);
        
        [window setAcceptsMouseMovedEvents:YES];    // Enable mouse moved events (important).
        [window makeFirstResponder:view];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        [window makeKeyAndOrderFront:nil];          // Show window and activate app.
        [app activateIgnoringOtherApps:YES];
        
        // Enable Cmd + Q to quit.
        NSMenu *menuBar = [[NSMenu alloc] init];
        NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
        [menuBar addItem:appMenuItem];
        [app setMainMenu:menuBar];
        NSMenu *appMenu = [[NSMenu alloc] init];
        NSString *quitTitle = [NSString stringWithFormat:@"Quit %@", [[NSProcessInfo processInfo] processName]];
        NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
        [appMenu addItem:quitMenuItem];
        [appMenuItem setSubmenu:appMenu];
        
        // Run the app main loop.
        [app run];
    }
    return 0;
}
