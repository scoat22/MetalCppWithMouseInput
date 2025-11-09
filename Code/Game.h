#include <cassert>
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>
#include <time.h>
#include "PlatformMemory.h"
#include "math.h"
#include "shader_types.h"

using namespace NS;
using namespace MTL;
using namespace simd;

// Allow Objective-C to call UpdateGame.
extern "C" void StartGame(PlatformMemory*);
extern "C" void UpdateGame(PlatformMemory*);

static constexpr size_t kInstanceRows = 10;
static constexpr size_t kInstanceColumns = 10;
static constexpr size_t kInstanceDepth = 10;
static constexpr size_t nInstances = (kInstanceRows * kInstanceColumns * kInstanceDepth);
static constexpr size_t nFramesInFlight = 3;

// We can put stuff here that just lives in C/C++ land.
struct GameMemory
{
    Library* shaderLibrary;
    CommandQueue* queue;
    RenderPipelineState* renderPipeline;
    DepthStencilState* depthStencilState;
    dispatch_semaphore_t semaphore;
    int frame; // Increases every frame.
    
    Buffer* instanceBuffers[nFramesInFlight];
    Buffer* cameraBuffers[nFramesInFlight];
    Buffer* vertexBuffer;
    Buffer* indexBuffer;
};
