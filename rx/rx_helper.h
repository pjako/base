typedef struct rx_RenderCmdBuilder {
    Arena* arena;
    u32 drawCount;

    flags32 changedFlags;

    u32 capacity;
    rx_DrawList* drawList;

    rx_renderPipeline pipeline;
    rx_buffer vertexBuffer[3];
    rx_buffer indexBuffer;
    rx_resGroup resGroup1;
    rx_resGroup resGroup2;
    rx_resGroup resGroup3;
    // resGroup3 is internal for dynamic data
    u32 dynResGroup0;
    u32 dynResGroup1;

    u32 instanceOffset;
    u32 instanceCount;
} rx_RenderCmdBuilder;


INLINE void rx_renderCmdBuilderInit(Arena* arena, rx_RenderCmdBuilder* builder, u32 capacity) {
    mem_structSetZero(builder);
    builder->arena = arena;
    builder->capacity = capacity;
    builder->drawList = (rx_DrawList*) mem_arenaPushZero(arena, sizeOf(rx_DrawList) + sizeOf(u32) * capacity);
}

#define rx_renderCmdBuilderSetPipeline(BUILDER, PIPELINE)             if ((BUILDER)->pipeline.id != PIPELINE.id)            {(BUILDER)->pipeline = (PIPELINE); (BUILDER)->changedFlags |= rx_renderCmd_pipeline;}
#define rx_renderCmdBuilderSetVertexBuffer0(BUILDER, VERTEXBUFFER)    if ((BUILDER)->vertexBuffer[0].id != VERTEXBUFFER.id) {(BUILDER)->vertexBuffer[0] = (VERTEXBUFFER); (BUILDER)->changedFlags |= rx_renderCmd_vertexBuffer0;}
#define rx_renderCmdBuilderSetVertexBuffer1(BUILDER, VERTEXBUFFER)    if ((BUILDER)->vertexBuffer[1].id != VERTEXBUFFER.id) {(BUILDER)->vertexBuffer[1] = (VERTEXBUFFER); (BUILDER)->changedFlags |= rx_renderCmd_vertexBuffer1;}
#define rx_renderCmdBuilderSetVertexBuffer2(BUILDER, VERTEXBUFFER)    if ((BUILDER)->vertexBuffer[2].id != VERTEXBUFFER.id) {(BUILDER)->vertexBuffer[2] = (VERTEXBUFFER); (BUILDER)->changedFlags |= rx_renderCmd_vertexBuffer2;}
#define rx_renderCmdBuilderSetIndexBuffer(BUILDER, INDEXBUFFER)       if ((BUILDER)->indexBuffer.id != INDEXBUFFER.id)      {(BUILDER)->indexBuffer = (INDEXBUFFER); (BUILDER)->changedFlags |= rx_renderCmd_indexBuffer;}
#define rx_renderCmdBuilderSetResGroup1(BUILDER, RESGROUP)            if ((BUILDER)->resGroup1.id != RESGROUP.id)           {(BUILDER)->resGroup1 = (RESGROUP); (BUILDER)->changedFlags |= rx_renderCmd_resGroup1;}
#define rx_renderCmdBuilderSetResGroup2(BUILDER, RESGROUP)            if ((BUILDER)->resGroup2.id != RESGROUP.id)           {(BUILDER)->resGroup2 = (RESGROUP); (BUILDER)->changedFlags |= rx_renderCmd_resGroup2;}
#define rx_renderCmdBuilderSetResGroup3(BUILDER, RESGROUP)            if ((BUILDER)->resGroup3.id != RESGROUP.id)           {(BUILDER)->resGroup3 = (RESGROUP); (BUILDER)->changedFlags |= rx_renderCmd_resGroup3;}
#define rx_renderCmdBuilderSetDynResGroup0(BUILDER, RESGROUP)         if ((BUILDER)->dynResGroup0 != RESGROUP)              {(BUILDER)->dynResGroup0 = (RESGROUP); (BUILDER)->changedFlags |= rx_renderCmd_dynResGroup0;}
#define rx_renderCmdBuilderSetDynResGroup1(BUILDER, RESGROUP)         if ((BUILDER)->dynResGroup1 != RESGROUP)              {(BUILDER)->dynResGroup1 = (RESGROUP); (BUILDER)->changedFlags |= rx_renderCmd_dynResGroup1;}
#define rx_renderCmdBuilderSetInstanceOffset(BUILDER, INSTANCEOFFSET) if ((BUILDER)->instanceOffset != INSTANCEOFFSET)      {(BUILDER)->instanceOffset = (INSTANCEOFFSET); (BUILDER)->changedFlags |= rx_renderCmd_instaceOffest;}
#define rx_renderCmdBuilderSetInstanceCount(BUILDER, INSTANCECOUNT)   if ((BUILDER)->instanceCount != INSTANCECOUNT)        {(BUILDER)->instanceCount = (INSTANCECOUNT); (BUILDER)->changedFlags |= rx_renderCmd_instanceCount;}

INLINE void rx_renderCmdBuilderDraw(rx_RenderCmdBuilder* builder, u32 indexOffset, u32 indexCount, u32 vertexOffset, u32 vertexCount) {
    u32 maxDrawSize = 17 * sizeof(u32);
    rx_DrawList* drawList = builder->drawList;
    drawList->commands[drawList->count++] = builder->changedFlags;

    if (builder->changedFlags != 0) {
        if ((builder->changedFlags & rx_renderCmd_pipeline) != 0) {
            drawList->commands[drawList->count++] = builder->pipeline.id;
        }

        if ((builder->changedFlags & rx_renderCmd_vertexBuffer0) != 0) {
            drawList->commands[drawList->count++] = builder->vertexBuffer[0].id;
        }

        if ((builder->changedFlags & rx_renderCmd_vertexBuffer1) != 0) {
            drawList->commands[drawList->count++] = builder->vertexBuffer[1].id;
        }

        if ((builder->changedFlags & rx_renderCmd_vertexBuffer2) != 0) {
            drawList->commands[drawList->count++] = builder->vertexBuffer[2].id;
        }

        if ((builder->changedFlags & rx_renderCmd_indexBuffer) != 0) {
            drawList->commands[drawList->count++] = builder->indexBuffer.id;
        }

        if ((builder->changedFlags & rx_renderCmd_resGroup1) != 0) {
            drawList->commands[drawList->count++] = builder->resGroup1.id;
            if (builder->resGroup1.hasPassDep) {
                drawList->passIdxDepFlags |= rx_getResGroupPassDepFlags(builder->resGroup1);
                // get pass dep!
                //drawList->passIdxDepFlags |=  (builder->resGroup1.passIdx - 1) << 2;
            }
        }

        if ((builder->changedFlags & rx_renderCmd_resGroup2) != 0) {
            drawList->commands[drawList->count++] = builder->resGroup2.id;
            if (builder->resGroup2.hasPassDep) {
                drawList->passIdxDepFlags |= rx_getResGroupPassDepFlags(builder->resGroup2);
                //drawList->passIdxDepFlags |=  (builder->resGroup1.passIdx - 1) << 2;
            }
        }
#if 0
        if ((builder->changedFlags & rx_renderCmd_resGroup3) != 0) {
            drawList->commands[drawList->count++] = builder->resGroup3.id;
            if (builder->resGroup3.hasPassDep) {
                drawList->passIdxDepFlags |= rx_getResGroupPassDepFlags(builder->resGroup3);
                //drawList->passIdxDepFlags |=  (builder->resGroup1.passIdx - 1) << 2;
            }
        }
#endif
        if ((builder->changedFlags & rx_renderCmd_dynResGroup0) != 0) {
            drawList->commands[drawList->count++] = builder->dynResGroup0;
        }

        if ((builder->changedFlags & rx_renderCmd_dynResGroup1) != 0) {
            drawList->commands[drawList->count++] = builder->dynResGroup1;
        }

        if ((builder->changedFlags & rx_renderCmd_instanceOffset) != 0) {
            drawList->commands[drawList->count++] = builder->instanceOffset;
        }

        if ((builder->changedFlags & rx_renderCmd_instanceCount) != 0) {
            drawList->commands[drawList->count++] = builder->instanceCount;
        }
    }

    drawList->commands[drawList->count++] = indexOffset;
    drawList->commands[drawList->count++] = indexCount;
    drawList->commands[drawList->count++] = vertexOffset;
    drawList->commands[drawList->count++] = vertexCount;

    builder->changedFlags = 0;
    builder->drawCount += 1;
}


#define rx_cmdBuilderScoped(NAME, ARENA, PASS) for (mem_Scratch NAME = mem_scratchStart(ARENA);(NAME).arena; (mem_scratchEnd(&NAME), (NAME).arena = NULL))