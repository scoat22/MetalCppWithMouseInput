#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include "Game.h"

extern "C" void StartGame(PlatformMemory* platformMemory)
{
    MTK::View* view = reinterpret_cast<MTK::View*>(platformMemory->view);
    GameMemory* gameMemory = reinterpret_cast<GameMemory*>(platformMemory->gameMemory);
    Device* device = view->device();
    Error* error = nullptr;
    
    // Create command queue (on the order of miliseconds).
    // We should release on app quit, but it doesn't really matter since the app is deleted. (It's actually a faster quit without release.)
    gameMemory->queue = device->newCommandQueue();
    gameMemory->semaphore = dispatch_semaphore_create(nFramesInFlight );
    gameMemory->shaderLibrary = device->newDefaultLibrary();

    if (!gameMemory->shaderLibrary) {
        printf("Failed to load default shader library.\n");
    }
    // Configure view
    {
        view->setColorPixelFormat(PixelFormatBGRA8Unorm_sRGB);
        view->setClearColor(ClearColor::Make(0.1, 0.1, 0.1, 1.0));
        view->setDepthStencilPixelFormat(PixelFormatDepth16Unorm);
        view->setClearDepth(1.0f);
    }
    // Create render pipeline.
    {
        Function* vert = gameMemory->shaderLibrary->newFunction(String::string("vert", UTF8StringEncoding));
        Function* frag = gameMemory->shaderLibrary->newFunction(String::string("frag", UTF8StringEncoding));
        RenderPipelineDescriptor* desc = RenderPipelineDescriptor::alloc()->init();
        desc->setVertexFunction(vert);
        desc->setFragmentFunction(frag);
        desc->colorAttachments()->object(0)->setPixelFormat(PixelFormatBGRA8Unorm_sRGB);
        desc->setDepthAttachmentPixelFormat(PixelFormatDepth16Unorm);
        gameMemory->renderPipeline = device->newRenderPipelineState(desc, &error);
        if (!gameMemory->renderPipeline)
        {
            printf("%s", error->localizedDescription()->utf8String());
            assert(false);
        }
        vert->release();
        frag->release();
        desc->release();
    }
    // Depth-stencil state.
    {
        // Todo: just try allocating this on the stack and see if it works.
        DepthStencilDescriptor* desc = DepthStencilDescriptor::alloc()->init();
        desc->setDepthCompareFunction(CompareFunctionLess);
        desc->setDepthWriteEnabled(true);
        gameMemory->depthStencilState = device->newDepthStencilState(desc);
        desc->release();
    }
    // Create buffers
    {
        using simd::float3;
        const float s = 0.5f;

        shader_types::VertexData verts[] = {
            //   Positions          Normals
            { { -s, -s, +s }, { 0.f,  0.f,  1.f } },
            { { +s, -s, +s }, { 0.f,  0.f,  1.f } },
            { { +s, +s, +s }, { 0.f,  0.f,  1.f } },
            { { -s, +s, +s }, { 0.f,  0.f,  1.f } },

            { { +s, -s, +s }, { 1.f,  0.f,  0.f } },
            { { +s, -s, -s }, { 1.f,  0.f,  0.f } },
            { { +s, +s, -s }, { 1.f,  0.f,  0.f } },
            { { +s, +s, +s }, { 1.f,  0.f,  0.f } },

            { { +s, -s, -s }, { 0.f,  0.f, -1.f } },
            { { -s, -s, -s }, { 0.f,  0.f, -1.f } },
            { { -s, +s, -s }, { 0.f,  0.f, -1.f } },
            { { +s, +s, -s }, { 0.f,  0.f, -1.f } },

            { { -s, -s, -s }, { -1.f, 0.f,  0.f } },
            { { -s, -s, +s }, { -1.f, 0.f,  0.f } },
            { { -s, +s, +s }, { -1.f, 0.f,  0.f } },
            { { -s, +s, -s }, { -1.f, 0.f,  0.f } },

            { { -s, +s, +s }, { 0.f,  1.f,  0.f } },
            { { +s, +s, +s }, { 0.f,  1.f,  0.f } },
            { { +s, +s, -s }, { 0.f,  1.f,  0.f } },
            { { -s, +s, -s }, { 0.f,  1.f,  0.f } },

            { { -s, -s, -s }, { 0.f, -1.f,  0.f } },
            { { +s, -s, -s }, { 0.f, -1.f,  0.f } },
            { { +s, -s, +s }, { 0.f, -1.f,  0.f } },
            { { -s, -s, +s }, { 0.f, -1.f,  0.f } },
        };

        uint16_t indices[] = {
             0,  1,  2,  2,  3,  0, /* front */
             4,  5,  6,  6,  7,  4, /* right */
             8,  9, 10, 10, 11,  8, /* back */
            12, 13, 14, 14, 15, 12, /* left */
            16, 17, 18, 18, 19, 16, /* top */
            20, 21, 22, 22, 23, 20, /* bottom */
        };

        const size_t vertexDataSize = sizeof( verts );
        const size_t indexDataSize = sizeof( indices );

        gameMemory->vertexBuffer = device->newBuffer(vertexDataSize, ResourceStorageModeManaged);
        gameMemory->indexBuffer= device->newBuffer(indexDataSize, ResourceStorageModeManaged);
        gameMemory->vertexBuffer->setLabel(String::string("Vertex Buffer", UTF8StringEncoding));
        gameMemory->indexBuffer->setLabel(String::string("Index Buffer", UTF8StringEncoding));

        memcpy(gameMemory->vertexBuffer->contents(), verts, vertexDataSize);
        memcpy(gameMemory->indexBuffer->contents(), indices, indexDataSize);

        gameMemory->vertexBuffer->didModifyRange(Range::Make( 0, gameMemory->vertexBuffer->length() ) );
        gameMemory->indexBuffer->didModifyRange(Range::Make( 0, gameMemory->indexBuffer->length() ) );

        const size_t instanceDataSize = nFramesInFlight * nInstances * sizeof(shader_types::InstanceData);
        for (size_t i = 0; i < nFramesInFlight; ++i)
        {
            gameMemory->instanceBuffers[i] = device->newBuffer(instanceDataSize, ResourceStorageModeManaged);
            gameMemory->instanceBuffers[i]->setLabel(String::string("Instance Buffer", UTF8StringEncoding));
        }

        const size_t cameraDataSize = nFramesInFlight * sizeof( shader_types::CameraData );
        for (size_t i = 0; i < nFramesInFlight; ++i)
        {
            gameMemory->cameraBuffers[i] = device->newBuffer(cameraDataSize, ResourceStorageModeManaged);
            gameMemory->cameraBuffers[i]->setLabel(String::string("Camera Buffer", UTF8StringEncoding));
        }
    }
    
    // Will this get the width?
    //view->currentDrawable()->texture()->width();
}

extern "C" void UpdateGame(PlatformMemory* platformMemory)
{
    AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init(); // The AutoreleasePool is required.
    MTK::View* view = reinterpret_cast<MTK::View*>(platformMemory->view);
    GameMemory* gameMemory = reinterpret_cast<GameMemory*>(platformMemory->gameMemory);
    // Get the width/height of the window.
    int width = (int)view->currentDrawable()->texture()->width();
    int height = (int)view->currentDrawable()->texture()->height();

    //MTL::Device* device = view->device();
    
    // Changes the background color based on mouse's Y position.
    /*RenderPassDescriptor* pass = view->currentRenderPassDescriptor();
    if(pass)
    {
        float c = (float)platformMemory.input.MouseY / 420.0f;
        
        pass->colorAttachments()->object(0)->setLoadAction(LoadActionClear);
        pass->colorAttachments()->object(0)->setClearColor(ClearColor(c, c, c, 1));
        
        CommandBuffer* cmd = gameMemory->queue->commandBuffer();
        RenderCommandEncoder* encoder = cmd->renderCommandEncoder(pass);
        
        encoder->endEncoding();
        cmd->presentDrawable(view->currentDrawable());
        cmd->commit();
    }*/
    
    // Lock & hide cursor.
    platformMemory->cursorState = CursorState::LockedHidden;
    
    int bufferId = gameMemory->frame % nFramesInFlight;
    float angle = (float)gameMemory->frame * 0.002f;
    //float angleX = (float)platformMemory->input.MouseX / (float)width * 4;
    //float angleY = (float)platformMemory->input.MouseY / (float)height * 4;
    float angleX = (float)platformMemory->input.MouseDeltaX / (float)width * 4;
    float angleY = (float)platformMemory->input.MouseDeltaY / (float)width * 4;
    Buffer* instanceBuffer = gameMemory->instanceBuffers[bufferId];
    
    CommandBuffer* cmd = gameMemory->queue->commandBuffer();
    dispatch_semaphore_wait(gameMemory->semaphore, DISPATCH_TIME_FOREVER );
    //Renderer* pRenderer = this;
    cmd->addCompletedHandler(^void(CommandBuffer* cmd){
        dispatch_semaphore_signal(gameMemory->semaphore);
    });
    
    // Update instance positions:
    const float scl = 0.2f;
    shader_types::InstanceData* instanceData = reinterpret_cast<shader_types::InstanceData*>(instanceBuffer->contents());

    float3 objectPosition = { 0.f, 0.f, -10.f };

    float4x4 rt = math::translate(objectPosition);
    float4x4 rr1 = math::rotateY(angleX);
    float4x4 rr0 = math::rotateX(angleY);
    float4x4 rtInv = math::translate({ -objectPosition.x, -objectPosition.y, -objectPosition.z });
    float4x4 fullObjectRot = rt * rr1 * rr0 * rtInv;

    size_t ix = 0;
    size_t iy = 0;
    size_t iz = 0;
    for (size_t i = 0; i < nInstances; ++i)
    {
        if (ix == kInstanceRows)
        {
            ix = 0;
            iy += 1;
        }
        if (iy == kInstanceRows)
        {
            iy = 0;
            iz += 1;
        }

        float4x4 scale = math::scale( (float3){ scl, scl, scl } );
        float4x4 zrot = math::rotateZ(angle * sinf((float)ix));
        float4x4 yrot = math::rotateY(angle * cosf((float)iy));

        float x = ((float)ix - (float)kInstanceRows    / 2.0f) * (2.0f * scl) + scl;
        float y = ((float)iy - (float)kInstanceColumns / 2.0f) * (2.0f * scl) + scl;
        float z = ((float)iz - (float)kInstanceDepth   / 2.0f) * (2.0f * scl);
        float4x4 translate = math::translate(math::add(objectPosition, { x, y, z } ));

        instanceData[i].instanceTransform = fullObjectRot * translate * yrot * zrot * scale;
        instanceData[i].instanceNormalTransform = math::discardTranslation(instanceData[i].instanceTransform);

        float iNorm = i / (float)nInstances;
        float r = iNorm;
        float g = 1.0f - r;
        float b = sinf(M_PI * 2.0f * iNorm);
        instanceData[i].instanceColor = (float4){ r, g, b, 1.0f };

        ix += 1;
    }
    instanceBuffer->didModifyRange(Range::Make(0, instanceBuffer->length()));

    // Update camera state:
    Buffer* cameraBuffer = gameMemory->cameraBuffers[bufferId];
    shader_types::CameraData* camera = reinterpret_cast<shader_types::CameraData*>(cameraBuffer->contents());
    camera->perspectiveTransform = math::perspective(45.f * M_PI / 180.f, 1.f, 0.03f, 500.0f) ;
    camera->worldTransform = math::identity();
    camera->worldNormalTransform = math::discardTranslation(camera->worldTransform);
    cameraBuffer->didModifyRange(NS::Range::Make(0, sizeof(shader_types::CameraData)));

    // Begin render pass:
    RenderPassDescriptor* desc = view->currentRenderPassDescriptor();
    RenderCommandEncoder* encoder = cmd->renderCommandEncoder(desc);
    encoder->setRenderPipelineState(gameMemory->renderPipeline);
    encoder->setDepthStencilState(gameMemory->depthStencilState);
    encoder->setVertexBuffer(gameMemory->vertexBuffer, 0, 0);   // Offset, Index.
    encoder->setVertexBuffer(instanceBuffer, 0, 1);             // Offset, Index.
    encoder->setVertexBuffer(cameraBuffer, 0, 2 );              // Offset, Index.
    encoder->setCullMode(CullModeBack);
    encoder->setFrontFacingWinding(WindingCounterClockwise);
    encoder->drawIndexedPrimitives(PrimitiveTypeTriangle, 6 * 6, IndexTypeUInt16, gameMemory->indexBuffer, 0, nInstances);
    encoder->endEncoding();
    cmd->presentDrawable(view->currentDrawable());
    cmd->commit();
 
    gameMemory->frame++;
    pool->release();
}
