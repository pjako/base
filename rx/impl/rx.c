
#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_math.h"
#include "base/base_str.h"

#include "os/os.h"

#define RX_INTERNAL
#include "rx/rx.h"

#define RX_OGL


#ifdef RX_OGL
#undef RX_OGL
#define RX_OGL 1
#else
#define RX_OGL 0
#endif

#ifndef RX_UNREACHABLE
    #define RX_UNREACHABLE() ASSERT(false)
#endif // RX_UNREACHABLE

//#if RX_METAL
//#import <QuartzCore/CAMetalLayer.h>
//#endif
#if RX_OGL
#if OS_APPLE
#include <TargetConditionals.h>
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif

#if OS_IOS
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#import <UIKit/UIKit.h>
#else
#include <OpenGL/gl3.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#define RX_GLCORE33
#endif
#elif OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment (lib, "kernel32")   // GetProcAddress()
#define RX_USE_WIN32_GL_LOADER

#if defined(RX_USE_WIN32_GL_LOADER)
typedef unsigned int  GLuint;
#endif

#else
#error "Undefined platform"
#endif
#endif

// todo: embed the library
#include "offalloc.c"

#if OS_APPLE
#if __has_feature(objc_arc)
#define RX_OBJC_RELESE(obj) { obj = nil; }
#else
#define RX_OBJC_RELESE(obj) { [obj release]; obj = nil; }
#endif
#endif

typedef struct rx_Shader {
    union {
#if RX_OGL
        GLuint handle;
#endif
    };
    u32 gen : 16;
} rx_Shader;

typedef struct rx_Buffer {
    union {
#if RX_OGL
        struct {
            GLuint handle;
            rx_bufferUsageFlags usage;
        } gl;
#endif
    };
    u32                 gen : 16;
    u64                 alignment;
    u64                 allocatedSize;
    u64                 size;
    rx_bufferUsageFlags usage;
} rx_Buffer;

typedef struct rx_Sampler {
    union {
#if RX_OGL
        struct {
            GLuint handle;
        } gl;
#endif
    };
    u32 gen : 16;
} rx_Sampler;

typedef struct rx_Texture {
    union {
#if RX_OGL
        struct {
            GLuint handle;
        } gl;
#endif
    };
    rx_texture ref;
    bx belongsToSwapChain : 1;
    u32 gen : 16;
} rx_Texture;

typedef struct rx_TextureView {
    union {
#if RX_OGL
        struct {
            GLuint handle;
        } gl;
#endif
    };
    u32 gen : 7;
    rx_texture textureRef;
} rx_TextureView;

typedef struct rx_RenderShader {
    union {
#if RX_OGL
        struct {
            GLuint handle;
        } gl;
#endif
    };
    u32 gen : 16;
} rx_RenderShader;


#if RX_OGL
typedef struct rx__GlVertexAttribute {
    i8 vbIndex;        // -1 if attr is not enabled
    i8 divisor;         // -1 if not initialized
    i8 stride;
    i8 size;
    bx normalized : 1;
    i32 offset;
    GLenum type;
} rx__GlVertexAttribute;
#endif

typedef struct rx_RenderPipeline {
    union {
#if RX_OGL
        struct {
            rx_ColorTargetState colorTargets[RX_MAX_COLOR_TARGETS];
            rx__GlVertexAttribute vertexAttributes[RX_MAX_VERTEX_ATTRIBUTES];
            rx_renderShader shader;
            GLenum indexType;
            GLenum primitiveTopology;
            bx depthWriteEnabled : 1;
            bx stencilEnabled : 1;
            bx alphaToCoverageEnabled : 1;
            bx useInstancedDraw : 1;
            rx_cullMode cullMode;
            rx_faceWinding faceWinding;
            u32 sampleCount;
            rx_DepthStencilState depthStencilState;
            rx_RasterizerState rasterizerState;
            rx_TargetBlendState blend;
            u8 colorWriteMask[RX_MAX_COLOR_TARGETS];
            u8 colorTargetCount;
        } gl;
#endif
    };
    u32 gen : 16;
} rx_RenderPipeline;

typedef enum rx__resType {
    rx__resType_none,
    rx__resType_texture,
    rx__resType_sampler,
} rx__resType;

typedef struct rx_ResGroupRes {
    union {
        u32 id;
        rx_texture texture;
        rx_sampler sampler;
    };
    rx__resType type : 2;
} rx_ResGroupRes;

typedef struct rx_ResGroup {
    union {
        struct {
            rx_ResGroupRes resources[RX_MAX_RESOURCES_PER_RES_GROUP];
        } gl;
    };
    rx_resGroupLayout layout;
    rx_buffer target;
    u32 targetOffset;
    u32 uniformSize;
    rx_resGroupUsage usage : 2;
    u64 lastUpdateFrameIdx;
    flags64 passDepFlags;
    u32 gen : 16;
} rx_ResGroup;

typedef struct rx_ResGroupLayout {
    union {
        struct {
            rx_ResLayoutDesc resources[12];
        } gl;
    };
    u32 gen : 16;
    u32 uniformSize;
} rx_ResGroupLayout;

typedef struct rx_SwapChain {
    rx_texture textures[RX_MAX_SWAP_TEXTURES];
    u32 textureCount : 2;
    u32 gen : 16;
} rx_SwapChain;

typedef enum rx_api {
    rx_api_default = 0,
    rx_api_ogl,
    rx_api_vk,
    rx_api_mtl,
    rx_api_dx12,
} rx_api;

typedef struct rx_IndexQueue {
    u64 out;
    u64 in;
    u32* indicies;
    u32 size;
} rx_IndexQueue;

#define rx_arrDef(TYPE) struct {TYPE* elements; u32 count; u32 capacity;}
#define rx_arrInit(ARENA, ARR, COUNT) (ARR)->elements = mem_arenaPush((ARENA), sizeOf((ARR)->elements[0]) * (COUNT) ); (ARR)->capacity = (COUNT)
#define rx__poolDef(TYPE) struct {TYPE* elements; u32 capacity; rx_IndexQueue freeIndicies; rx_IndexQueue trashedIndicies; u64 frameOffets[4];}
#define rx__poolInit(ARENA, POOL, COUNT) (POOL)->elements = mem_arenaPush((ARENA), sizeOf((POOL)->elements[0]) * (COUNT) ); (POOL)->capacity = (COUNT); rx__queueInit((ARENA), &(POOL)->freeIndicies, (COUNT), (COUNT)); rx__queueInit((ARENA), &(POOL)->trashedIndicies, (COUNT), (COUNT)); mem_structSetZero((POOL)->frameOffets)
// rx_poolInit(ctx->.arena, &ctx->buffers, descWithDefaults.maxBuffers);
// unsave queue
#define rx__frameQueueDef(TYPE, CAPACITY, OFFSETCOUNTS) struct {TYPE* indicies; u64 currIdx; u64 capacity; u64 offsets[OFFSETCOUNTS];}

typedef struct rx_ColorTarget {
    rx_texture target;
    rx_loadOp loadOp : 2;
    rx_storeOp storeOp : 2;
    Rgba clearColor;
} rx_ColorTarget;

typedef enum rx_passType {
    rx_passType_render,
    rx_passType_compute
} rx_passType;

typedef struct rx_RenderPass {
    union {
        struct {
            GLuint frameBufferIdx;
            GLuint msaaResolveFramebuffer[RX_MAX_COLOR_TARGETS];
        } gl;
    };
    rx_DrawArea* areas;
    u32 areaCount;
    rx_DrawList* drawList;
    u32 colorAttachmentCount : 3;
    struct {
        bx enabled : 1;
        rx_loadOp loadOp : 2;
    } depth;
    struct {
        bx enabled : 1;
        rx_loadOp loadOp : 2;
    } stencil;
    rx_ColorTarget colorTargets[RX_MAX_COLOR_TARGETS];
    int width;
    int height;
} rx_RenderPass;

typedef struct rx_Pass {
    rx_passType type : 2;
    u16 passRef; // references PassDefinition (used in Vulkan)
    union {
        rx_RenderPass renderPass;
#if 0
        struct {

        } computePass;
#endif
    };
} rx_Pass;

typedef struct rx_PassDependencies {
    flags64 passReadDeps;
    flags64 passWriteDeps;
} rx_PassDependencies;

typedef struct rx__GraphNode {
    //rx__Res* deps; // count is always max slots
    uint32_t executionQueueIndex;
    uint32_t globalExecutionIndex;
    uint32_t localToDependencyLevelExecutionIndex;
    uint32_t localToQueueExecutionIndex;
} rx__GraphNode;

#if 0
typedef struct rx_AdjacencyPassList {
    uint32_t count;
    uint32_t* dependencyPassIndicies;
} rx_AdjacencyPassList;
#endif

typedef struct rx__QueueWork {
    uint32_t count;
    uint16_t* graphNodes;
} rx__QueueWork;

typedef struct rx__DependencyLevel {
    uint32_t depCount;
    uint32_t* deps;
    uint32_t crossQueResReadsInvolvementFlags;
} rx__DependencyLevel;


typedef struct rx__SyncCoverage {
    uint32_t nodeIdx;
    uint64_t syncedQueueIdxBitfield;
    uint32_t syncedQueueCount;
} rx__SyncCoverage;


typedef struct rx_FrameGraph {
    u32 maxPasses;
    u32 maxQueues;
    u32 passCount;
    u32 maxSlotsPerNode;
    u32 dependenciesPerPass;

    rx_arrDef(rx_PassDependencies) dependencies;
    rx_arrDef(flags64) dependants;

    rx_arrDef(u8) executionQueues;
    rx_arrDef(rx__GraphNode) nodes;
    rx_arrDef(u16) nodesInGlobalExecutionOrder;
    rx_arrDef(u16) nodesToSyncWith;

    rx_arrDef(u32) tempPerQueuePreviousNodes;
    rx_arrDef(u16) tempPerQueueNodes;
    rx_arrDef(u32) synchronizationIndexSet;
    //rx_arrDef(u16) tempSyncCoverage;
    rx_arrDef(u16) hasAnyDependencies;
    rx_arrDef(u16) topologicallySortedNodes;
    rx_arrDef(u16) longestDistances;


    rx_arrDef(u16) syncSignalRequired;

    rx_arrDef(rx__SyncCoverage) tempSyncCoverage;



    struct {
        rx_arrDef(rx__DependencyLevel) deplevels;
        rx_arrDef(u32) nodes;
        struct {
            rx_arrDef(rx__QueueWork) queueWork;
            //rx_arrDef(u16) nodeIndicies;
        } queuesPerDepLevel;
    } dependencyLevels;

    flags64 visited;
    flags64 onStack;

    rx_arrDef(u32)  queues;
    rx_arrDef(u16) queueEntries;

    
    //rx_arrDef(rx_AdjacencyPassList) adjacencyLists;

} rx_FrameGraph;

typedef struct rx_Features {
    bx originTopLeft;
    bx imageClampToBorder;
    bx mrtIndependentBlendState;
    bx mrtIndependentWriteMask;
} rx_Features;

typedef struct rx_Limits {
    u32 maxImageSize2d;
    u32 maxImageSizeArray;
    u32 maxImageSizeCube;
    u32 maxVertexAttributes;
    u32 glMaxVertexUniformVectors;
    u32 maxImageSize3d;
    u32 glMaxCombinedTextureImageUnits;
    u32 maxImageArrayLayers;
    u32 uniformBufferAlignment;
} rx_Limits;

typedef struct rx_Ctx {
    rx_api api;
    Arena* arena;
    rx_error error;
    u64 frameIdx;
    rx_Features features;
    rx_Limits limits;
    rx__poolDef(rx_Buffer) buffers;
    rx__poolDef(rx_Texture) textures;
    rx__poolDef(rx_Sampler) samplers;
    rx__poolDef(rx_RenderShader) renderShaders;
    rx__poolDef(rx_RenderPipeline) renderPipelines;
    rx__poolDef(rx_ResGroup) resGroups;
    rx__poolDef(rx_ResGroupLayout) resGroupLayouts;
    rx__poolDef(rx_SwapChain) swapChains;


    rx_swapChain defaultSwapChain;
    
    
    rx_arrDef(rx_Pass) passes;
    rx_FrameGraph frameGraph;
    
    //rx__poolDef(rx_Texture) textures;
} rx_Ctx;


#define rx_valueOrDefault(VAR, DEFAULT) (VAR) != 0 ? (VAR) : (DEFAULT)

// Shared code


// IndexQueu implementation

enum {
    RX__INVALID_INDEX = RX_U32_MAX
};

#define rx__queueRingEntry(queue, index) (queue->indicies + (index & (queue->size - 1)))

void rx__queueInit(Arena* arena, rx_IndexQueue* queue, u32 size, u32 endIndex) {
    u32_nextPowerOfTwo(size);
    queue->in = 0;
    queue->out = 0;
    queue->size = size;
    queue->indicies = mem_arenaPushArray(arena, u32, size);
    u32 idx = 0;
    for (; idx < endIndex; idx++) {
        queue->indicies[idx] = idx + 1;
    }
    queue->in = idx;
    u32 zeroSize = (queue->size - idx);
    if (zeroSize > 0) {
        mem_setZero(&queue->indicies[idx], zeroSize * sizeOf(u32));
    }
}


bool rx__queuePush(rx_IndexQueue* queue, u32 index) {
    if ((queue->in + 1) == queue->out) {
        // queue full
        return false;
    }

    u32* idxPtr = rx__queueRingEntry(queue, queue->in);
    *idxPtr = (index + 1);
    queue->in += 1;

    return false;
}

u32 rx__queuePull(rx_IndexQueue* queue) {
    if (queue->in <= (queue->out + 1)) {
        // queue empty
        return RX__INVALID_INDEX;
    }
    u32 idx = queue->out;
    queue->out += 1;

    idx = (idx & (queue->size - 1));
    u32 out = queue->indicies[idx] - 1;
    ASSERT(out != RX__INVALID_INDEX);
    queue->indicies[idx] = 0;
    return out;
}

// RenderPass implementation


#if 0
inline uint64_t rx__nextBit32(uint64_t x) {
  for (uint32_t i = 0; i < 512; i++) {
    if (x & (1 << i)) {
      return i;
    }
  }
  return 0;
}
#endif

#ifdef OS_WINDOWS
#include <intrin.h>
static inline uint64_t rx__nextBitIdx(uint64_t bitflield) {
    uint32_t out = 0;
    _BitScanForward64(&out, bitflield);
    return out;
}

#else
#define rx__nextBitIdx(V) __builtin_ctzll(V)
//#define rx_ctzll64(val, out) out = __builtin_ctzll(val)
//#define rx_ctzll32(val, out) out = __builtin_ctz(val)
// #define rx_forEachBitflag64(BITFIELD) for (uint64_t val = BITFIELD, idx = 0, bitIdx = __builtin_ctzll(BITFIELD); val != 0; (val = val << idx, idx = rx_ctzll64(val, idx), bitIdx += idx))
#endif
#define rx_forEachBitflag64(BITFIELD) for (uint64_t val = BITFIELD, idx = rx__nextBitIdx(BITFIELD), bitIdx = rx__nextBitIdx(BITFIELD); val != 0; (val = val >> (idx + 1), idx = rx__nextBitIdx(val), bitIdx += (idx + 1)))

LOCAL void rx__initFrameGaph(Arena* arena, rx_FrameGraph* passGraph, u32 passCount, u32 maxQueues) {
    passGraph->maxPasses = passCount;
    passGraph->maxQueues = maxQueues;
    passGraph->maxSlotsPerNode = 8;//desc->maxSlotsPerNode ? desc->maxSlotsPerNode : 8;
    passGraph->passCount = 0;
    passGraph->dependenciesPerPass = 7;//desc->maxSlotsPerNode ? desc->maxSlotsPerNode : 8;
    //rx__queueInit(&passGraph->usedTextureIndicies, 64, 0);
    
    rx_arrInit(arena, &passGraph->dependencies, passGraph->maxPasses);
    rx_arrInit(arena, &passGraph->dependants, passGraph->maxPasses);
    //rx_arrInit(arena, &passGraph->passes, passGraph->maxPasses);
    
    rx_arrInit(arena, &passGraph->executionQueues, passGraph->maxPasses);
    
    // rx_arrInit(arena, &passGraph->dependencySlotFlags, passGraph->maxPasses * passGraph->maxSlotsPerNode);
    
    rx_arrInit(arena, &passGraph->nodes, passGraph->maxPasses);
    
    // TODO: 8 slots per pass (make this dynamic maybe)
    // uint32_t totalDepCount = passGraph->maxPasses * passGraph->maxSlotsPerNode;
    // rx_arrInit(arena, &passGraph->slotDeps, totalDepCount);
    // rx_arrInit(arena, &passGraph->slotResources, totalDepCount);
    // rx_arrInit(arena, &passGraph->resMultipleQeuesReadsBitFields, totalDepCount);
    
    //rx_arrInit(arena, &passGraph->adjacencyLists._entries, totalDepCount);
    
    

    rx_arrInit(arena, &passGraph->nodesInGlobalExecutionOrder, passGraph->maxPasses);
    
    //rx_arrInit(arena, &passGraph->adjacencyLists, passGraph->maxPasses);
    
    rx_arrInit(arena, &passGraph->nodesToSyncWith, passGraph->maxPasses);
    
    rx_arrInit(arena, &passGraph->tempPerQueuePreviousNodes, passGraph->maxQueues);
    rx_arrInit(arena, &passGraph->tempPerQueueNodes, passGraph->maxQueues);
    rx_arrInit(arena, &passGraph->synchronizationIndexSet, passGraph->maxPasses * passGraph->maxQueues);
    rx_arrInit(arena, &passGraph->tempSyncCoverage, passGraph->maxPasses);
    
    rx_arrInit(arena, &passGraph->hasAnyDependencies, passGraph->maxPasses);
    
    rx_arrInit(arena, &passGraph->topologicallySortedNodes, passGraph->maxPasses);
    
    rx_arrInit(arena, &passGraph->longestDistances, passGraph->maxPasses);
    
    //rx_arrInit(arena, &passGraph->tempResQueueMap, passGraph->maxSlotsPerNode);
    
    rx_arrInit(arena, &passGraph->syncSignalRequired, passGraph->maxPasses);
    
    uint32_t maxQueuesMultMaxNodes = passGraph->maxPasses * passGraph->maxQueues;
    
    rx_arrInit(arena, &passGraph->dependencyLevels.deplevels, passGraph->maxQueues);
    
    rx_arrInit(arena, &passGraph->dependencyLevels.queuesPerDepLevel.queueWork, maxQueuesMultMaxNodes);
    
    //rx_arrInit(arena, &passGraph->dependencyLevels.queuesPerDepLevel.nodeIndicies, passGraph->maxPasses);
    rx_arrInit(arena, &passGraph->dependencyLevels.nodes, passGraph->maxPasses);

    //rx_arrInit(arena, &passGraph->visited, passGraph->maxPasses);
    //rx_arrInit(arena, &passGraph->onStack, passGraph->maxPasses);
    
    rx_arrInit(arena, &passGraph->queues, passGraph->maxQueues);
    rx_arrInit(arena, &passGraph->queueEntries, passGraph->maxPasses);
    
    // setup execution
    
    // for now we only have one queue
}

#define rx__bitfieldContainsFlags(BITFIELD, FLAGS) ((BITFIELD & (FLAGS)) == (FLAGS))
#define rx__bitfieldRemoveFlags(BITFIELD, FLAGS) ((BITFIELD & ~(FLAGS)))
#define rx__graphGetIdx(VAL) ((VAL) - 1)

#define rx__idxToU64Flag(IDX) (u64_val(1) << u64_cast(IDX))

#define rx__passHasAnyDeps(PASSGRAPH, PASSIDX) ((PASSGRAPH)->dependencies.elements[PASSIDX].passReadDeps != 0 || (passGraph->dependencies.elements[PASSIDX].passWriteDeps != 0))
LOCAL void rx__buildAdjacencyLists(rx_FrameGraph* passGraph) {
    ASSERT(passGraph);
    u32 totalPasses = passGraph->passCount;
    for (u32 passIdx = 0; passIdx < totalPasses; passIdx++) {
        passGraph->dependants.elements[passIdx] = 0;
        
        const u8 executionQueue = passGraph->executionQueues.elements[passIdx];
        flags64 passFlag = rx__idxToU64Flag(passIdx);
        for (u32 otherPassIdx = 0; otherPassIdx < totalPasses; otherPassIdx++) {
            if (otherPassIdx == passIdx) {
                continue;
            }
            flags64 dep = (passGraph->dependencies.elements[otherPassIdx].passReadDeps & passFlag) | (passGraph->dependencies.elements[otherPassIdx].passWriteDeps & passFlag);
            if (dep != 0) {
                passGraph->syncSignalRequired.elements[passIdx] = true;
                flags64 otherPassFlag = rx__idxToU64Flag(otherPassIdx);
                passGraph->dependants.elements[passIdx] |= otherPassFlag;
                const u8 otherExecutionQueue = passGraph->executionQueues.elements[otherPassIdx];
                if (executionQueue != otherExecutionQueue) {
                    passGraph->nodesToSyncWith.elements[otherPassIdx] |= otherPassFlag;
                }
            }
        }
    }
    #if 0
    passGraph->adjacencyLists._entries.count = 0;
    const uint32_t totalPasses = passGraph->passCount;
    // for each node, iterate over all other nodes if it got a direct dependency on it an store it in its adjastance list
    for (uint32_t nodeIdx = 0; nodeIdx < totalPasses; nodeIdx++) {
        rx__AdjacencyPassList* adjacencyPassList = passGraph->adjacencyLists.elements + nodeIdx;
        adjacencyPassList->count = 0;
        if (passGraph->hasAnyDependencies.elements[nodeIdx] == false) {
            adjacencyPassList->dependencyPassIndicies = NULL;
            continue;
        }
        
        // TODO: Check if no one depends on this pass to shortcut this
        
        adjacencyPassList->dependencyPassIndicies = passGraph->adjacencyLists._entries.elements + passGraph->adjacencyLists._entries.count;
        
        const uint8_t executionQueue = passGraph->executionQueues.elements[nodeIdx];
        for (uint32_t otherNodeIdx = 0; otherNodeIdx < totalPasses; otherNodeIdx++) {
            // cant depend on itself
            if (otherNodeIdx == nodeIdx) {
                continue;
            }
            const uint8_t otherExecutionQueue = passGraph->executionQueues.elements[otherNodeIdx];

            rx__Res* otherDeps = passGraph->slotDeps.elements + (otherNodeIdx * passGraph->maxSlotsPerNode);
            for (uint32_t depIdx = 0; depIdx < passGraph->maxSlotsPerNode; depIdx++) {
                rx__Res* od = &otherDeps[depIdx];
                if (od->dep.pass.idx == 0) {
                    continue;
                }
                uint32_t reqIdx = rx__graphGetIdx(od->dep.pass.idx);
                if (reqIdx == nodeIdx) {
                    passGraph->syncSignalRequired.elements[nodeIdx] = true;
                    adjacencyPassList->dependencyPassIndicies[adjacencyPassList->count++] = otherNodeIdx;
                    if (executionQueue != otherExecutionQueue) {
                        passGraph->nodesToSyncWith.elements[otherNodeIdx] |= 1 << nodeIdx;
                    }
                }
            }

            if (adjacencyPassList->count > 0) {
                passGraph->adjacencyLists._entries.count += adjacencyPassList->count;
            } else {
                adjacencyPassList->dependencyPassIndicies = NULL;
            }
        }
    }
    #endif
}

// returns value is true when its cyclic
LOCAL bool rx__depthFirstSearch(rx_FrameGraph* passGraph, uint32_t nodeIdx) {
    bool isCyclic = false;

    flags64 passFlag = rx__idxToU64Flag(nodeIdx);

    passGraph->onStack |= passFlag;
    passGraph->visited |= passFlag;

    flags64 dependants = passGraph->dependants.elements[nodeIdx];

    rx_forEachBitflag64(dependants) {
        uint32_t neighbourIdx = idx; // adjacencyPassList->dependencyPassIndicies[idx];
        flags64 neighbourPassFlag = rx__idxToU64Flag(neighbourIdx);
        
        if (rx__bitfieldContainsFlags(passGraph->onStack, neighbourPassFlag) && rx__bitfieldContainsFlags(passGraph->visited, neighbourPassFlag)) {
            return true;
        }
        
        if (!rx__bitfieldContainsFlags(passGraph->visited, neighbourPassFlag)) {
            isCyclic = rx__depthFirstSearch(passGraph, neighbourIdx) || isCyclic;
        }
    }

    passGraph->onStack &= passFlag;
    // start at the end of the list and push indicies towards the front
    // so we don't need to reverse the list when its done building
    passGraph->topologicallySortedNodes.elements[passGraph->passCount - (++passGraph->topologicallySortedNodes.count)] = nodeIdx;

    return isCyclic;
#if 0

    rx__AdjacencyPassList* adjacencyPassList = passGraph->adjacencyLists.elements + nodeIdx;

    for (uint32_t idx = 0; idx < adjacencyPassList->count; idx++) {
        uint32_t neighbourIdx = adjacencyPassList->dependencyPassIndicies[idx];
        if (passGraph->onStack.elements[neighbourIdx] && passGraph->visited.elements[neighbourIdx]) {
            return true;
        }
        
        if (!passGraph->visited.elements[neighbourIdx]) {
            isCyclic = rx__depthFirstSearch(passGraph, neighbourIdx) || isCyclic;
        }
    }
    
    passGraph->onStack &= passFlag;
    // start at the end of the list and push indicies towards the front
    // so we don't need to reverse the list when its done building
    passGraph->topologicallySortedNodes.elements[passGraph->passCount - (++passGraph->topologicallySortedNodes.count)] = nodeIdx;
    return isCyclic;
#endif
}

LOCAL void rx__topologicalSort(rx_FrameGraph* passGraph) {
    passGraph->visited = 0;
    passGraph->onStack = 0;
    if (passGraph->passCount == 1) {
        passGraph->topologicallySortedNodes.count = 1;
        passGraph->topologicallySortedNodes.elements[0] = 0;
        return;
    }
    bool isCyclic = false;
    passGraph->topologicallySortedNodes.count = 0;
    for (uint32_t nodeIdx = 0; nodeIdx < passGraph->passCount; nodeIdx++) {
        flags64 nodeFlag = rx__idxToU64Flag(nodeIdx);
        
        if (!rx__bitfieldContainsFlags(passGraph->visited, nodeFlag) && rx__passHasAnyDeps(passGraph, nodeIdx)) {
            isCyclic = rx__depthFirstSearch(passGraph, nodeIdx) || isCyclic;
            ASSERT(isCyclic == false && "Detected cyclic dependency!");
        }
    }
    ASSERT(passGraph->passCount == passGraph->topologicallySortedNodes.count);
}

LOCAL void rx__buildDependencyLevels(rx_FrameGraph* passGraph) {
    uint32_t dependencyLevelCount = 1;
    
    mem_arrSetZero(passGraph->longestDistances.elements, passGraph->passCount);

    // Perform longest node distance search
    for (uint32_t passIdx = 0; passIdx < passGraph->passCount; passIdx++) {
        flags64 dependants = passGraph->dependants.elements[passIdx];

        rx_forEachBitflag64(dependants) {
            u32 adjacentNodeIndex = idx;
            if (passGraph->longestDistances.elements[adjacentNodeIndex] < (passGraph->longestDistances.elements[passIdx] + 1)) {
                uint32_t newLongestDistance = passGraph->longestDistances.elements[passIdx] + 1;
                passGraph->longestDistances.elements[adjacentNodeIndex] = newLongestDistance;
                newLongestDistance += 1;
                dependencyLevelCount = maxVal(newLongestDistance, dependencyLevelCount);
            }
        }
    }
    
    mem_arrSetZero(passGraph->dependencyLevels.deplevels.elements, dependencyLevelCount);
    passGraph->dependencyLevels.deplevels.count = dependencyLevelCount;
    // figure out how many deps we got per
    for (uint32_t nodeIdx = 0; nodeIdx < passGraph->passCount; nodeIdx++) {
        uint32_t depLevel = passGraph->longestDistances.elements[nodeIdx];
        passGraph->dependencyLevels.deplevels.elements[depLevel].depCount += 1;
    }
    // size deps level arrays
    passGraph->dependencyLevels.nodes.count = 0;
    for (uint32_t depIdx = 0; depIdx < dependencyLevelCount; depIdx++) {
        passGraph->dependencyLevels.deplevels.elements[depIdx].deps = passGraph->dependencyLevels.nodes.elements + passGraph->dependencyLevels.nodes.count;
        uint32_t count = passGraph->dependencyLevels.deplevels.elements[depIdx].depCount;
        passGraph->dependencyLevels.nodes.count += count;
        passGraph->dependencyLevels.deplevels.elements[depIdx].depCount = 0;
    }
    
    for (uint32_t idx = 0; idx < passGraph->topologicallySortedNodes.count; idx++) {
        uint32_t nodeIdx = passGraph->topologicallySortedNodes.elements[idx];
        uint32_t depLevel = passGraph->longestDistances.elements[nodeIdx];
        uint32_t depIdx = passGraph->dependencyLevels.deplevels.elements[depLevel].depCount++;
        passGraph->dependencyLevels.deplevels.elements[depLevel].deps[depIdx] = nodeIdx;
    }
}

LOCAL void rx__finalizeDependencyLevels(rx_FrameGraph* passGraph) {
    uint64_t globalExecutionIndex = 0;

    //mem_arrSetZero(passGraph->resMultipleQeuesReadsBitFields.elements, passGraph->passCount * 8);
    
    mem_arrSetZero(passGraph->tempPerQueuePreviousNodes.elements, passGraph->tempPerQueuePreviousNodes.capacity);
    u32* perQueuePreviousNodes = passGraph->tempPerQueuePreviousNodes.elements;
    
    mem_arrSetZero(passGraph->queues.elements, passGraph->queues.capacity);
    // Dispatch nodes to corresponding dependency levels.
    uint32_t detectedQueueCount = 1;
    for (uint32_t nodeIdx = 0; nodeIdx < passGraph->passCount; nodeIdx++) {
        uint8_t execQueue = passGraph->executionQueues.elements[nodeIdx]; // +1
        //detectedQueueCount = RX__maxVal(detectedQueueCount, execQueue);
        passGraph->queues.elements[execQueue] += 1;
    }
    
    for (uint32_t depLevelIdx = 0; depLevelIdx < passGraph->dependencyLevels.deplevels.count; depLevelIdx++) {
        uint32_t globalExecutionIndexStartPerDep = globalExecutionIndex;
        uint32_t localExecutionIndex = 0;
        rx__QueueWork* queuesWorkPerDepLevel = passGraph->dependencyLevels.queuesPerDepLevel.queueWork.elements + (passGraph->maxQueues * depLevelIdx);
        queuesWorkPerDepLevel->graphNodes =  NULL;
        queuesWorkPerDepLevel->count = 0;
        //passGraph->tempResQueueMap.count = 0;

        rx__DependencyLevel* depLevel = passGraph->dependencyLevels.deplevels.elements + depLevelIdx;
        uint32_t count = depLevel->depCount;
        uint32_t* depsOnLevel = depLevel->deps;
        
        for (uint32_t idx = 0; idx < count; idx++) {
            uint32_t dep = depsOnLevel[idx];
            uint32_t slotCount = passGraph->maxSlotsPerNode;
            uint32_t nodeIdx = dep;
            const uint8_t executionQueue = passGraph->executionQueues.elements[nodeIdx];

            rx__GraphNode* node = passGraph->nodes.elements + dep;
            #if 0
            rx__Res* nodeDepthIndicies = passGraph->slotDeps.elements + (slotCount * dep);

            uint8_t* slotDeph = passGraph->dependencySlotFlags.elements + (idx * passGraph->maxSlotsPerNode);

            for (uint32_t slotIdx = 0; slotIdx < slotCount; slotIdx++) {
                rx__Res* res = &nodeDepthIndicies[slotIdx];
                if (res->dep.pass.idx == 0) {
                    continue;
                }
                rx_SlotFlags flags = slotDeph[slotIdx];
                if ((flags & rx_slotFlag_read) == rx_slotFlag_read) {
                    bool found = false;
                    for (uint32_t idx = 0; idx < passGraph->tempResQueueMap.count; idx++) {
                        if (passGraph->tempResQueueMap.elements[idx].res.dep.res.val == res->dep.res.val) {
                            
                            passGraph->tempResQueueMap.elements[idx].passFlags |= 1 << executionQueue;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        ASSERT((passGraph->tempResQueueMap.count + 1) < passGraph->tempResQueueMap.capacity);
                        passGraph->tempResQueueMap.elements[passGraph->tempResQueueMap.count].res = *res;
                        passGraph->tempResQueueMap.elements[passGraph->tempResQueueMap.count].passFlags = 1 << executionQueue;
                        passGraph->tempResQueueMap.count += 1;
                    }
                }
                if ((flags & rx_slotFlag_write) == rx_slotFlag_write) {
                    // TODO: Check if duplicates exist or do that earlier in a validation check...
                    rx__ResNodePair* resNodePair = passGraph->writtenSlotResToPassMap.elements + (passGraph->writtenSlotResToPassMap.count++);
                    resNodePair->nodeIdx = dep;
                    resNodePair->res = *res;
                }
            }
            #endif
            
            node->globalExecutionIndex = globalExecutionIndex;
            node->localToDependencyLevelExecutionIndex = localExecutionIndex;
            passGraph->queues.elements[node->executionQueueIndex] += 1;
            //node->localToQueueExecutionIndex = passGraph->queueNodeCounters[node->executionQueueIndex]++;
            
            passGraph->nodesInGlobalExecutionOrder.elements[globalExecutionIndex] = (u16) nodeIdx;

            //passGraph->dependencyLevel.mNodesPerQueue[node->ExecutionQueueIndex].push_back(node);
            //passGraph->mNodesPerQueue[node->ExecutionQueueIndex].push_back(node);
            
            // Add previous node on that queue as a dependency for sync optimization later
            // NOTE: Do this at a later state maybe...
            if (perQueuePreviousNodes[node->executionQueueIndex]) {
                passGraph->nodesToSyncWith.elements[nodeIdx] |= 1 << (perQueuePreviousNodes[node->executionQueueIndex] - 1);
            }
            perQueuePreviousNodes[node->executionQueueIndex] = nodeIdx + 1;
            
            localExecutionIndex++;
            globalExecutionIndex++;
        }
        #if 0
        // Record queue indices that are detected to read common resources
        for (uint32_t idx = 0;  idx < passGraph->tempResQueueMap.count; idx++) {
            rx__ResQueueCovery* resQueueCovery = passGraph->tempResQueueMap.elements + idx;
            uint32_t queuesMask = resQueueCovery->passFlags;
            
            for (uint32_t queueCount = 0, queueIdx = 0; queueIdx < passGraph->maxQueues; queueIdx++) {
                uint32_t queueMask = 1 << queueIdx;
                if ((queuesMask & queueMask) == queueMask) {
                    if (queueCount > 0) {
                        depLevel->crossQueResReadsInvolvementFlags |= queuesMask;
                        uint32_t nodeIdx = rx__graphGetIdx(resQueueCovery->res.dep.pass.idx);
                        uint32_t slot = resQueueCovery->res.dep.pass.slot;
                        passGraph->resMultipleQeuesReadsBitFields.elements[nodeIdx * 8 + slot] |= 1 << depLevelIdx;
                        //dependencyLevel.mQueuesInvoledInCrossQueueResourceReads.insert(queueIndex);
                        // TODO: Look into if we may need this later of if we can easily gather the info elsewhere
                        //dependencyLevel.mSubresourcesReadByMultipleQueues.insert(subresourceName);
                        break;
                    }
                    queueCount++;
                }
            }
        }
        #endif
        
        
        
        // for each node in nodesInGlobalExecutionOrder starting at globalExecutionIndexStartPerDep
        // add it to the current depth level queue

        // passGraph->dependencyLevel.mNodesPerQueue[node->ExecutionQueueIndex].push_back(node);
        // passGraph->mNodesPerQueue[node->ExecutionQueueIndex].push_back(node);
    }
}

LOCAL void rx__cullRedundantSynchronizations(rx_FrameGraph* passGraph) {

    // reset synchronizationIndexSet flags
    mem_arrSetZero(passGraph->synchronizationIndexSet.elements, passGraph->passCount * passGraph->queues.count);
    uint32_t* synchronizationIndexSets = passGraph->synchronizationIndexSet.elements;
    
    for (uint32_t depLevelIdx = 0; depLevelIdx < passGraph->dependencyLevels.deplevels.count; depLevelIdx++) {
        rx__DependencyLevel* depLevel = passGraph->dependencyLevels.deplevels.elements + depLevelIdx;
        for (uint32_t idx = 0; idx < depLevel->depCount; idx++) {
            uint32_t nodeIdx = depLevel->deps[idx];
            rx__GraphNode* node = passGraph->nodes.elements + nodeIdx;
            mem_arrSetZero(passGraph->tempPerQueuePreviousNodes.elements, passGraph->tempPerQueuePreviousNodes.capacity);
            uint32_t* closestNodesToSyncWith = passGraph->tempPerQueuePreviousNodes.elements;
            uint64_t syncBitFlags = passGraph->nodesToSyncWith.elements[nodeIdx];
            if (syncBitFlags != 0) {
                rx_forEachBitflag64(syncBitFlags) {
                    rx__GraphNode* dependencyNode = passGraph->nodes.elements + bitIdx;
                    uint32_t closestNodeIdx = closestNodesToSyncWith[dependencyNode->executionQueueIndex];
                    if (closestNodeIdx == 0 || dependencyNode->localToQueueExecutionIndex > passGraph->nodes.elements[closestNodeIdx - 1].localToQueueExecutionIndex) {
                        closestNodesToSyncWith[dependencyNode->executionQueueIndex] = bitIdx + 1;
                    }
                }

                // Get rid of nodes to sync that may have had redundancies
                passGraph->nodesToSyncWith.elements[nodeIdx] = 0;
                
                // Compute initial SSIS
                for (uint32_t queueIdx = 0; queueIdx < passGraph->queues.count; ++queueIdx) {
                    uint32_t closestNodePerQueueIdx = closestNodesToSyncWith[queueIdx];
                    if (closestNodePerQueueIdx == 0) {
                        // If we do not have a closest node to sync with on another queue (queueIdx),
                        // we need to use SSIS value for that queue from the previous node on this node's queue (closestNodesToSyncWith[node->ExecutionQueueIndex])
                        // to correctly propagate SSIS values for all queues through the graph and do not lose them
                        uint32_t previousNodeIdxOnNodesQueue = closestNodesToSyncWith[node->executionQueueIndex];
                        
                        // Previous node can be null if we're dealing with first node in the queue
                        if (previousNodeIdxOnNodesQueue != 0) {
                            uint32_t syncIndexForOtherQueueFromPreviousNode = synchronizationIndexSets[previousNodeIdxOnNodesQueue * passGraph->queues.count + queueIdx];
                            // TODO: Maybe we should do +1 on the index so we do not need to deal with 0
                            synchronizationIndexSets[nodeIdx * passGraph->queues.count + queueIdx] = syncIndexForOtherQueueFromPreviousNode + 1;
                            //uint64_t syncIndexForOtherQueueFromPreviousNode = previousNodeOnNodesQueue->mSynchronizationIndexSet[queueIdx];
                            //node->mSynchronizationIndexSet[queueIdx] = syncIndexForOtherQueueFromPreviousNode;
                        }
                    } else {
                        rx__GraphNode* closestNode = passGraph->nodes.elements + (closestNodePerQueueIdx - 1);
                        // Update SSIS using closest nodes' indices
                        if (closestNode->executionQueueIndex != node->executionQueueIndex) {
                            synchronizationIndexSets[nodeIdx * passGraph->queues.count + closestNode->executionQueueIndex] = closestNode->localToQueueExecutionIndex;
                        }
                        // Store only closest nodes to sync with
                        passGraph->nodesToSyncWith.elements[nodeIdx] |= (1 << (closestNodePerQueueIdx - 1));
                    }
                }
                
                // Use node's execution index as synchronization index on its own queue
                synchronizationIndexSets[nodeIdx * passGraph->queues.count + node->executionQueueIndex] = node->localToQueueExecutionIndex + 1;
            }
        }
        
        // Second pass: cull redundant dependencies by searching for indirect synchronizations
        for (uint32_t idx = 0; idx < depLevel->depCount; idx++) {
            uint32_t nodeIdx = depLevel->deps[idx];
            rx__GraphNode* node = passGraph->nodes.elements + nodeIdx;
            uint64_t nodesToSyncWith = passGraph->nodesToSyncWith.elements[nodeIdx];
            
            // Keep track of queues we still need to sync with
            uint64_t queueToSyncWithBitField = 0;

            // Store nodes and queue syncs they cover
            //std::vector<SyncCoverage> syncCoverageArray;
            passGraph->tempSyncCoverage.count = 0;

            // Final optimized list of nodes without redundant dependencies
            uint64_t optimalNodesToSyncWith = 0;
            
            
            rx_forEachBitflag64(nodesToSyncWith) {
                uint32_t nodeIdxToSyncWith = bitIdx;
                queueToSyncWithBitField |= 1 << passGraph->nodes.elements[nodeIdxToSyncWith].executionQueueIndex;
            }
            
            /*for (const Node* nodeToSyncWith : node->mNodesToSyncWith)
            {
                queueToSyncWithIndices.insert(nodeToSyncWith->ExecutionQueueIndex);
            }*/
            
            uint64_t alreadySyncedWithNodesBitField = 0;
            
            while (queueToSyncWithBitField) {
                uint64_t maxNumberOfSyncsCoveredBySingleNode = 0;
                rx_forEachBitflag64(nodesToSyncWith) {
                    uint32_t dependencyNodeIdx = bitIdx;
                    rx__GraphNode* dependencyNode = passGraph->nodes.elements + dependencyNodeIdx;
                    uint32_t syncedQueues = 0;
                    
                    // Take a dependency node and check how many queues we would sync with
                    // if we would only sync with this one node. We very well may encounter a case
                    // where by synchronizing with just one node we will sync with more then one queue
                    // or even all of them through indirect synchronizations,
                    // which will make other synchronizations previously detected for this node redundant.
                    uint64_t syncedQueueBitField = 0;
                    rx_forEachBitflag64(queueToSyncWithBitField) {
                        uint32_t queueIndex = bitIdx;
                        uint64_t currentNodeDesiredSyncIndex = synchronizationIndexSets[nodeIdx * passGraph->queues.count + queueIndex];
                        uint64_t dependencyNodeSyncIndex = synchronizationIndexSets[dependencyNodeIdx * passGraph->queues.count + queueIndex];

                        ASSERT(currentNodeDesiredSyncIndex != 0 && "Bug! Node that wants to sync with some queue must have a valid sync index for that queue.");

                        if (queueIndex == node->executionQueueIndex) {
                            // so we are on the same queue so we point to the previous node
                            currentNodeDesiredSyncIndex -= 1;
                        }

                        if (dependencyNodeSyncIndex != 0 && dependencyNodeSyncIndex >= currentNodeDesiredSyncIndex) {
                            uint64_t syncedQueueBitFieldBefore = syncedQueueBitField;
                            syncedQueueBitField |= 1 << queueIndex;
                            if (syncedQueueBitField != syncedQueueBitFieldBefore) {
                                syncedQueues += 1;
                            }
                        }
                    }
                    alreadySyncedWithNodesBitField |= 1 << dependencyNodeIdx;
                    rx__SyncCoverage syncCoverage;
                    syncCoverage.nodeIdx = dependencyNodeIdx;
                    syncCoverage.syncedQueueIdxBitfield = syncedQueueBitField;
                    syncCoverage.syncedQueueCount = syncedQueues;
                    passGraph->tempSyncCoverage.elements[passGraph->tempSyncCoverage.count++] = syncCoverage;
                    maxNumberOfSyncsCoveredBySingleNode = maxVal(maxNumberOfSyncsCoveredBySingleNode, syncedQueues);
                    
                }
                for (uint32_t idx = 0; idx < passGraph->tempSyncCoverage.count; idx++) {
                    rx__SyncCoverage* syncCoverage = passGraph->tempSyncCoverage.elements + idx;
                    
                    if (syncCoverage->syncedQueueCount >= maxNumberOfSyncsCoveredBySingleNode) {
                        // Optimal list of synchronizations should not contain nodes from the same queue,
                        // because work on the same queue is synchronized automatically and implicitly
                        uint32_t syncCovExecutionQueueIndex = passGraph->nodes.elements[syncCoverage->nodeIdx].executionQueueIndex;
                        if (passGraph->nodes.elements[syncCoverage->nodeIdx].executionQueueIndex != node->executionQueueIndex) {
                            optimalNodesToSyncWith |= 1 << syncCoverage->nodeIdx;
                            
                            // Update SSIS
                            uint32_t* idx = synchronizationIndexSets + (syncCoverage->nodeIdx * passGraph->queues.count + syncCovExecutionQueueIndex);
                            uint32_t nodeSyncIdx = synchronizationIndexSets[syncCoverage->nodeIdx * passGraph->queues.count + syncCovExecutionQueueIndex];
                            *idx = maxVal(*idx, nodeSyncIdx);
                        }
                        
                        // Remove covered queues from the list of queues we need to sync with
                        queueToSyncWithBitField &= ~nodesToSyncWith;
                        /*
                        for (uint64_t syncedQueueIndex : syncCoverage.SyncedQueueIndices) {
                            queueToSyncWithIndices.erase(syncedQueueIndex);
                        }
                        */
                        
                    }
                    
                }
                // Remove nodes that we synced with from the original list. Reverse iterating to avoid index invalidation.
                passGraph->nodesToSyncWith.elements[nodeIdx] &= ~alreadySyncedWithNodesBitField;
                /*for (auto syncCoverageIt = syncCoverageArray.rbegin(); syncCoverageIt != syncCoverageArray.rend(); ++syncCoverageIt)
                {
                    node->mNodesToSyncWith.erase(node->mNodesToSyncWith.begin() + syncCoverageIt->NodeToSyncWithIndex);
                }*/
            }
        }
    }
}

LOCAL void rx__passGraphBuild(rx_FrameGraph* passGraph, u32 passCount) {
    ASSERT(passGraph);
    mem_arrSetZero(passGraph->nodesToSyncWith.elements, passGraph->passCount);
    mem_arrSetZero(passGraph->nodes.elements, passGraph->passCount);

    passGraph->passCount = passCount;
    
    passGraph->topologicallySortedNodes.count = 0;

    //passGraph->visited = passGraph->passCount;

    rx__buildAdjacencyLists(passGraph);
    rx__topologicalSort(passGraph);
    rx__buildDependencyLevels(passGraph);
    rx__finalizeDependencyLevels(passGraph);
    rx__cullRedundantSynchronizations(passGraph);
}

// Shared code that are called by variouse backend aswell

LOCAL void rx__setError(rx_Ctx* ctx, rx_error error) {
    ASSERT(ctx);
    ctx->error = error;
}

LOCAL void rx__log(rx_Ctx* ctx, Str8 msg) {
    ASSERT(ctx);
    os_log(msg);
}

LOCAL rx_texture rx__allocTexture(rx_Ctx* ctx) {
    u32 idx = rx__queuePull(&ctx->textures.freeIndicies);
    ASSERT(idx != RX__INVALID_INDEX && "No free textures available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_texture) {0};
    }

    rx_Texture* texture = &ctx->textures.elements[idx];
    texture->gen = maxVal(1, (texture->gen + 1) % u16_max);

    rx_texture handle = {0};
    handle.idx = idx;
    handle.gen = texture->gen;

    return handle;
}

LOCAL rx_resGroupLayout rx__allocResGroupLayout(rx_Ctx* ctx) {
    u32 idx = rx__queuePull(&ctx->resGroupLayouts.freeIndicies);
    ASSERT(idx != RX__INVALID_INDEX && "No free ResGroups available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_resGroupLayout) {0};
    }

    rx_ResGroupLayout* resGroupLayout = &ctx->resGroupLayouts.elements[idx];
    u32 gen = maxVal(1, (resGroupLayout->gen + 1) % u16_max);

    mem_structSetZero(resGroupLayout);

    resGroupLayout->gen = gen;

    rx_resGroupLayout handle = {0};
    handle.idx = idx;
    handle.gen = resGroupLayout->gen;

    return handle;
}

LOCAL rx_resGroup rx__allocResGroup(rx_Ctx* ctx) {
    u32 idx = rx__queuePull(&ctx->resGroups.freeIndicies);
    ASSERT(idx != RX__INVALID_INDEX && "No free ResGroups available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_resGroup) {0};
    }

    rx_ResGroup* resGroup = &ctx->resGroups.elements[idx];
    u32 gen = maxVal(1, (resGroup->gen + 1) % u16_max);

    mem_structSetZero(resGroup);


    resGroup->gen = gen;

    rx_resGroup handle = {0};
    handle.idx = idx;
    handle.gen = resGroup->gen;

    return handle;
}



LOCAL rx_swapChain rx__allocSwapChain(rx_Ctx* ctx) {
    ASSERT(ctx);
    u32 idx = rx__queuePull(&ctx->swapChains.freeIndicies);
    ASSERT(idx != RX__INVALID_INDEX && "No free Swap Chains views available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_swapChain) {0};
    }

    rx_SwapChain* swapChain = &ctx->swapChains.elements[idx];
    swapChain->gen = maxVal(1, (swapChain->gen + 1) % u16_max);

    rx_swapChain handle = {0};
    handle.idx = idx;
    handle.gen = swapChain->gen;

    return handle;
}

LOCAL rx_Buffer* rx__getBuffer(rx_Ctx* ctx, rx_buffer bufferHandle) {
    ASSERT(ctx);
    ASSERT(bufferHandle.id != 0);
    ASSERT(bufferHandle.idx < ctx->buffers.capacity);

    rx_Buffer* buffer = &ctx->buffers.elements[bufferHandle.idx];
    ASSERT(buffer->gen == bufferHandle.gen);
    return buffer;
}

LOCAL rx_Texture* rx__getTexture(rx_Ctx* ctx, rx_texture textureHandle) {
    ASSERT(ctx);
    ASSERT(textureHandle.id != 0);
    ASSERT(textureHandle.idx < ctx->textures.capacity);

    rx_Texture* texture = &ctx->textures.elements[textureHandle.idx];
    ASSERT(texture->gen == textureHandle.gen);
    return texture;
}

LOCAL rx_RenderShader* rx__getRenderShader(rx_Ctx* ctx, rx_renderShader renderShaderHandle) {
    ASSERT(ctx);
    ASSERT(renderShaderHandle.id != 0);
    ASSERT(renderShaderHandle.idx < ctx->renderPipelines.capacity);

    rx_RenderShader* renderShader = &ctx->renderShaders.elements[renderShaderHandle.idx];
    ASSERT(renderShader->gen == renderShaderHandle.gen);
    return renderShader;
}

LOCAL rx_RenderPipeline* rx__getRenderPipeline(rx_Ctx* ctx, rx_renderPipeline renderPipelineHandle) {
    ASSERT(ctx);
    ASSERT(renderPipelineHandle.id != 0);
    ASSERT(renderPipelineHandle.idx < ctx->resGroups.capacity);

    rx_RenderPipeline* renderPipeline = &ctx->renderPipelines.elements[renderPipelineHandle.idx];
    ASSERT(renderPipeline->gen == renderPipelineHandle.gen);
    return renderPipeline;
}

LOCAL rx_SwapChain* rx__getSwapChain(rx_Ctx* ctx, rx_swapChain swapChainHandle) {
    ASSERT(ctx);
    ASSERT(swapChainHandle.id != 0);
    ASSERT(swapChainHandle.idx < ctx->resGroups.capacity);

    rx_SwapChain* swapChain = &ctx->swapChains.elements[swapChainHandle.idx];
    ASSERT(swapChain->gen == swapChainHandle.gen);
    return swapChain;
}

LOCAL rx_ResGroup* rx__getResGroup(rx_Ctx* ctx, rx_resGroup resGroupHandle) {
    ASSERT(ctx);
    ASSERT(resGroupHandle.id != 0);
    ASSERT(resGroupHandle.idx < ctx->resGroups.capacity);

    rx_ResGroup* resGroup = &ctx->resGroups.elements[resGroupHandle.idx];
    ASSERT(resGroup->gen == resGroupHandle.gen);
    return resGroup;
}

LOCAL rx_ResGroupLayout* rx__getResGroupLayout(rx_Ctx* ctx, rx_resGroupLayout resGroupLayoutHandle) {
    ASSERT(ctx);
    ASSERT(resGroupLayoutHandle.id != 0);
    ASSERT(resGroupLayoutHandle.idx < ctx->resGroups.capacity);

    rx_ResGroupLayout* resGroup = &ctx->resGroupLayouts.elements[resGroupLayoutHandle.idx];
    ASSERT(resGroup->gen == resGroupLayoutHandle.gen);
    return resGroup;
}


LOCAL uint32_t rx__vertexFormatBytesize(rx_vertexFormat fmt) {
    switch (fmt) {
        case rx_vertexFormat_u8x2:      return 2;
        case rx_vertexFormat_u8x4:      return 4;
        case rx_vertexFormat_s8x2:      return 2;
        case rx_vertexFormat_s8x4:      return 4;
        case rx_vertexFormat_u8x2n:     return 2;
        case rx_vertexFormat_u8x4n:     return 4;
        case rx_vertexFormat_s8x2n:     return 2;
        case rx_vertexFormat_s8x4n:     return 4;
        case rx_vertexFormat_u16x2:     return 4;
        case rx_vertexFormat_u16x4:     return 8;
        case rx_vertexFormat_s16x2:     return 4;
        case rx_vertexFormat_s16x4:     return 8;
        case rx_vertexFormat_u16x2n:    return 4;
        case rx_vertexFormat_u16x4n:    return 8;
        case rx_vertexFormat_s16x2n:    return 4;
        case rx_vertexFormat_s16x4n:    return 8;
        case rx_vertexFormat_f16x2:     return 4;
        case rx_vertexFormat_f16x4:     return 8;
        case rx_vertexFormat_u32x1:     return 4;
        case rx_vertexFormat_u32x2:     return 8;
        case rx_vertexFormat_u32x3:     return 12;
        case rx_vertexFormat_u32x4:     return 16;
        case rx_vertexFormat_s32x1:     return 4;
        case rx_vertexFormat_s32x2:     return 8;
        case rx_vertexFormat_s32x3:     return 12;
        case rx_vertexFormat_s32x4:     return 16;
        case rx_vertexFormat_f32x1:     return 4;
        case rx_vertexFormat_f32x2:     return 8;
        case rx_vertexFormat_f32x3:     return 12;
        case rx_vertexFormat_f32x4:     return 16;
        case rx_vertexFormat__invalid:  return 0;
        default:
            RX_UNREACHABLE();
            return 0;
    }
}

#if RX_VULKAN


#define RX__VK_INSTANCE_FUNCS \
    RX__XMACRO(vkCreateInstance,                 VKAPI_PTR, (const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)) \
    RX__XMACRO(vkDestroyInstance,                VKAPI_PTR, (VkInstance instance, const VkAllocationCallbacks* pAllocator))


#define RX__XMACRO(name, ret, args) typedef ret (VK_APIENTRY* PFN_ ## name) args;
RX__VK_INSTANCE_FUNCS
#undef RX__XMACRO



typedef struct VkInstanceApi {
#define RX__XMACRO(name, ret, args) PFN_ ## name name;
RX__VK_INSTANCE_FUNCS
#undef RX__XMACRO
} VkInstanceApi;

typedef struct VkDeviceApi {
    int toDo;
} VkDeviceApi;

typedef struct rx_VulkanCtx {
    rx_Ctx base;
    VkInstance instance;
    struct {
        VkResult (VKAPI_PTR* vkCreateInstance)(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
        void     (VKAPI_PTR* vkDestroyInstance)(VkInstance instance, const VkAllocationCallbacks* pAllocator);
        VkResult (VKAPI_PTR* vkEnumerateInstanceLayerProperties)(u32* pPropertyCount, VkLayerProperties* pProperties);
        VkResult (VKAPI_PTR* vkEnumerateInstanceExtensionProperties)(const char* pLayerName, u32* pPropertyCount, VkExtensionProperties* pProperties);

        VkResult (VKAPI_PTR* vkCreateDevice)(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
        void     (VKAPI_PTR* vkDestroyDevice)(VkDevice device, const VkAllocationCallbacks* pAllocator);
    } instanceApi;
    VkInstanceApi instanceApi;
    VkDeviceApi deviceApi;
} rx_VulkanCtx;

#endif

#if RX_OGL
#define rx__oglCheckErrors() { ASSERT(glGetError() == GL_NO_ERROR); }

// optional GL loader for win32
#if defined(RX_USE_WIN32_GL_LOADER)

#define __gl_h_ 1
#define __gl32_h_ 1
#define __gl31_h_ 1
#define __GL_H__ 1
#define __glext_h_ 1
#define __GLEXT_H_ 1
#define __gltypes_h_ 1
#define __glcorearb_h_ 1
#define __gl_glcorearb_h_ 1
#define GL_APIENTRY APIENTRY

typedef unsigned int  GLenum;
typedef unsigned int  GLuint;
typedef int  GLsizei;
typedef char  GLchar;
typedef ptrdiff_t  GLintptr;
typedef ptrdiff_t  GLsizeiptr;
typedef double  GLclampd;
typedef unsigned short  GLushort;
typedef unsigned char  GLubyte;
typedef unsigned char  GLboolean;
typedef uint64_t  GLuint64;
typedef double  GLdouble;
typedef unsigned short  GLhalf;
typedef float  GLclampf;
typedef unsigned int  GLbitfield;
typedef signed char  GLbyte;
typedef short  GLshort;
typedef void  GLvoid;
typedef int64_t  GLint64;
typedef float  GLfloat;
typedef int  GLint;
#define GL_INT_2_10_10_10_REV 0x8D9F
#define GL_R32F 0x822E
#define GL_PROGRAM_POINT_SIZE 0x8642
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_R16F 0x822D
#define GL_COLOR_ATTACHMENT22 0x8CF6
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_NUM_EXTENSIONS 0x821D
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_VERTEX_SHADER 0x8B31
#define GL_INCR 0x1E02
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_STATIC_DRAW 0x88E4
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_FUNC_SUBTRACT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_CONSTANT_COLOR 0x8001
#define GL_DECR_WRAP 0x8508
#define GL_R8 0x8229
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_SHORT 0x1402
#define GL_DEPTH_TEST 0x0B71
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_LINK_STATUS 0x8B82
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_RGBA16F 0x881A
#define GL_CONSTANT_ALPHA 0x8003
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_MIN_LOD 0x813A
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_STREAM_DRAW 0x88E0
#define GL_ONE 1
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_RGB10_A2 0x8059
#define GL_RGBA8 0x8058
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_RGBA4 0x8056
#define GL_RGB8 0x8051
#define GL_ARRAY_BUFFER 0x8892
#define GL_STENCIL 0x1802
#define GL_TEXTURE_2D 0x0DE1
#define GL_DEPTH 0x1801
#define GL_FRONT 0x0404
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_REPEAT 0x2901
#define GL_RGBA 0x1908
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_DECR 0x1E03
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_FLOAT 0x1406
#define GL_TEXTURE_MAX_LOD 0x813B
#define GL_DEPTH_COMPONENT 0x1902
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_COLOR 0x1800
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_TRIANGLES 0x0004
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_NONE 0
#define GL_SRC_COLOR 0x0300
#define GL_BYTE 0x1400
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_LINE_STRIP 0x0003
#define GL_TEXTURE_3D 0x806F
#define GL_CW 0x0900
#define GL_LINEAR 0x2601
#define GL_RENDERBUFFER 0x8D41
#define GL_GEQUAL 0x0206
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_RGBA32F 0x8814
#define GL_BLEND 0x0BE2
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_EXTENSIONS 0x1F03
#define GL_NO_ERROR 0
#define GL_REPLACE 0x1E01
#define GL_KEEP 0x1E00
#define GL_CCW 0x0901
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_RGB 0x1907
#define GL_TRIANGLE_STRIP 0x0005
#define GL_FALSE 0
#define GL_ZERO 0
#define GL_CULL_FACE 0x0B44
#define GL_INVERT 0x150A
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_UNSIGNED_SHORT 0x1403
#define GL_NEAREST 0x2600
#define GL_SCISSOR_TEST 0x0C11
#define GL_LEQUAL 0x0203
#define GL_STENCIL_TEST 0x0B90
#define GL_DITHER 0x0BD0
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_EQUAL 0x0202
#define GL_FRAMEBUFFER 0x8D40
#define GL_RGB5 0x8050
#define GL_LINES 0x0001
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_SRC_ALPHA 0x0302
#define GL_INCR_WRAP 0x8507
#define GL_LESS 0x0201
#define GL_MULTISAMPLE 0x809D
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_BACK 0x0405
#define GL_ALWAYS 0x0207
#define GL_FUNC_ADD 0x8006
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_NOTEQUAL 0x0205
#define GL_DST_COLOR 0x0306
#define GL_COMPILE_STATUS 0x8B81
#define GL_RED 0x1903
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_DST_ALPHA 0x0304
#define GL_RGB5_A1 0x8057
#define GL_GREATER 0x0204
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_TRUE 1
#define GL_NEVER 0x0200
#define GL_POINTS 0x0000
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_MIRRORED_REPEAT 0x8370
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_R11F_G11F_B10F 0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define GL_RGB9_E5 0x8C3D
#define GL_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#define GL_RGBA32UI 0x8D70
#define GL_RGB32UI 0x8D71
#define GL_RGBA16UI 0x8D76
#define GL_RGB16UI 0x8D77
#define GL_RGBA8UI 0x8D7C
#define GL_RGB8UI 0x8D7D
#define GL_RGBA32I 0x8D82
#define GL_RGB32I 0x8D83
#define GL_RGBA16I 0x8D88
#define GL_RGB16I 0x8D89
#define GL_RGBA8I 0x8D8E
#define GL_RGB8I 0x8D8F
#define GL_RED_INTEGER 0x8D94
#define GL_RG 0x8227
#define GL_RG_INTEGER 0x8228
#define GL_R8 0x8229
#define GL_R16 0x822A
#define GL_RG8 0x822B
#define GL_RG16 0x822C
#define GL_R16F 0x822D
#define GL_R32F 0x822E
#define GL_RG16F 0x822F
#define GL_RG32F 0x8230
#define GL_R8I 0x8231
#define GL_R8UI 0x8232
#define GL_R16I 0x8233
#define GL_R16UI 0x8234
#define GL_R32I 0x8235
#define GL_R32UI 0x8236
#define GL_RG8I 0x8237
#define GL_RG8UI 0x8238
#define GL_RG16I 0x8239
#define GL_RG16UI 0x823A
#define GL_RG32I 0x823B
#define GL_RG32UI 0x823C
#define GL_RGBA_INTEGER 0x8D99
#define GL_R8_SNORM 0x8F94
#define GL_RG8_SNORM 0x8F95
#define GL_RGB8_SNORM 0x8F96
#define GL_RGBA8_SNORM 0x8F97
#define GL_R16_SNORM 0x8F98
#define GL_RG16_SNORM 0x8F99
#define GL_RGB16_SNORM 0x8F9A
#define GL_RGBA16_SNORM 0x8F9B
#define GL_RGBA16 0x805B
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
#define GL_MAX_3D_TEXTURE_SIZE 0x8073
#define GL_MAX_ARRAY_TEXTURE_LAYERS 0x88FF
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_TEXTURE_BORDER_COLOR 0x1004
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_TEXTURE_COMPARE_FUNC 0x884D
#define GL_COMPARE_REF_TO_TEXTURE 0x884E
#define GL_TEXTURE_CUBE_MAP_SEAMLESS 0x884F
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_FRAMEBUFFER_UNDEFINED 0x8219
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56


// X Macro list of GL function names and signatures
#define RX__GL_FUNCS \
    RX__XMACRO(glBindVertexArray,                 void, (GLuint array)) \
    RX__XMACRO(glFramebufferTextureLayer,         void, (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)) \
    RX__XMACRO(glGenFramebuffers,                 void, (GLsizei n, GLuint * framebuffers)) \
    RX__XMACRO(glBindFramebuffer,                 void, (GLenum target, GLuint framebuffer)) \
    RX__XMACRO(glBindRenderbuffer,                void, (GLenum target, GLuint renderbuffer)) \
    RX__XMACRO(glGetStringi,                      const GLubyte *, (GLenum name, GLuint index)) \
    RX__XMACRO(glClearBufferfi,                   void, (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)) \
    RX__XMACRO(glClearBufferfv,                   void, (GLenum buffer, GLint drawbuffer, const GLfloat * value)) \
    RX__XMACRO(glClearBufferuiv,                  void, (GLenum buffer, GLint drawbuffer, const GLuint * value)) \
    RX__XMACRO(glClearBufferiv,                   void, (GLenum buffer, GLint drawbuffer, const GLint * value)) \
    RX__XMACRO(glDeleteRenderbuffers,             void, (GLsizei n, const GLuint * renderbuffers)) \
    RX__XMACRO(glUniform1fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    RX__XMACRO(glUniform2fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    RX__XMACRO(glUniform3fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    RX__XMACRO(glUniform4fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    RX__XMACRO(glUniform1iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    RX__XMACRO(glUniform2iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    RX__XMACRO(glUniform3iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    RX__XMACRO(glUniform4iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    RX__XMACRO(glUniformMatrix4fv,                void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)) \
    RX__XMACRO(glUseProgram,                      void, (GLuint program)) \
    RX__XMACRO(glShaderSource,                    void, (GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length)) \
    RX__XMACRO(glLinkProgram,                     void, (GLuint program)) \
    RX__XMACRO(glGetUniformLocation,              GLint, (GLuint program, const GLchar * name)) \
    RX__XMACRO(glGetShaderiv,                     void, (GLuint shader, GLenum pname, GLint * params)) \
    RX__XMACRO(glGetProgramInfoLog,               void, (GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog)) \
    RX__XMACRO(glGetAttribLocation,               GLint, (GLuint program, const GLchar * name)) \
    RX__XMACRO(glDisableVertexAttribArray,        void, (GLuint index)) \
    RX__XMACRO(glDeleteShader,                    void, (GLuint shader)) \
    RX__XMACRO(glDeleteProgram,                   void, (GLuint program)) \
    RX__XMACRO(glCompileShader,                   void, (GLuint shader)) \
    RX__XMACRO(glStencilFuncSeparate,             void, (GLenum face, GLenum func, GLint ref, GLuint mask)) \
    RX__XMACRO(glStencilOpSeparate,               void, (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)) \
    RX__XMACRO(glRenderbufferStorageMultisample,  void, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)) \
    RX__XMACRO(glDrawBuffers,                     void, (GLsizei n, const GLenum * bufs)) \
    RX__XMACRO(glVertexAttribDivisor,             void, (GLuint index, GLuint divisor)) \
    RX__XMACRO(glBufferSubData,                   void, (GLenum target, GLintptr offset, GLsizeiptr size, const void * data)) \
    RX__XMACRO(glGenBuffers,                      void, (GLsizei n, GLuint * buffers)) \
    RX__XMACRO(glCheckFramebufferStatus,          GLenum, (GLenum target)) \
    RX__XMACRO(glFramebufferRenderbuffer,         void, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)) \
    RX__XMACRO(glCompressedTexImage2D,            void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data)) \
    RX__XMACRO(glCompressedTexImage3D,            void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data)) \
    RX__XMACRO(glActiveTexture,                   void, (GLenum texture)) \
    RX__XMACRO(glTexSubImage3D,                   void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels)) \
    RX__XMACRO(glRenderbufferStorage,             void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)) \
    RX__XMACRO(glGenTextures,                     void, (GLsizei n, GLuint * textures)) \
    RX__XMACRO(glPolygonOffset,                   void, (GLfloat factor, GLfloat units)) \
    RX__XMACRO(glDrawElements,                    void, (GLenum mode, GLsizei count, GLenum type, const void * indices)) \
    RX__XMACRO(glDeleteFramebuffers,              void, (GLsizei n, const GLuint * framebuffers)) \
    RX__XMACRO(glBlendEquationSeparate,           void, (GLenum modeRGB, GLenum modeAlpha)) \
    RX__XMACRO(glDeleteTextures,                  void, (GLsizei n, const GLuint * textures)) \
    RX__XMACRO(glGetProgramiv,                    void, (GLuint program, GLenum pname, GLint * params)) \
    RX__XMACRO(glBindTexture,                     void, (GLenum target, GLuint texture)) \
    RX__XMACRO(glTexImage3D,                      void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels)) \
    RX__XMACRO(glCreateShader,                    GLuint, (GLenum type)) \
    RX__XMACRO(glTexSubImage2D,                   void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels)) \
    RX__XMACRO(glFramebufferTexture2D,            void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
    RX__XMACRO(glCreateProgram,                   GLuint, (void)) \
    RX__XMACRO(glViewport,                        void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    RX__XMACRO(glDeleteBuffers,                   void, (GLsizei n, const GLuint * buffers)) \
    RX__XMACRO(glDrawArrays,                      void, (GLenum mode, GLint first, GLsizei count)) \
    RX__XMACRO(glDrawElementsInstanced,           void, (GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount)) \
    RX__XMACRO(glVertexAttribPointer,             void, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer)) \
    RX__XMACRO(glUniform1i,                       void, (GLint location, GLint v0)) \
    RX__XMACRO(glDisable,                         void, (GLenum cap)) \
    RX__XMACRO(glColorMask,                       void, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    RX__XMACRO(glColorMaski,                      void, (GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    RX__XMACRO(glBindBuffer,                      void, (GLenum target, GLuint buffer)) \
    RX__XMACRO(glDeleteVertexArrays,              void, (GLsizei n, const GLuint * arrays)) \
    RX__XMACRO(glDepthMask,                       void, (GLboolean flag)) \
    RX__XMACRO(glDrawArraysInstanced,             void, (GLenum mode, GLint first, GLsizei count, GLsizei instancecount)) \
    RX__XMACRO(glScissor,                         void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    RX__XMACRO(glGenRenderbuffers,                void, (GLsizei n, GLuint * renderbuffers)) \
    RX__XMACRO(glBufferData,                      void, (GLenum target, GLsizeiptr size, const void * data, GLenum usage)) \
    RX__XMACRO(glBlendFuncSeparate,               void, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)) \
    RX__XMACRO(glTexParameteri,                   void, (GLenum target, GLenum pname, GLint param)) \
    RX__XMACRO(glGetIntegerv,                     void, (GLenum pname, GLint * data)) \
    RX__XMACRO(glEnable,                          void, (GLenum cap)) \
    RX__XMACRO(glBlitFramebuffer,                 void, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)) \
    RX__XMACRO(glStencilMask,                     void, (GLuint mask)) \
    RX__XMACRO(glAttachShader,                    void, (GLuint program, GLuint shader)) \
    RX__XMACRO(glGetError,                        GLenum, (void)) \
    RX__XMACRO(glBlendColor,                      void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
    RX__XMACRO(glTexParameterf,                   void, (GLenum target, GLenum pname, GLfloat param)) \
    RX__XMACRO(glTexParameterfv,                  void, (GLenum target, GLenum pname, const GLfloat* params)) \
    RX__XMACRO(glGetShaderInfoLog,                void, (GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog)) \
    RX__XMACRO(glDepthFunc,                       void, (GLenum func)) \
    RX__XMACRO(glStencilOp ,                      void, (GLenum fail, GLenum zfail, GLenum zpass)) \
    RX__XMACRO(glStencilFunc,                     void, (GLenum func, GLint ref, GLuint mask)) \
    RX__XMACRO(glEnableVertexAttribArray,         void, (GLuint index)) \
    RX__XMACRO(glBlendFunc,                       void, (GLenum sfactor, GLenum dfactor)) \
    RX__XMACRO(glReadBuffer,                      void, (GLenum src)) \
    RX__XMACRO(glTexImage2D,                      void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels)) \
    RX__XMACRO(glGenVertexArrays,                 void, (GLsizei n, GLuint * arrays)) \
    RX__XMACRO(glFrontFace,                       void, (GLenum mode)) \
    RX__XMACRO(glCullFace,                        void, (GLenum mode)) \
    RX__XMACRO(glPixelStorei,                     void, (GLenum pname, GLint param)) \
    RX__XMACRO(glBindSampler,                     void, (GLuint unit, GLuint sampler)) \
    RX__XMACRO(glGenSamplers,                     void, (GLsizei n, GLuint* samplers)) \
    RX__XMACRO(glSamplerParameteri,               void, (GLuint sampler, GLenum pname, GLint param)) \
    RX__XMACRO(glSamplerParameterf,               void, (GLuint sampler, GLenum pname, GLfloat param)) \
    RX__XMACRO(glSamplerParameterfv,              void, (GLuint sampler, GLenum pname, const GLfloat* params)) \
    RX__XMACRO(glDeleteSamplers,                  void, (GLsizei n, const GLuint* samplers))

// generate GL function pointer typedefs
#define RX__XMACRO(name, ret, args) typedef ret (GL_APIENTRY* PFN_ ## name) args;
RX__GL_FUNCS
#undef RX__XMACRO

// generate GL function pointers
#define RX__XMACRO(name, ret, args) static PFN_ ## name name;
RX__GL_FUNCS
#undef RX__XMACRO

#endif

#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8 0x84FA
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_COMPRESSED_RED_RGTC1
#define GL_COMPRESSED_RED_RGTC1 0x8DBB
#endif
#ifndef GL_COMPRESSED_SIGNED_RED_RGTC1
#define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#endif
#ifndef GL_COMPRESSED_RED_GREEN_RGTC2
#define GL_COMPRESSED_RED_GREEN_RGTC2 0x8DBD
#endif
#ifndef GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2
#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2 0x8DBE
#endif
#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM_ARB
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB 0x8E8C
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB 0x8E8D
#endif
#ifndef GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB 0x8E8E
#endif
#ifndef GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB 0x8E8F
#endif
#ifndef GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#endif
#ifndef GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif
#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif
#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif
#ifndef GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#endif
#ifndef GL_COMPRESSED_RG11_EAC
#define GL_COMPRESSED_RG11_EAC 0x9272
#endif
#ifndef GL_COMPRESSED_SIGNED_RG11_EAC
#define GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT 0x140B
#endif
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL 0x84F9
#endif
#ifndef GL_LUMINANCE
#define GL_LUMINANCE 0x1909
#endif

#if 0
_SOKOL_PRIVATE void _sg_gl_unload_opengl(void) {
    SOKOL_ASSERT(_sg.gl.opengl32_dll);
    FreeLibrary(_sg.gl.opengl32_dll);
    _sg.gl.opengl32_dll = 0;
}
#endif

#if OS_OSX
#define RXGLLayer NSOpenGLLayer
#else
#define RXGLLayer CAEAGLLayer
#endif

typedef struct rx_OpenGlCtx {
    rx_Ctx base;
#if defined(RX_USE_WIN32_GL_LOADER)
    os_Dl* openGl32Dll;
#endif
#if OS_APPLE
    RXGLLayer* nsGlLayer;
#endif
    GLuint vao;
    GLuint defaultFramebuffer;
    bx extAnisotropic;
    u32 maxAnisotropy;
    struct {
        GLuint vertexBuffer;
        GLuint indexBuffer;
        GLuint uniformBuffer;
        GLuint storedVertexBuffer;
        GLuint storedIndexBuffer;
        GLuint storedUniformBuffer;
    } cache;

    struct {
        struct {
            rx_buffer buffer;
            u32 currentOffset;
            u32 capacity;
        } streaming;
        struct {
            rx_buffer buffer;
            oa_allocator_t* allocator;
            u32 capacity;
        } dynamic;
    } uniform;
} rx_OpenGlCtx;

#if defined(RX_USE_WIN32_GL_LOADER)
// helper function to lookup GL functions in GL DLL
typedef PROC (WINAPI * rx__wglGetProcAddress)(LPCSTR);
LOCAL void* rx__glGetProcAddr(os_Dl* openGl32Dll, const char* name, rx__wglGetProcAddress wgl_getprocaddress) {
    void* proc_addr = (void*) wgl_getprocaddress(name);
    if (0 == proc_addr) {
        proc_addr = (void*) GetProcAddress(openGl32Dll, name);
    }
    SOKOL_ASSERT(proc_addr);
    return proc_addr;
}

// populate GL function pointers
LOCAL void rx__loadOpenGl(rx_OpenGlCtx* ctx) {
    ASSERT(ctx->openGl32Dll == 0);
    ctx->openGl32Dll = LoadLibraryA("opengl32.dll");
    ASSERT(ctx->openGl32Dll != NULL);
    rx__wglGetProcAddress wgl_getprocaddress = (rx__wglGetProcAddress) GetProcAddress(ctx->openGl32Dll, "wglGetProcAddress");
    ASSERT(wgl_getprocaddress);
    #define RX__XMACRO(name, ret, args) name = (PFN_ ## name) rx__glGetProcAddr(ctx->openGl32Dll, #name, wgl_getprocaddress);
    RX__GL_FUNCS
    #undef RX__XMACRO
}
#endif // RX_USE_WIN32_GL_LOADER

LOCAL void rx__glInitLimits(rx_OpenGlCtx* ctx) {
    rx_Limits* limits = &ctx->base.limits;
    rx__oglCheckErrors();
    GLint glInt;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glInt);
    rx__oglCheckErrors();
    limits->maxImageSize2d = glInt;
    limits->maxImageSizeArray = glInt;
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &glInt);
    rx__oglCheckErrors();
    limits->maxImageSizeCube = glInt;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &glInt);
    rx__oglCheckErrors();
    if (glInt > RX_MAX_VERTEX_ATTRIBUTES) {
        glInt = RX_MAX_VERTEX_ATTRIBUTES;
    }
    limits->maxVertexAttributes = glInt;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &glInt);
    rx__oglCheckErrors();
    limits->glMaxVertexUniformVectors = glInt;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &glInt);
    rx__oglCheckErrors();
    limits->maxImageSize3d = glInt;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &glInt);
    rx__oglCheckErrors();
    limits->maxImageArrayLayers = glInt;
    if (ctx->extAnisotropic) {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glInt);
        rx__oglCheckErrors();
        ctx->maxAnisotropy = glInt;
    } else {
        ctx->maxAnisotropy = 1;
    }
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &glInt);
    rx__oglCheckErrors();
    limits->glMaxCombinedTextureImageUnits = glInt;

    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &glInt);
    rx__oglCheckErrors();
    limits->uniformBufferAlignment = glInt;
}


#if defined(RX_GLCORE33)
LOCAL void rx__glInitCapsGlCore33(rx_OpenGlCtx* ctx) {
    rx_Features* features = &ctx->base.features;
    //rx_Limits* limits = &ctx_limits;
    //_sg.backend = SG_BACKEND_GLCORE33;

    features->originTopLeft = false;
    features->imageClampToBorder = true;
    features->mrtIndependentBlendState = false;
    features->mrtIndependentWriteMask = true;

    // scan extensions
    bool has_s3tc = false;  // BC1..BC3
    bool has_rgtc = false;  // BC4 and BC5
    bool has_bptc = false;  // BC6H and BC7
    bool has_pvrtc = false;
    bool has_etc2 = false;
    GLint numExt = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
    for (int i = 0; i < numExt; i++) {
        const char* ext = (const char*) glGetStringi(GL_EXTENSIONS, (GLuint)i);
        Str8 extensionName = str_fromNullTerminatedCharPtr((char*)ext);
        if (extensionName.size > 0) {
            if (str_isEqual(extensionName, s8("_texture_compression_s3tc"))) {
                has_s3tc = true;
            } else if (str_isEqual(extensionName, s8("_texture_compression_rgtc"))) {
                has_rgtc = true;
            } else if (str_isEqual(extensionName, s8("_texture_compression_bptc"))) {
                has_bptc = true;
            } else if (str_isEqual(extensionName, s8("_texture_compression_pvrtc"))) {
                has_pvrtc = true;
            } else if (str_isEqual(extensionName, s8("_ES3_compatibility"))) {
                has_etc2 = true;
            } else if (str_isEqual(extensionName, s8("_texture_filter_anisotropic"))) {
                ctx->extAnisotropic = true;
            }
        }
    }

    // limits
    rx__glInitLimits(ctx);

#if 0
    // pixel formats
    const bool hasBgra = false;    // not a bug
    const bool hasColorbufferFloat = true;
    const bool hasColorbufferHalfFloat = true;
    const bool hasTextureFloatLinear = true; // FIXME???
    const bool hasFloatBlend = true;
    _sg_gl_init_pixelformats(hasBgra);
    _sg_gl_init_pixelformats_float(hasColorbufferFloat, hasTextureFloatLinear, hasFloatBlend);
    _sg_gl_init_pixelformats_half_float(hasColorbufferHalfFloat);

    if (has_s3tc) {
        _sg_gl_init_pixelformats_s3tc();
    }
    if (has_rgtc) {
        _sg_gl_init_pixelformats_rgtc();
    }
    if (has_bptc) {
        _sg_gl_init_pixelformats_bptc();
    }
    if (has_pvrtc) {
        _sg_gl_init_pixelformats_pvrtc();
    }
    if (has_etc2) {
        _sg_gl_init_pixelformats_etc2();
    }
#endif
}
#elif defined(SOKOL_GLES3)
LOCAL void rx__glInitCapsGlES3(rx_OpenGlCtx* ctx) {
    ASSERT(!"Implement me!");
}
#endif

LOCAL void rx__setupBackend(rx_Ctx* baseCtx, const rx_SetupDesc* desc) {
    ASSERT(baseCtx);
    ASSERT(desc);
    //ASSERT(desc->context.gl.default_framebuffer_cb == 0 || desc->context.gl.default_framebuffer_userdata_cb == 0);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;

    // assumes that _sg.gl is already zero-initialized
    //_sg.gl.valid = true;

    #if defined(RX_USE_WIN32_GL_LOADER)
    rx__glLoadOpenGl();
    #endif

    // clear initial GL error state
    #if defined(RX_DEBUG)
        while (glGetError() != GL_NO_ERROR);
    #endif
    #if defined(RX_GLCORE33)
        rx__glInitCapsGlCore33(ctx);
    #elif defined(RX_GLES3)
        rx__glInitCapsGlES3(ctx);
    #endif

    ASSERT(desc->streamingUniformSize > 0);
    ctx->uniform.streaming.currentOffset = 0;
    ctx->uniform.streaming.capacity = desc->streamingUniformSize;
    ctx->uniform.streaming.buffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_uniform | rx_bufferUsage_stream,
        .size = desc->streamingUniformSize
    });

    ASSERT(desc->dynamicUniformSize > 0);
    ctx->uniform.streaming.capacity = desc->dynamicUniformSize;
    ctx->uniform.dynamic.buffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_uniform | rx_bufferUsage_dynamic,
        .size = desc->dynamicUniformSize
    });
    struct {
        struct {
            rx_buffer buffer;
            u32 currentOffset;
        } streaming;
        struct {
            rx_buffer buffer;
            oa_allocator_t* allocator;
        } dynamic;
    } uniform;
}

LOCAL rx_Ctx* rx__create(Arena* arena, rx_SetupDesc* desc) {
    ASSERT(arena);
    ASSERT(desc);
    rx_OpenGlCtx* ctx = mem_arenaPushStructZero(arena, rx_OpenGlCtx);
    ctx->base.api = rx_api_ogl;
#if OS_WINDOWS
    os_Dl* openGlLib = os_dlOpen(s8(""))
#endif


#if OS_APPLE
    ctx->nsGlLayer = (RXGLLayer*) desc->context.gl.appleCaOpenGlLayer;
    ASSERT(ctx->nsGlLayer != 0);
#endif
    // init context
#if 1
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&ctx->defaultFramebuffer);
    rx__oglCheckErrors()
    //ASSERT(ctx->defaultFramebuffer);
    glGenVertexArrays(1, &ctx->vao);
    rx__oglCheckErrors()
    ASSERT(ctx->vao);
    glBindVertexArray(ctx->vao);
#endif
    rx__oglCheckErrors()
    // incoming texture data is generally expected to be packed tightly
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    rx__oglCheckErrors()

    #if defined(RX_GLCORE33)
        // enable seamless cubemap sampling (only desktop GL)
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    #endif

    return (rx_Ctx*) ctx;
}

LOCAL void rx__glDiscardContext(rx_Ctx* baseCtx) {
    ASSERT(baseCtx);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    if (ctx->vao) {
        glDeleteVertexArrays(1, &ctx->vao);
    }
    rx__oglCheckErrors();
}

LOCAL void rx__activateContext(rx_Ctx* baseCtx) {
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;

    rx__oglCheckErrors();
    // NOTE: ctx can be 0 to unset the current context
    //_sg.gl.cur_context = ctx;
    //_sg_gl_reset_state_cache();
}

LOCAL void rx__glCacheBindBuffer(rx_OpenGlCtx* ctx, GLenum target, GLuint buffer) {
    switch (target) {
        case GL_ARRAY_BUFFER: {
            if (ctx->cache.vertexBuffer != buffer) {
                ctx->cache.vertexBuffer = buffer;
                glBindBuffer(target, buffer);
            }
        } break;
        case GL_ELEMENT_ARRAY_BUFFER: {
            if (ctx->cache.indexBuffer != buffer) {
                ctx->cache.indexBuffer = buffer;
                glBindBuffer(target, buffer);
            }
        } break;
        case GL_UNIFORM_BUFFER: {
            if (ctx->cache.uniformBuffer != buffer) {
                ctx->cache.uniformBuffer = buffer;
                glBindBuffer(target, buffer);
            }
        } break;
        default: ASSERT(!"Unknown buffer type");
    }
}

LOCAL void rx__glCacheStoreBufferBinding(rx_OpenGlCtx* ctx, GLenum target) {
    switch (target) {
        case GL_ARRAY_BUFFER: {
            ctx->cache.storedVertexBuffer = ctx->cache.vertexBuffer;
        } break;
        case GL_ELEMENT_ARRAY_BUFFER: {
            ctx->cache.storedIndexBuffer = ctx->cache.indexBuffer;
        } break;
        case GL_UNIFORM_BUFFER: {
            ctx->cache.storedUniformBuffer = ctx->cache.uniformBuffer;
        } break;
        default: ASSERT(!"Unknown buffer type");
    }
}

LOCAL void rx__glCacheRestoreBufferBinding(rx_OpenGlCtx* ctx, GLenum target) {
    switch (target) {
        case GL_ARRAY_BUFFER: {
            if (ctx->cache.storedVertexBuffer != 0) {
                // we only care about restoring valid ids
                rx__glCacheBindBuffer(ctx, target, ctx->cache.storedVertexBuffer);
                ctx->cache.storedVertexBuffer = 0;
            }
        } break;
        case GL_ELEMENT_ARRAY_BUFFER: {
            if (ctx->cache.storedIndexBuffer != 0) {
                // we only care about restoring valid ids
                rx__glCacheBindBuffer(ctx, target, ctx->cache.storedIndexBuffer);
                ctx->cache.storedIndexBuffer = 0;
            }
        } break;
        case GL_UNIFORM_BUFFER: {
            if (ctx->cache.storedUniformBuffer != 0) {
                // we only care about restoring valid ids
                rx__glCacheBindBuffer(ctx, target, ctx->cache.storedUniformBuffer);
                ctx->cache.storedUniformBuffer = 0;
            }
        } break;
        default: ASSERT(!"Unknown buffer type");
    }

    if (target == GL_ARRAY_BUFFER) {
        if (ctx->cache.storedVertexBuffer != 0) {
            // we only care about restoring valid ids
            rx__glCacheBindBuffer(ctx, target, ctx->cache.storedVertexBuffer);
            ctx->cache.storedVertexBuffer = 0;
        }
    } else {
        if (ctx->cache.storedIndexBuffer != 0) {
            // we only care about restoring valid ids
            rx__glCacheBindBuffer(ctx, target, ctx->cache.storedIndexBuffer);
            ctx->cache.storedIndexBuffer = 0;
        }
    }
}

LOCAL GLenum rx__glBufferTarget(rx_bufferUsage usage) {
    if ((usage & rx_bufferUsage_vertex) != 0) {
        return GL_ARRAY_BUFFER;
    }
    if ((usage & rx_bufferUsage_index) != 0) {
        return GL_ELEMENT_ARRAY_BUFFER;
    }
    if ((usage & rx_bufferUsage_uniform) != 0) {
        return GL_UNIFORM_BUFFER;
    }
    ASSERT(!"Unsupported buffer type");
    return 0;
}

LOCAL GLenum rx__glBufferUsage(rx_bufferUsage usage) {
    if ((usage & rx_bufferUsage_immutable) != 0) {
        return GL_STATIC_DRAW;
    }
    if ((usage & GL_STREAM_DRAW) != 0) {
        return GL_STREAM_DRAW;
    }
    return GL_DYNAMIC_DRAW;
}

LOCAL bx rx__glMakeBuffer(rx_Ctx* baseCtx, rx_Buffer* buffer, rx_BufferDesc* desc) {
    ASSERT(baseCtx && buffer && desc);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    rx__oglCheckErrors();

    const GLenum glTarget = rx__glBufferTarget(desc->usage);
    const GLenum glUsage  = rx__glBufferUsage(desc->usage);

    GLuint glBuffer = 0;
    glGenBuffers(1, &glBuffer);
    ASSERT(glBuffer != 0); 
    buffer->gl.handle = glBuffer;
    buffer->gl.usage = desc->usage;
    rx__oglCheckErrors();

    rx__glCacheStoreBufferBinding(ctx, glTarget);
    rx__glCacheBindBuffer(ctx, glTarget, glBuffer);
    glBufferData(glTarget, desc->size, 0, glUsage);
    rx__oglCheckErrors();
    //_sg_gl_cache_restore_buffer_binding(gl_target);
    //rx__oglCheckErrors();

    return true;
}

typedef enum ogl_error {
    ogl_GL_NO_ERROR =                       0,
    ogl_GL_INVALID_ENUM =                   0x0500,
    ogl_GL_INVALID_VALUE =                  0x0501,
    ogl_GL_INVALID_OPERATION =              0x0502,
    ogl_GL_STACK_OVERFLOW =                 0x0503,
    ogl_GL_STACK_UNDERFLOW =                0x0504,
    ogl_GL_OUT_OF_MEMORY =                  0x0505,
    ogl_error__forceU32 = RX_U32_MAX
} ogl_error;


LOCAL void rx__glUpdateBuffer(rx_Ctx* baseCtx, rx_Buffer* buffer, mms offset, rx_Range range) {
    ASSERT(baseCtx && buffer);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    GLenum glTarget = rx__glBufferTarget(buffer->usage);
    rx__glCacheStoreBufferBinding(ctx, glTarget);
    rx__glCacheBindBuffer(ctx, glTarget, buffer->gl.handle);
    rx__oglCheckErrors();
    glBufferSubData(glTarget, offset, (GLsizeiptr)range.size, range.content);
    rx__oglCheckErrors();
    rx__glCacheRestoreBufferBinding(ctx, glTarget);
    rx__oglCheckErrors();
}

#if 0
LOCAL GLenum rx__glMinFilter(sg_filter min_f, sg_filter mipmap_f) {
    if (min_f == SG_FILTER_NEAREST) {
        switch (mipmap_f) {
            case SG_FILTER_NONE:    return GL_NEAREST;
            case SG_FILTER_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
            case SG_FILTER_LINEAR:  return GL_NEAREST_MIPMAP_LINEAR;
            default: SOKOL_UNREACHABLE; return (GLenum)0;
        }
    } else if (min_f == SG_FILTER_LINEAR) {
        switch (mipmap_f) {
            case SG_FILTER_NONE:    return GL_LINEAR;
            case SG_FILTER_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
            case SG_FILTER_LINEAR:  return GL_LINEAR_MIPMAP_LINEAR;
            default: SOKOL_UNREACHABLE; return (GLenum)0;
        }
    } else {
        SOKOL_UNREACHABLE; return (GLenum)0;
    }
}
#endif

LOCAL GLenum rx__glMagFilter(rx_filterMode mag) {
    if (mag == rx_filterMode_nearest) {
        return GL_NEAREST;
    }
    return GL_LINEAR;
}
LOCAL GLenum rx__glFilterMode(rx_filterMode filterMode) {
    switch (filterMode) {
        case rx_filterMode_nearest: return GL_NEAREST;
        case rx_filterMode_linear:  return GL_LINEAR;
        default:
                                    RX_UNREACHABLE();
                                    return -1;
    }
}

LOCAL GLenum rx__glAddressMode(rx_addressMode addressMode) {
    switch (addressMode) {
        case rx_addressMode_repeat:       return GL_REPEAT;
        case rx_addressMode_mirrorRepeat: return GL_MIRRORED_REPEAT;
        case rx_addressMode_clampToEdge:  return GL_CLAMP_TO_EDGE;
        default:
                                          RX_UNREACHABLE();
                                          return -1;
    }
}

LOCAL GLenum rx__glCompareFunc(rx_compareFunc compareFunc) {
    switch (compareFunc) {
        case rx_compareFunc_never:          return GL_NEVER;  
        case rx_compareFunc_less:           return GL_LESS;  
        case rx_compareFunc_equal:          return GL_EQUAL;  
        case rx_compareFunc_lessEqual:      return GL_LEQUAL;  
        case rx_compareFunc_greater:        return GL_GREATER;  
        case rx_compareFunc_notEqual:       return GL_NOTEQUAL;  
        case rx_compareFunc_greaterEqual:   return GL_GEQUAL;  
        case rx_compareFunc_always:         return GL_ALWAYS;  
        default:
                                            RX_UNREACHABLE();
                                            return -1;
    }
}

LOCAL void rx__glMakeSampler(rx_Ctx* baseCtx, rx_Sampler* sampler, rx_SamplerDesc* samplerDesc) {
    ASSERT(baseCtx && sampler && samplerDesc);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    
    glGenSamplers(1, &sampler->gl.handle);
    ASSERT(sampler->gl.handle);

    const GLenum glMinFilter = rx__glFilterMode(samplerDesc->magFilter);
    // rx__glFilterMode(samplerDesc->minFilter, samplerDesc->cmn.mipmapFilter);
    const GLenum glMagFilter = rx__glFilterMode(samplerDesc->magFilter);
    glSamplerParameteri(sampler->gl.handle, GL_TEXTURE_MIN_FILTER, (GLint) glMinFilter);
    glSamplerParameteri(sampler->gl.handle, GL_TEXTURE_MAG_FILTER, (GLint) glMagFilter);
    // GL spec has strange defaults for mipmap min/max lod: -1000 to +1000
    const float minLod = clampVal(samplerDesc->lodMinClamp, 0.0f, 1000.0f);
    const float maxLod = clampVal(samplerDesc->lodMaxClamp, 0.0f, 1000.0f);
    glSamplerParameterf(sampler->gl.handle, GL_TEXTURE_MIN_LOD, minLod);
    glSamplerParameterf(sampler->gl.handle, GL_TEXTURE_MAX_LOD, maxLod);
    glSamplerParameteri(sampler->gl.handle, GL_TEXTURE_WRAP_S, (GLint) rx__glAddressMode(samplerDesc->addressModeU));
    glSamplerParameteri(sampler->gl.handle, GL_TEXTURE_WRAP_T, (GLint) rx__glAddressMode(samplerDesc->addressModeV));
    glSamplerParameteri(sampler->gl.handle, GL_TEXTURE_WRAP_R, (GLint) rx__glAddressMode(samplerDesc->addressModeW));
#if defined(RX_GLCORE33)
    f32 border[4] = {0, 0, 0, 1};
    #if 0
    switch (samplerDesc->borderColor) {
        case SG_BORDERCOLOR_TRANSPARENT_BLACK:
            border[0] = 0.0f; border[1] = 0.0f; border[2] = 0.0f; border[3] = 0.0f;
            break;
        case SG_BORDERCOLOR_OPAQUE_WHITE:
            border[0] = 1.0f; border[1] = 1.0f; border[2] = 1.0f; border[3] = 1.0f;
            break;
        default:
            border[0] = 0.0f; border[1] = 0.0f; border[2] = 0.0f; border[3] = 1.0f;
            break;
    }
    #endif
    glSamplerParameterfv(sampler->gl.handle, GL_TEXTURE_BORDER_COLOR, border);
#endif
    if (samplerDesc->compare != rx_compareFunc_never) {
        glSamplerParameteri(sampler->gl.handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(sampler->gl.handle, GL_TEXTURE_COMPARE_FUNC, (GLint)rx__glCompareFunc(samplerDesc->compare));
    } else {
        glSamplerParameteri(sampler->gl.handle, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }
    if (ctx->extAnisotropic && (ctx->maxAnisotropy > 1)) {
        GLint maxAnisotropy = (GLint) samplerDesc->maxAnisotropy;
        if (maxAnisotropy > ctx->maxAnisotropy) {
            maxAnisotropy = ctx->maxAnisotropy;
        }
        glSamplerParameteri(sampler->gl.handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    }
}

LOCAL GLenum rx__shaderStage(rx_shaderStage stage) {
    return stage == rx_shaderStage_vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
}

LOCAL GLuint rx__compileShader(rx_OpenGlCtx* ctx, rx_shaderStage stage, const Str8 source) {
    rx__oglCheckErrors();
    GLuint glShd = glCreateShader(rx__shaderStage(stage));
    glShaderSource(glShd, 1, (const GLchar**) &source.content, (const GLint*) &source.size);
    glCompileShader(glShd);
    GLint compileStatus = 0;
    glGetShaderiv(glShd, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        // compilation failed, log error and delete shader
        GLint logLen = 0;
        glGetShaderiv(glShd, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0) {
            mem_scoped(memName, ctx->base.arena) {
                GLchar* logBuf = (GLchar*) mem_arenaPush(memName.arena, logLen);
                glGetShaderInfoLog(glShd, logLen, &logLen, logBuf);
                rx__setError(&ctx->base, rx_error_shaderCompilationFailed);
                rx__log(&ctx->base, str_fromNullTerminatedCharPtr(logBuf));
            }
        }
        glDeleteShader(glShd);
        glShd = 0;
    }
    rx__oglCheckErrors();
    return glShd;
}

LOCAL bx rx__glMakeRenderShader(rx_Ctx* baseCtx, rx_RenderShader* renderShader, const rx_RenderShaderDesc* desc) {
    ASSERT(baseCtx && renderShader && desc);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    rx__oglCheckErrors();

    GLuint vsShader = rx__compileShader(ctx, rx_shaderStage_vertex, desc->vs.source);
    GLuint fsShader = rx__compileShader(ctx, rx_shaderStage_fragment, desc->fs.source);
    renderShader->gl.handle = 0;
    if (!(vsShader && fsShader)) {
        return false;
    }

    GLuint glProgam = glCreateProgram();
    glAttachShader(glProgam, vsShader);
    glAttachShader(glProgam, fsShader);
    glLinkProgram(glProgam);
    glDeleteShader(vsShader);
    glDeleteShader(fsShader);
    rx__oglCheckErrors();

    GLint linkStatus = 0;
    glGetProgramiv(glProgam, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        // compilation failed, log error and delete shader
        GLint logLen = 0;
        glGetProgramiv(glProgam, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0) {
            mem_scoped(memName, ctx->base.arena) {
                GLchar* logBuf = (GLchar*) mem_arenaPush(memName.arena, logLen);
                glGetProgramInfoLog(glProgam, logLen, &logLen, logBuf);
                rx__setError(&ctx->base, rx_error_shaderLinkFailed);
                rx__log(&ctx->base, str_fromNullTerminatedCharPtr(logBuf));
            }
        }
        glDeleteProgram(glProgam);
        return false;
    }
    rx__oglCheckErrors();

    renderShader->gl.handle = glProgam;


    GLuint resGroupIndex[5];
    // resolve uniforms
    resGroupIndex[0] = glGetUniformBlockIndex(glProgam, "resGroup0");
    resGroupIndex[1] = glGetUniformBlockIndex(glProgam, "resGroup1");
    resGroupIndex[2] = glGetUniformBlockIndex(glProgam, "resGroup2");
    resGroupIndex[3] = glGetUniformBlockIndex(glProgam, "resGroup3");
    resGroupIndex[4] = glGetUniformBlockIndex(glProgam, "resGroup4");



    for (u32 idx = 0; idx < countOf(resGroupIndex); idx++) {
        if (resGroupIndex[idx] != GL_INVALID_INDEX) {
            glUniformBlockBinding(glProgam, resGroupIndex[idx], idx);
        }
    }

    // resolve combined image samplers

    GLuint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&currentProgram);
    glUseProgram(currentProgram);

    glUseProgram(currentProgram);
    rx__oglCheckErrors();

    return true;
}

LOCAL void rx__glMakeResGroupLayout(rx_Ctx* baseCtx, rx_ResGroupLayout* resGroupLayout, const rx_ResGroupLayoutDesc* desc) {
    ASSERT(baseCtx && resGroupLayout && desc);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;

    for (u32 idx = 0; idx < countOf(desc->resources); idx++) {
        resGroupLayout->gl.resources[idx] = desc->resources[idx];
    }
}

LOCAL void rx__glInternalUpdateResGroup(rx_Ctx* baseCtx, rx_ResGroup* resGroup, rx_ResGroupUpdateDesc* desc) {
    ASSERT(baseCtx && resGroup && desc);

}

LOCAL void rx__glUpdateResGroup(rx_Ctx* baseCtx, rx_ResGroup* resGroup, const rx_ResGroupUpdateDesc* desc, bx isNew) {
    ASSERT(baseCtx && resGroup && desc);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    if (resGroup->uniformSize) {
        u32 size = resGroup->uniformSize;
        u32 offset = resGroup->targetOffset;
        rx_buffer target = desc->target;
        if (target.id == 0) {
            switch (resGroup->usage) {
                case rx_resGroupUsage_dynamic: {
                    target = ctx->uniform.dynamic.buffer;
                    ASSERT(!"ALLOC uniform space");
                    //offset = ctx->uniform.dynamic.currentOffset;
                    //ctx->uniform.dynamic.currentOffset += size;
                } break;
                case rx_resGroupUsage_streaming: {
                    target = ctx->uniform.streaming.buffer;
                    if (!isNew && resGroup->lastUpdateFrameIdx == baseCtx->frameIdx) {
                        // updating a res group twice a frame, just reused the existing slot
                        offset = resGroup->targetOffset;
                    } else {
                        offset = ctx->uniform.streaming.currentOffset;
                        offset = alignUp(ctx->uniform.streaming.currentOffset, baseCtx->limits.uniformBufferAlignment);

                        ctx->uniform.streaming.currentOffset = offset + size;
                    }

                } break;
                default: ASSERT(!"Unknown usage type");
            }
        }
        resGroup->targetOffset = offset;
        resGroup->target = target;
        rx_Buffer* uniformBuffer = rx__getBuffer(baseCtx, target);
        rx__glUpdateBuffer(baseCtx, uniformBuffer, offset, desc->uniformContent);
    }
    flags64 dependencies = 0;
    for (u32 idx = 0; idx < countOf(resGroup->gl.resources); idx++) {
        if (desc->resources[idx].texture.id != 0) {
            if (desc->resources[idx].texture.passIdx != 0) {
                dependencies |= 1 << desc->resources[idx].texture.passIdx;
            }
            resGroup->gl.resources[idx].texture = desc->resources[idx].texture;
            resGroup->gl.resources[idx].type    = rx__resType_texture;
        } else if (desc->resources[idx].sampler.id != 0) {
            resGroup->gl.resources[idx].sampler = desc->resources[idx].sampler;
            resGroup->gl.resources[idx].type    = rx__resType_sampler;
        } else {
            resGroup->gl.resources[idx].type    = rx__resType_none;
        }
    }
    resGroup->passDepFlags = dependencies;
    resGroup->lastUpdateFrameIdx = baseCtx->frameIdx;
}

LOCAL void rx__glMakeResGroup(rx_Ctx* baseCtx, rx_ResGroup* resGroup, const rx_ResGroupDesc* desc) {
    ASSERT(baseCtx && resGroup && desc);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    rx_ResGroupLayout* resGroupLayout = rx__getResGroupLayout(baseCtx, resGroup->layout);
    resGroup->uniformSize = resGroupLayout->uniformSize;
    resGroup->usage = desc->usage;
    rx__glUpdateResGroup(baseCtx, resGroup, &desc->initalContent, true);
}

LOCAL GLenum rx__glVertexformatType(rx_vertexFormat fmt) {
    switch (fmt) {
        case rx_vertexFormat_f32x1:
        case rx_vertexFormat_f32x2:
        case rx_vertexFormat_f32x3:
        case rx_vertexFormat_f32x4:
            return GL_FLOAT;
        case rx_vertexFormat_s8x2:
        case rx_vertexFormat_s8x4:
        case rx_vertexFormat_s8x2n:
        case rx_vertexFormat_s8x4n:
            return GL_BYTE;
        case rx_vertexFormat_u8x2:
        case rx_vertexFormat_u8x4:
        case rx_vertexFormat_u8x2n:
        case rx_vertexFormat_u8x4n:
            return GL_UNSIGNED_BYTE;
        case rx_vertexFormat_s16x2:
        case rx_vertexFormat_s16x4:
        case rx_vertexFormat_s16x2n:
        case rx_vertexFormat_s16x4n:
            return GL_SHORT;
        case rx_vertexFormat_u16x2:
        case rx_vertexFormat_u16x4:
        case rx_vertexFormat_u16x2n:
        case rx_vertexFormat_u16x4n:
            return GL_UNSIGNED_SHORT;
        case rx_vertexFormat_u2x10x10x10:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        case rx_vertexFormat_f16x2:
        case rx_vertexFormat_f16x4:
            return GL_HALF_FLOAT;
        default:
            RX_UNREACHABLE(); return 0;
    }
}

LOCAL GLboolean rx__glVertexformatNormalized(rx_vertexFormat fmt) {
    switch (fmt) {
        case rx_vertexFormat_u8x2n:
        case rx_vertexFormat_u8x4n:
        case rx_vertexFormat_s8x2n:
        case rx_vertexFormat_s8x4n:
        case rx_vertexFormat_u16x2n:
        case rx_vertexFormat_u16x4n:
        case rx_vertexFormat_s16x2n:
        case rx_vertexFormat_s16x4n:
            return GL_TRUE;
        default:
            return GL_FALSE;
    }
}

LOCAL GLenum rx__glPrimitiveTopology(rx_primitiveTopology t) {
    switch (t) {
        case rx_primitiveTopology_pointList: return GL_POINTS;
        case rx_primitiveTopology_lineList: return GL_LINES;
        case rx_primitiveTopology_lineStrip: return GL_LINE_STRIP;
        case rx_primitiveTopology_triangleList: return GL_TRIANGLES;
        case rx_primitiveTopology_triangleStrip: return GL_TRIANGLE_STRIP;
        case rx_primitiveTopology_triangleFan: return GL_TRIANGLE_FAN;
        case rx_primitiveTopology_lineListWithAdjancy: return GL_LINES_ADJACENCY;
        case rx_primitiveTopology_lineStripWithAdjancy: return GL_LINE_STRIP_ADJACENCY;
        case rx_primitiveTopology_triangleListWithAdjancy: return GL_TRIANGLES_ADJACENCY;
        case rx_primitiveTopology_triangleStripWithAdjancy: return GL_TRIANGLE_STRIP_ADJACENCY;
        case rx_primitiveTopology_patchList: return GL_PATCHES;
        default: RX_UNREACHABLE(); return 0;
    }
}

LOCAL GLenum rx__glIndexType(rx_indexType t) {
    switch (t) {
        case rx_indexType_none:     return 0;
        case rx_indexType_u16:      return GL_UNSIGNED_SHORT;
        case rx_indexType_u32:      return GL_UNSIGNED_INT;
        default: RX_UNREACHABLE(); return 0;
    }
}

LOCAL GLint rx__glVertexformatSize(rx_vertexFormat fmt) {
    switch (fmt) {
        case rx_vertexFormat_u8x2:           return 2;
        case rx_vertexFormat_u8x4:           return 4;
        case rx_vertexFormat_s8x2:           return 2;
        case rx_vertexFormat_s8x4:           return 4;
        case rx_vertexFormat_u8x2n:          return 2;
        case rx_vertexFormat_u8x4n:          return 4;
        case rx_vertexFormat_s8x2n:          return 2;
        case rx_vertexFormat_s8x4n:          return 4;
        case rx_vertexFormat_u2x10x10x10:    return 4;
        case rx_vertexFormat_u16x2:          return 2;
        case rx_vertexFormat_u16x4:          return 4;
        case rx_vertexFormat_s16x2:          return 2;
        case rx_vertexFormat_s16x4:          return 4;
        case rx_vertexFormat_u16x2n:         return 2;
        case rx_vertexFormat_u16x4n:         return 4;
        case rx_vertexFormat_s16x2n:         return 2;
        case rx_vertexFormat_s16x4n:         return 4;
        case rx_vertexFormat_f16x2:          return 2;
        case rx_vertexFormat_f16x4:          return 4;
        case rx_vertexFormat_u32x1:          return 1;
        case rx_vertexFormat_u32x2:          return 2;
        case rx_vertexFormat_u32x3:          return 3;
        case rx_vertexFormat_u32x4:          return 4;
        case rx_vertexFormat_s32x1:          return 1;
        case rx_vertexFormat_s32x2:          return 2;
        case rx_vertexFormat_s32x3:          return 3;
        case rx_vertexFormat_s32x4:          return 4;
        case rx_vertexFormat_f32x1:          return 1;
        case rx_vertexFormat_f32x2:          return 2;
        case rx_vertexFormat_f32x3:          return 3;
        case rx_vertexFormat_f32x4:          return 4;
        default: RX_UNREACHABLE();           return 0;
    }
}

LOCAL bx rx__glMakeRenderPipeline(rx_Ctx* baseCtx, rx_RenderPipeline* renderPipeline, const rx_RenderPipelineDesc* desc) {
    ASSERT(baseCtx && renderPipeline && desc);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;

    renderPipeline->gl.shader = desc->program.shader;
    renderPipeline->gl.primitiveTopology = rx__glPrimitiveTopology(desc->primitiveTopology);
    renderPipeline->gl.indexType = rx__glIndexType(desc->indexFormat);
    renderPipeline->gl.depthWriteEnabled = desc->depthStencil.depthWriteEnabled;
    renderPipeline->gl.stencilEnabled = desc->depthStencil.stencilEnabled;
    // FIXME: blend color and write mask per draw-buffer-attachment (requires GL4)

    renderPipeline->gl.depthStencilState = desc->depthStencil;

    mem_copy(&renderPipeline->gl.colorTargets[0], &desc->colorTargets[0], sizeOf(renderPipeline->gl.colorTargets));

    renderPipeline->gl.cullMode = desc->rasterizer.cullMode;
    renderPipeline->gl.faceWinding = desc->rasterizer.faceWinding;
    renderPipeline->gl.sampleCount = desc->sampleCount;
    renderPipeline->gl.alphaToCoverageEnabled = desc->alphaToCoverageEnabled;

    renderPipeline->gl.useInstancedDraw = false;

    GLint shaderHandle = rx__getRenderShader(baseCtx, renderPipeline->gl.shader)->gl.handle;

    for (u32 attrIdx = 0; attrIdx < countOf(renderPipeline->gl.vertexAttributes); attrIdx++) {
        renderPipeline->gl.vertexAttributes[attrIdx].vbIndex = -1;
    }

    for (u32 attrIdx = 0; attrIdx < countOf(desc->layout.attrs); attrIdx++) {
        const rx_VertexAttrDesc* attribute = &desc->layout.attrs[attrIdx];
        if (attribute->format == rx_vertexFormat__invalid) {
            continue;
        }

        rx_vertexStep stepFunc = desc->layout.buffers[attribute->bufferIndex].stepFunc;
        u32 stepRate = desc->layout.buffers[attribute->bufferIndex].stepRate;
        GLint attrLoc = (GLint) attrIdx;
        u32 stride = desc->layout.buffers[attribute->bufferIndex].stride;

        if (attribute->name.size > 0) {
            // build a null terminated string
            u8 name[1025];
            ASSERT(attribute->name.size < sizeOf(name));
            mem_copy(name, attribute->name.content, attribute->name.size);
            name[attribute->name.size] = '\0';
            attrLoc = glGetAttribLocation(shaderHandle, (const GLchar*) &name[0]);
        }

        if (attrLoc != -1) {
            rx__GlVertexAttribute* glAttribute = &renderPipeline->gl.vertexAttributes[attrLoc];

            glAttribute->vbIndex = attribute->bufferIndex;
            if (stepFunc == rx_vertexStep_perVertex) {
                glAttribute->divisor = 0;
            } else {
                glAttribute->divisor = (i8) stepRate;
                renderPipeline->gl.useInstancedDraw = true;
            }

            glAttribute->stride = (i8) stride;
            glAttribute->offset = attribute->offset;
            glAttribute->size = (i8) rx__glVertexformatSize(attribute->format);
            glAttribute->type = rx__glVertexformatType(attribute->format);
            glAttribute->normalized = rx__glVertexformatNormalized(attribute->format);
        } else {
            ASSERT(!"Vertex attribute was not found");
        }
    }


    return true;
}
#if 0
_SOKOL_PRIVATE sg_resource_state _sg_gl_create_pipeline(_sg_pipeline_t* pip, _sg_shader_t* shd, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && shd && desc);
    SOKOL_ASSERT((pip->shader == 0) && (pip->cmn.shader_id.id != SG_INVALID_ID));
    SOKOL_ASSERT(desc->shader.id == shd->slot.id);
    SOKOL_ASSERT(shd->gl.prog);
    pip->shader = shd;
    pip->gl.primitive_type = desc->primitive_type;
    pip->gl.depth = desc->depth;
    pip->gl.stencil = desc->stencil;
    // FIXME: blend color and write mask per draw-buffer-attachment (requires GL4)
    pip->gl.blend = desc->colors[0].blend;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        pip->gl.color_write_mask[i] = desc->colors[i].write_mask;
    }
    pip->gl.cull_mode = desc->cull_mode;
    pip->gl.face_winding = desc->face_winding;
    pip->gl.sample_count = desc->sample_count;
    pip->gl.alpha_to_coverage_enabled = desc->alpha_to_coverage_enabled;

    // resolve vertex attributes
    for (int attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
        pip->gl.attrs[attr_index].vb_index = -1;
    }
    for (int attr_index = 0; attr_index < _sg.limits.max_vertex_attrs; attr_index++) {
        const sg_vertex_attr_state* a_state = &desc->layout.attrs[attr_index];
        if (a_state->format == SG_VERTEXFORMAT_INVALID) {
            break;
        }
        SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEX_BUFFERS);
        const sg_vertex_buffer_layout_state* l_state = &desc->layout.buffers[a_state->buffer_index];
        const sg_vertex_step step_func = l_state->step_func;
        const int step_rate = l_state->step_rate;
        GLint attr_loc = attr_index;
        if (!_sg_strempty(&shd->gl.attrs[attr_index].name)) {
            attr_loc = glGetAttribLocation(pip->shader->gl.prog, _sg_strptr(&shd->gl.attrs[attr_index].name));
        }
        SOKOL_ASSERT(attr_loc < (GLint)_sg.limits.max_vertex_attrs);
        if (attr_loc != -1) {
            _sg_gl_attr_t* gl_attr = &pip->gl.attrs[attr_loc];
            SOKOL_ASSERT(gl_attr->vb_index == -1);
            gl_attr->vb_index = (int8_t) a_state->buffer_index;
            if (step_func == SG_VERTEXSTEP_PER_VERTEX) {
                gl_attr->divisor = 0;
            } else {
                gl_attr->divisor = (int8_t) step_rate;
                pip->cmn.use_instanced_draw = true;
            }
            SOKOL_ASSERT(l_state->stride > 0);
            gl_attr->stride = (uint8_t) l_state->stride;
            gl_attr->offset = a_state->offset;
            gl_attr->size = (uint8_t) _sg_gl_vertexformat_size(a_state->format);
            gl_attr->type = _sg_gl_vertexformat_type(a_state->format);
            gl_attr->normalized = _sg_gl_vertexformat_normalized(a_state->format);
            pip->cmn.vertex_buffer_layout_active[a_state->buffer_index] = true;
        } else {
            _SG_ERROR(GL_VERTEX_ATTRIBUTE_NOT_FOUND_IN_SHADER);
            _SG_LOGMSG(GL_VERTEX_ATTRIBUTE_NOT_FOUND_IN_SHADER, _sg_strptr(&shd->gl.attrs[attr_index].name));
        }
    }
    return SG_RESOURCESTATE_VALID;
}
#endif

LOCAL bx rx__glCreateSwapChain(rx_Ctx* baseCtx, rx_SwapChain* swapChain) {
    ASSERT(baseCtx);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;

    rx_texture texHandle = rx__allocTexture(baseCtx);
    rx_Texture* texture = rx__getTexture(baseCtx, texHandle);
    texture->belongsToSwapChain = true;

    swapChain->textures[0] = texHandle;

    swapChain->textureCount = 1;

    // SwapChain in OpenGL can be platform driven.
    // MacOS: updated by the system and can be only used on provided callback
    // TODO(PJ): iOS, Android, Windows, Linux
    GLuint defaultFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&defaultFramebuffer);
    return true;
}

LOCAL rx_texture rx__glGetCurrentSwapTexture(rx_Ctx* baseCtx, rx_SwapChain* swapChain) {
    ASSERT(baseCtx);
    ASSERT(swapChain);
    // swapchain is external controlled and created for MacOS
    // TODO(PJ): might do something different on other platforms
    return swapChain->textures[0];
}

LOCAL GLenum rx__glStencilOp(rx_stencilOp op) {
    switch (op) {
        case rx_stencilOp_keep:         return GL_KEEP;
        case rx_stencilOp_zero:         return GL_ZERO;
        case rx_stencilOp_replace:      return GL_REPLACE;
        case rx_stencilOp_incrClamp:    return GL_INCR;
        case rx_stencilOp_decrClamp:    return GL_DECR;
        case rx_stencilOp_invert:       return GL_INVERT;
        case rx_stencilOp_incrWrap:     return GL_INCR_WRAP;
        case rx_stencilOp_decrWrap:     return GL_DECR_WRAP;
        default: RX_UNREACHABLE(); return 0;
    }
}

LOCAL GLenum rx__glBlendFactor(rx_blendFactor f) {
    switch (f) {
        case rx_blendFactor_zero:                return GL_ZERO;
        case rx_blendFactor_one:                 return GL_ONE;
        case rx_blendFactor_srcColor:            return GL_SRC_COLOR;
        case rx_blendFactor_oneMinusSrcColor:    return GL_ONE_MINUS_SRC_COLOR;
        case rx_blendFactor_srcAlpha:            return GL_SRC_ALPHA;
        case rx_blendFactor_oneMinusSrcAlpha:    return GL_ONE_MINUS_SRC_ALPHA;
        case rx_blendFactor_dstColor:            return GL_DST_COLOR;
        case rx_blendFactor_oneMinusDstColor:    return GL_ONE_MINUS_DST_COLOR;
        case rx_blendFactor_dstAlpha:            return GL_DST_ALPHA;
        case rx_blendFactor_oneMinusDstAlpha:    return GL_ONE_MINUS_DST_ALPHA;
        case rx_blendFactor_srcAlphaSaturated:   return GL_SRC_ALPHA_SATURATE;
        case rx_blendFactor_blendColor:          return GL_CONSTANT_COLOR;
        case rx_blendFactor_oneMinusBlendColor:  return GL_ONE_MINUS_CONSTANT_COLOR;
        case rx_blendFactor_blendAlpha:          return GL_CONSTANT_ALPHA;
        case rx_blendFactor_oneMinusBlendAlpha:  return GL_ONE_MINUS_CONSTANT_ALPHA;
        default: RX_UNREACHABLE(); return 0;
    }
}

LOCAL GLenum rx__glBlendOp(rx_blendOp op) {
    switch (op) {
        case rx_blendOp_add:                return GL_FUNC_ADD;
        case rx_blendOp_substract:          return GL_FUNC_SUBTRACT;
        case rx_blendOp_reverseSubstract:   return GL_FUNC_REVERSE_SUBTRACT;
        default: RX_UNREACHABLE(); return 0;
    }
}

typedef struct rx_GlStateCache {
    // Pipeline state cache
    rx_DepthStencilState depthStencilState;
    rx_RasterizerState rasterizerState;
    rx_TargetBlendState blend;
    bx polygonOffsetEnabled : 1;
    u8 colorWriteMask[RX_MAX_COLOR_TARGETS];
    //rx_cullMode cullMode;
    //rx_faceWinding faceWinding;
    rx_primitiveTopology primitiveTopology;
    bx alphaToCoverageEnabled : 1;
    rx_renderShader shader;
    u32 sampleCount;
    rx__GlVertexAttribute vertexAttributes[RX_MAX_VERTEX_ATTRIBUTES];
} rx_GlStateCache;

LOCAL void rx__glInitStateCache(rx_OpenGlCtx* ctx, rx_GlStateCache* stateCache) {

    mem_structSetZero(stateCache);

    rx__oglCheckErrors();
    glBindVertexArray(ctx->vao);
    rx__oglCheckErrors();
    mem_structSetZero(stateCache);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    rx__oglCheckErrors();
    // _sg_gl_cache_clear_texture_sampler_bindings(true);
    rx__oglCheckErrors();

    for (u32 idx = 0; idx < ctx->base.limits.maxVertexAttributes; idx++) {
        stateCache->vertexAttributes[idx].vbIndex = -1;
        glDisableVertexAttribArray(idx);
    }

    stateCache->primitiveTopology = GL_TRIANGLES;

    // shader program
    //glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&stateCache->prog);
    rx__oglCheckErrors();

    // depth and stencil state
    stateCache->depthStencilState.depthCompareFunc = rx_compareFunc_always;
    stateCache->depthStencilState.stencilFront.compareFunc = rx_compareFunc_always;
    stateCache->depthStencilState.stencilFront.failOp = rx_stencilOp_keep;
    stateCache->depthStencilState.stencilFront.depthFailOp = rx_stencilOp_keep;
    stateCache->depthStencilState.stencilFront.passOp = rx_stencilOp_keep;
    stateCache->depthStencilState.stencilBack.compareFunc = rx_compareFunc_always;
    stateCache->depthStencilState.stencilBack.failOp = rx_stencilOp_keep;
    stateCache->depthStencilState.stencilBack.depthFailOp = rx_stencilOp_keep;
    stateCache->depthStencilState.stencilBack.passOp = rx_stencilOp_keep;
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0);
    // _sg_stats_add(gl.num_render_state, 7);

    // blend state
    stateCache->blend.srcFactorRgb = rx_blendFactor_one;
    stateCache->blend.dstFactorRgb = rx_blendFactor_zero;
    stateCache->blend.opRgb = rx_blendOp_add;
    stateCache->blend.srcFactorAlpha = rx_blendFactor_one;
    stateCache->blend.dstFactorAlpha = rx_blendFactor_zero;
    stateCache->blend.opAlpha = rx_blendOp_add;
    glDisable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);
    // _sg_stats_add(gl.num_render_state, 4);

    // standalone state
    for (u32 i = 0; i < RX_MAX_COLOR_TARGETS; i++) {
        stateCache->colorWriteMask[i] = rx_colorMask_rgba;
    }

    stateCache->rasterizerState.cullMode = rx_cullMode_none;
    stateCache->rasterizerState.faceWinding = rx_faceWinding_clockwise;
    stateCache->sampleCount = 1;
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glEnable(GL_DITHER);
    glDisable(GL_POLYGON_OFFSET_FILL);
#if defined(SOKOL_GLCORE33)
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_PROGRAM_POINT_SIZE);
#endif
}

LOCAL void rx__glApplyRenderPipeline(rx_OpenGlCtx* ctx, rx_RenderPipeline* renderPipeline, rx_GlStateCache* stateCache) {
    ASSERT(ctx);
    ASSERT(renderPipeline);
    ASSERT(stateCache);
    rx__oglCheckErrors();

    // update depth state
    {
        
        if (renderPipeline->gl.depthStencilState.depthCompareFunc != stateCache->depthStencilState.depthCompareFunc) {
            stateCache->depthStencilState.depthCompareFunc = renderPipeline->gl.depthStencilState.depthCompareFunc;
            glDepthFunc(rx__glCompareFunc(stateCache->depthStencilState.depthCompareFunc));
        }
        if (renderPipeline->gl.depthStencilState.depthWriteEnabled != stateCache->depthStencilState.depthWriteEnabled) {
            stateCache->depthStencilState.depthWriteEnabled = renderPipeline->gl.depthStencilState.depthWriteEnabled;
            glDepthMask(stateCache->depthStencilState.depthWriteEnabled);
        }
        
        if (!f32_equal(renderPipeline->gl.rasterizerState.depthBias, stateCache->rasterizerState.depthBias, 0.000001f) ||
            !f32_equal(renderPipeline->gl.rasterizerState.depthBiasSlopeScale, stateCache->rasterizerState.depthBiasSlopeScale, 0.000001f)) {
            /* according to ANGLE's D3D11 backend:
                D3D11 SlopeScaledDepthBias ==> GL polygonOffsetFactor
                D3D11 DepthBias ==> GL polygonOffsetUnits
                DepthBiasClamp has no meaning on GL
            */
            stateCache->rasterizerState.depthBias = renderPipeline->gl.rasterizerState.depthBias;
            stateCache->rasterizerState.depthBiasSlopeScale = renderPipeline->gl.rasterizerState.depthBiasSlopeScale;

            glPolygonOffset(stateCache->rasterizerState.depthBiasSlopeScale, stateCache->rasterizerState.depthBias);

            bool poEnabled = true;
            if (f32_equal(stateCache->rasterizerState.depthBias, 0.0f, 0.000001f) && f32_equal(stateCache->rasterizerState.depthBiasSlopeScale , 0.0f, 0.000001f)) {
                poEnabled = false;
            }
            if (poEnabled != stateCache->polygonOffsetEnabled) {
                stateCache->polygonOffsetEnabled = poEnabled;
                if (poEnabled) {
                    glEnable(GL_POLYGON_OFFSET_FILL);
                } else {
                    glDisable(GL_POLYGON_OFFSET_FILL);
                }
            }
        }
    }

    // update stencil state
    {
        if (renderPipeline->gl.depthStencilState.stencilEnabled != stateCache->depthStencilState.stencilEnabled) {
            stateCache->depthStencilState.stencilEnabled = renderPipeline->gl.depthStencilState.stencilEnabled;
            if (stateCache->depthStencilState.stencilEnabled) {
                glEnable(GL_STENCIL_TEST);
            } else {
                glDisable(GL_STENCIL_TEST);
            }
        }
        if (renderPipeline->gl.depthStencilState.stencilWriteMask != stateCache->depthStencilState.stencilWriteMask) {
            stateCache->depthStencilState.stencilWriteMask = renderPipeline->gl.depthStencilState.stencilWriteMask;
            glStencilMask(stateCache->depthStencilState.stencilWriteMask);
        }
        for (i32 i = 0; i < 2; i++) {
            rx_StencilState* pipStencilState = (i == 0) ? &renderPipeline->gl.depthStencilState.stencilFront : &renderPipeline->gl.depthStencilState.stencilBack;
            rx_StencilState* cacheStencilState = (i == 0) ? &stateCache->depthStencilState.stencilFront : &stateCache->depthStencilState.stencilBack;
            GLenum glFace = (i==0)? GL_FRONT : GL_BACK;
            if ((pipStencilState->compareFunc != cacheStencilState->compareFunc) || (renderPipeline->gl.depthStencilState.stencilReadMask != stateCache->depthStencilState.stencilReadMask) || (renderPipeline->gl.depthStencilState.stencilRef != stateCache->depthStencilState.stencilRef)) {
                cacheStencilState->compareFunc = pipStencilState->compareFunc;
                glStencilFuncSeparate(
                    glFace,
                    rx__glCompareFunc(pipStencilState->compareFunc),
                    renderPipeline->gl.depthStencilState.stencilRef,
                    renderPipeline->gl.depthStencilState.stencilReadMask
                );
            }
            if ((pipStencilState->failOp != cacheStencilState->failOp) ||
                (pipStencilState->depthFailOp != cacheStencilState->depthFailOp) ||
                (pipStencilState->passOp != cacheStencilState->passOp))
            {
                cacheStencilState->failOp = pipStencilState->failOp;
                cacheStencilState->depthFailOp = pipStencilState->depthFailOp;
                cacheStencilState->passOp = pipStencilState->passOp;
                glStencilOpSeparate(
                    glFace,
                    rx__glStencilOp(pipStencilState->failOp),
                    rx__glStencilOp(pipStencilState->depthFailOp),
                    rx__glStencilOp(pipStencilState->passOp)
                );
            }
        }
        stateCache->depthStencilState.stencilReadMask = renderPipeline->gl.depthStencilState.stencilReadMask;
        stateCache->depthStencilState.stencilRef = renderPipeline->gl.depthStencilState.stencilRef;
    }

    if (renderPipeline->gl.colorTargetCount > 0) {
        // update blend state
        // FIXME: separate blend state per color attachment not support, needs GL >= 4.2
        rx_TargetBlendState* cacheBlend = &stateCache->blend;
        rx_TargetBlendState* pipBlend = &renderPipeline->gl.blend;
        if (pipBlend->enabled != cacheBlend->enabled) {
            cacheBlend->enabled = pipBlend->enabled;
            if (pipBlend->enabled) {
                glEnable(GL_BLEND);
            } else {
                glDisable(GL_BLEND);
            }
        }
        if ((pipBlend->srcFactorRgb   != cacheBlend->srcFactorRgb) ||
            (pipBlend->dstFactorRgb   != cacheBlend->dstFactorRgb) ||
            (pipBlend->srcFactorAlpha != cacheBlend->srcFactorAlpha) ||
            (pipBlend->dstFactorAlpha != cacheBlend->dstFactorAlpha)) {
            cacheBlend->srcFactorRgb   = pipBlend->srcFactorRgb;
            cacheBlend->dstFactorRgb   = pipBlend->dstFactorRgb;
            cacheBlend->srcFactorAlpha = pipBlend->srcFactorAlpha;
            cacheBlend->dstFactorAlpha = pipBlend->dstFactorAlpha;
            glBlendFuncSeparate(
                rx__glBlendFactor(pipBlend->srcFactorRgb),
                rx__glBlendFactor(pipBlend->dstFactorRgb),
                rx__glBlendFactor(pipBlend->srcFactorAlpha),
                rx__glBlendFactor(pipBlend->dstFactorAlpha)
            );
        }
        if ((pipBlend->opRgb != cacheBlend->opRgb) || (pipBlend->opAlpha != cacheBlend->opAlpha)) {
            cacheBlend->opRgb   = pipBlend->opRgb;
            cacheBlend->opAlpha = pipBlend->opAlpha;
            glBlendEquationSeparate(rx__glBlendOp(pipBlend->opRgb), rx__glBlendOp(pipBlend->opAlpha));
        }

        // standalone color target state
        for (GLuint idx = 0; idx < (GLuint)renderPipeline->gl.colorTargetCount; idx++) {
            if (stateCache->colorWriteMask[idx] != renderPipeline->gl.colorWriteMask[idx]) {
                const rx_colorMask cm = renderPipeline->gl.colorWriteMask[idx];
                stateCache->colorWriteMask[idx] = cm;
                #ifdef SOKOL_GLCORE33
                    glColorMaski(
                        i,
                        (cm & SG_COLORMASK_R) != 0,
                        (cm & SG_COLORMASK_G) != 0,
                        (cm & SG_COLORMASK_B) != 0,
                        (cm & SG_COLORMASK_A) != 0
                    );
                #else
                    if (idx == 0) {
                        glColorMask(
                            (cm & rx_colorMask_r) != 0,
                            (cm & rx_colorMask_g) != 0,
                            (cm & rx_colorMask_b) != 0,
                            (cm & rx_colorMask_a) != 0
                        );
                    }
                #endif
            }
        }

        if (!f32_equal(pipBlend->blendColor.red,   cacheBlend->blendColor.red,   0.0001f) ||
            !f32_equal(pipBlend->blendColor.green, cacheBlend->blendColor.green, 0.0001f) ||
            !f32_equal(pipBlend->blendColor.blue,  cacheBlend->blendColor.blue,  0.0001f) ||
            !f32_equal(pipBlend->blendColor.alpha, cacheBlend->blendColor.alpha, 0.0001f))
        {
            Rgba c = pipBlend->blendColor;
            cacheBlend->blendColor = c;
            glBlendColor(c.red, c.green, c.blue, c.alpha);
        }
    } // pip->cmn.color_count > 0

    if (renderPipeline->gl.cullMode != stateCache->rasterizerState.cullMode) {
        stateCache->rasterizerState.cullMode = renderPipeline->gl.cullMode;
        if (renderPipeline->gl.cullMode == rx_cullMode_none) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            GLenum glMode = (renderPipeline->gl.cullMode == rx_cullMode_front) ? GL_FRONT : GL_BACK;
            glCullFace(glMode);
        }
    }
    if (renderPipeline->gl.faceWinding != stateCache->rasterizerState.faceWinding) {
        stateCache->rasterizerState.faceWinding = renderPipeline->gl.faceWinding;
        GLenum glWinding = (renderPipeline->gl.faceWinding = rx_faceWinding_clockwise) ? GL_CW : GL_CCW;
        glFrontFace(glWinding);
    }
    if (renderPipeline->gl.alphaToCoverageEnabled != stateCache->alphaToCoverageEnabled) {
        stateCache->alphaToCoverageEnabled = renderPipeline->gl.alphaToCoverageEnabled;
        if (renderPipeline->gl.alphaToCoverageEnabled) {
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        } else {
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
    }
    #ifdef SOKOL_GLCORE33
    if (renderPipeline->gl.sampleCount != stateCache->sampleCount) {
        stateCache->sampleCount = renderPipeline->gl.sampleCount;
        if (renderPipeline->gl.sampleCount > 1) {
            glEnable(GL_MULTISAMPLE);
        } else {
            glDisable(GL_MULTISAMPLE);
        }
    }
    #endif

    // bind shader program
    if (renderPipeline->gl.shader.id != stateCache->shader.id) {
        stateCache->shader = renderPipeline->gl.shader;
        GLuint programHandle = rx__getRenderShader(&ctx->base, renderPipeline->gl.shader)->gl.handle;
        glUseProgram(programHandle);
    }
    rx__oglCheckErrors();
}

LOCAL void rx__glExcuteDrawList(rx_OpenGlCtx* ctx, rx_GlStateCache* glStateCache, rx_DrawArea* drawAreas, u32 drawAreasCount, rx_DrawList* drawList, u32 maxWidth, u32 maxHeight) {
    ASSERT(ctx);
    ASSERT(drawList);

    rx_renderPipeline currentPipeline = {0};

    uint32_t indexOffset  = 0;
    uint32_t vertexOffset = 0;
    uint32_t indexCount   = 0;
    rx_ViewPort currentViewPort;
    currentViewPort.x = f32_infinity;
    currentViewPort.y = f32_infinity;
    currentViewPort.width = f32_infinity;
    currentViewPort.height = f32_infinity;
    currentViewPort.minDepth = f32_infinity;
    currentViewPort.maxDepth = f32_infinity;
    rx_ScissorRect currentScissor;
    currentScissor.offset.x = u16_max;
    currentScissor.offset.y = u16_max;
    currentScissor.extend.x = u16_max;
    currentScissor.extend.y = u16_max;

    rx_resGroup areaResBlock0 = {0};
    rx_resGroup drawResBlock1 = {0};
    rx_resGroup drawResBlock2 = {0};
    rx_resGroup drawResBlock3 = {0};

    rx_buffer boundVertexBuffers[3] = {0};
    rx_buffer boundIndexBuffer = {0};
    
    uint32_t pushIndicies[5] = {0};
    
    bool viewport = false;
    bool scissor = false;

    for (uint32_t drawAreaIdx = 0; drawAreaIdx < drawAreasCount; drawAreaIdx++) {
        rx_DrawArea* drawArea = drawAreas + drawAreaIdx;
        uint32_t endCount = drawArea->drawOffset + drawArea->drawCount;

        // set viewport if it changed
        if (drawArea->viewPort.x != currentViewPort.x || drawArea->viewPort.y != currentViewPort.y || drawArea->viewPort.width != currentViewPort.width || drawArea->viewPort.height != currentViewPort.height) {
            currentViewPort.x        = drawArea->viewPort.x;
            currentViewPort.y        = drawArea->viewPort.y;
            currentViewPort.width    = drawArea->viewPort.width;
            currentViewPort.height   = drawArea->viewPort.height;
            currentViewPort.minDepth = drawArea->viewPort.minDepth;
            currentViewPort.maxDepth = drawArea->viewPort.maxDepth;
            if (currentViewPort.width == 0) {
                currentViewPort.width = maxWidth - currentViewPort.x;
            }
            if (currentViewPort.height == 0) {
                currentViewPort.height = maxHeight - currentViewPort.y;
            }
            if (currentViewPort.maxDepth == 0) {
                currentViewPort.maxDepth = 1.0f;
            }
        }

        // set scisor if it changed
        if (drawArea->scissor.offset.x != currentScissor.offset.x || drawArea->scissor.offset.y != currentScissor.offset.y || drawArea->scissor.extend.x != currentScissor.extend.x || drawArea->scissor.extend.y != currentScissor.extend.y) {
            currentScissor.offset.x = drawArea->scissor.offset.x;
            currentScissor.offset.y = drawArea->scissor.offset.y;

            currentScissor.extend.x = drawArea->scissor.extend.x == 0 ? maxWidth : drawArea->scissor.extend.x;
            currentScissor.extend.y = drawArea->scissor.extend.y == 0 ? maxHeight : drawArea->scissor.extend.y;
        }

        glScissor(currentScissor.offset.x, currentScissor.offset.y, currentScissor.extend.x, currentScissor.extend.y);
        rx__oglCheckErrors();
        glViewport(currentViewPort.x, currentViewPort.y, currentViewPort.width, currentViewPort.height);
        rx__oglCheckErrors();

#if 0
        if (drawArea->resGroup0.id != 0 && drawArea->resGroup0.id != areaResBlock0.id) {
            // set res group!
            rx_ResGroup* resourceGroup = rx__getResGroup(&ctx->base, drawArea->resGroup0);
            pushIndicies[0] = resourceGroup->offset;
        } else {
            pushIndicies[0] = 0;
        }
#endif

        uint32_t instanceOffset = 0;
        uint32_t instanceCount  = 0;
        rx_RenderPipeline* renderPipeline = NULL;
        uint32_t lastVertexOffset = 0;
        for (uint32_t currentOffset = drawArea->drawOffset, currDrawCount = 0; currDrawCount < drawArea->drawCount; currDrawCount++) {
            uint32_t flags = drawList->commands[currentOffset++];
            bx needsToRebindVertexAttributes = false;
            if (flags != 0) {
                i32 dynamicSlots[2] = {-1, -1};
                u32 dynamicSlotUniformSize[2] = {0, 0};
                GLuint dynamicUniformBufferHandle = 0;
                if ((flags & rx_renderCmd_pipeline) != 0) {
                    uint32_t id = drawList->commands[currentOffset++];
                    if (id != currentPipeline.id) {
                        currentPipeline.id = id;
                        renderPipeline = rx__getRenderPipeline(&ctx->base, currentPipeline);

                        rx__oglCheckErrors();
                        rx__glApplyRenderPipeline(ctx, renderPipeline, glStateCache);
                        rx__oglCheckErrors();
                        
                        //ctx->halApi.cmdSetRenderPipeline(halRenderPass, &renderPipeline->halRenderPipeline);



                        //_sg_gl_apply_pipeline

                        // update depth state

                        // update stencile state

                        // apply color attachment states

                        // update rasterizer state



                        // currentViewPort.minDepth, currentViewPort.maxDepth
                    }
                }

                if ((flags & rx_renderCmd_vertexBuffer0) != 0) {
                    uint32_t id = drawList->commands[currentOffset++];
                    boundVertexBuffers[0].id = id;
                }

                if ((flags & rx_renderCmd_vertexBuffer1) != 0) {
                    uint32_t id = drawList->commands[currentOffset++];
                    boundVertexBuffers[1].id = id;
                }

                if ((flags & rx_renderCmd_vertexBuffer2) != 0) {
                    uint32_t id = drawList->commands[currentOffset++];
                    boundVertexBuffers[2].id = id;
                }

                if ((flags & (rx_renderCmd_vertexBuffer0 | rx_renderCmd_vertexBuffer1 | rx_renderCmd_vertexBuffer2)) != 0) {
                    needsToRebindVertexAttributes = true;
                }

                if ((flags & rx_renderCmd_indexBuffer) != 0) {
                    uint32_t id = drawList->commands[currentOffset++];
                    boundIndexBuffer.id = id;
                    rx_Buffer* idxBuf = rx__getBuffer(&ctx->base, boundIndexBuffer);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxBuf->gl.handle);
                }
                // TODO(PJ): ResGroups

                if ((flags & (rx_renderCmd_resGroup1 | rx_renderCmd_resGroup2 | rx_renderCmd_resGroup3 | rx_renderCmd_dynResGroup0 | rx_renderCmd_dynResGroup1)) != 0) {
                    const u32 resFlags[] = {rx_renderCmd_resGroup1, rx_renderCmd_resGroup2, rx_renderCmd_resGroup3};
                    const u32 resSlots[] = {1, 2, 3};
                    for (u32 idx = 0; idx < countOf(resFlags); idx++) {
                        if ((flags & resFlags[idx]) != 0) {
                            uint32_t id = drawList->commands[currentOffset++];
                            rx_resGroup resGroup = {id};
                            rx_ResGroup* resourceGroup = rx__getResGroup(&ctx->base, resGroup);
                            if (resourceGroup->target.id != 0) {
                                rx__oglCheckErrors();
                                rx_Buffer* targetBuffer = rx__getBuffer(&ctx->base, resourceGroup->target);
                                glBindBufferRange(
                                    GL_UNIFORM_BUFFER,
                                    resSlots[idx],
                                    targetBuffer->gl.handle,
                                    resourceGroup->targetOffset,
                                    resourceGroup->uniformSize
                                );

                                ogl_error error = (ogl_error) glGetError();
                                ASSERT(error == 0);
                                rx__oglCheckErrors();
                            }
                        }
                    }
                    const u32 dynResFlags[] = {rx_renderCmd_dynResGroup0, rx_renderCmd_dynResGroup1};
                    for (u32 idx = 0; idx < countOf(dynResFlags); idx++) {
                        if ((flags & dynResFlags[idx]) != 0) {
                            ASSERT(dynamicSlots[idx] != -1);
                            ASSERT(dynamicSlotUniformSize[idx] > 0);
                            ASSERT(!"Implement me");
                            uint32_t offset = drawList->commands[currentOffset++];
                            
                            glBindBufferRange(
                                GL_UNIFORM_BUFFER,
                                dynResFlags[idx],
                                dynamicUniformBufferHandle,
                                offset,
                                dynamicSlotUniformSize[idx]
                            );
                        }
                    }
                }

                if ((flags & rx_renderCmd_instanceOffset) != 0) {
                    instanceOffset = drawList->commands[currentOffset++];
                }

                if ((flags & rx_renderCmd_instanceCount) != 0) {
                    instanceCount = drawList->commands[currentOffset++];
                }
            }
            rx__oglCheckErrors();

            uint32_t indexOffset  = drawList->commands[currentOffset++];
            uint32_t indexCount   = drawList->commands[currentOffset++];
            ASSERT(indexCount > 0);
            uint32_t vertexOffset = drawList->commands[currentOffset++];
            uint32_t vertexCount  = drawList->commands[currentOffset++];
            

            if (needsToRebindVertexAttributes || vertexOffset != lastVertexOffset) {
                lastVertexOffset = vertexOffset;
                // uint32_t vertexBufferCount = 0;
                // for (; boundVertexBuffers[vertexBufferCount].id != 0 && vertexBufferCount < countOf(boundVertexBuffers); vertexBufferCount++);
                
                GLintptr vbOffset = vertexOffset * sizeOf(f32);
                for (u32 attributeIdx = 0, bufferIdx = u32_max; attributeIdx < ctx->base.limits.maxVertexAttributes; attributeIdx++) {
                    rx__GlVertexAttribute* attribute = &renderPipeline->gl.vertexAttributes[attributeIdx];
                    rx__GlVertexAttribute* cachedAttribute = &glStateCache->vertexAttributes[attributeIdx];
                    if (attribute->vbIndex >= 0) {
                        GLintptr totalOffset = vertexOffset + attribute->offset;
                        if (attribute->vbIndex != cachedAttribute->vbIndex
                            || attribute->divisor    != cachedAttribute->divisor
                            || attribute->divisor    != cachedAttribute->divisor
                            || attribute->stride     != cachedAttribute->stride
                            || attribute->size       != cachedAttribute->size
                            || attribute->normalized != cachedAttribute->normalized
                            || attribute->offset     != cachedAttribute->offset
                            || attribute->type       != cachedAttribute->type
                        ) {

                            if (u32_cast(attribute->vbIndex) != bufferIdx) {
                                bufferIdx = attribute->vbIndex;
                                rx_Buffer* vertBuf = rx__getBuffer(&ctx->base, boundVertexBuffers[bufferIdx]);
                                glBindBuffer(GL_ARRAY_BUFFER, vertBuf->gl.handle);
                                rx__oglCheckErrors();
                            }

                            GLintptr totalOffset = vertexOffset + attribute->offset;
                            glVertexAttribPointer(attributeIdx, attribute->size, attribute->type, attribute->normalized, attribute->stride, (const GLvoid*) totalOffset);

                            rx__oglCheckErrors();
                            glVertexAttribDivisor(attributeIdx, (GLuint)attribute->divisor);
                            rx__oglCheckErrors();
                            glEnableVertexAttribArray(attributeIdx);

                            if (cachedAttribute->vbIndex == -1) {
                                glEnableVertexAttribArray(attributeIdx);
                            }
                            *cachedAttribute = *attribute;
                        }
                    } else {
                        cachedAttribute->vbIndex = -1;
                        glDisableVertexAttribArray(attributeIdx);
                    }
/*
typedef struct rx__GlVertexAttribute {
    i8 vbIndex;        // -1 if attr is not enabled
    i8 divisor;         // -1 if not initialized
    i8 stride;
    i8 size;
    bx normalized : 1;
    i32 offset;
    GLenum type;
} rx__GlVertexAttribute;

*/
                    if (attribute->vbIndex == -1) {
                        glDisableVertexAttribArray(attributeIdx);
                        continue;
                    }

                    if (u32_cast(attribute->vbIndex) != bufferIdx) {
                        bufferIdx = attribute->vbIndex;
                        rx_Buffer* vertBuf = rx__getBuffer(&ctx->base, boundVertexBuffers[bufferIdx]);
                        glBindBuffer(GL_ARRAY_BUFFER, vertBuf->gl.handle);
                        rx__oglCheckErrors();
                    }

                    GLintptr totalOffset = vertexOffset + attribute->offset;
                    glVertexAttribPointer(attributeIdx, attribute->size, attribute->type, attribute->normalized, attribute->stride, (const GLvoid*) totalOffset);

                    rx__oglCheckErrors();
                    glVertexAttribDivisor(attributeIdx, (GLuint)attribute->divisor);
                    rx__oglCheckErrors();
                    glEnableVertexAttribArray(attributeIdx);
                }
            }

            rx__oglCheckErrors();

            // draw
            const GLenum indexType = renderPipeline->gl.indexType;
            const GLenum primitiveTopology = renderPipeline->gl.primitiveTopology;

            u32 baseElement = 0;
            if (indexType != 0) {
                // indexed rendering
                const i32 indexSize = (indexType == GL_UNSIGNED_SHORT) ? 2 : 4;
                const GLvoid* indices = (const GLvoid*)(GLintptr)(baseElement * indexSize + indexOffset);
                if (renderPipeline->gl.useInstancedDraw) {
                    if (instanceOffset == 0) {
                        glDrawElementsInstanced(primitiveTopology, indexCount, indexType, indices, instanceCount);
                    } else {
                        ASSERT(!"Instance offset currently not supported in OpenGL backend.");
                        // glDrawElementsInstancedBaseInstance(primitiveTopology, indexCount, indexType, indices, instanceCount, instanceOffset);
                    }
                } else {
                    glDrawElements(primitiveTopology, indexCount, indexType, indices);
                }
            } else {
                // non-indexed rendering
                if (renderPipeline->gl.useInstancedDraw && instanceCount > 0) {
                    if (instanceOffset == 0) {
                        glDrawArraysInstanced(primitiveTopology, baseElement, indexCount, instanceCount);
                    } else {
                        ASSERT(!"Instance offset currently not supported in OpenGL backend.");
                        // glDrawArraysInstancedBaseInstance(primitiveTopology, baseElement, indexCount, instanceCount, instanceOffset);
                    }
                } else {
                    glDrawArrays(primitiveTopology, baseElement, indexCount);
                    rx__oglCheckErrors();
                }
            }
        }
    }
}

// _sg_image_t** color_images, _sg_image_t** resolve_images, _sg_image_t* ds_image, const sg_pass_desc* desc
LOCAL void rx__glCreateRenderPass(rx_RenderPass* pass) {
    #if 0
    SOKOL_ASSERT(pass && desc);
    SOKOL_ASSERT(color_images && resolve_images);
    _SG_GL_CHECK_ERROR();

    // copy image pointers
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        const sg_pass_attachment_desc* color_desc = &desc->color_attachments[i];
        _SOKOL_UNUSED(color_desc);
        SOKOL_ASSERT(color_desc->image.id != SG_INVALID_ID);
        SOKOL_ASSERT(0 == pass->gl.color_atts[i].image);
        SOKOL_ASSERT(color_images[i] && (color_images[i]->slot.id == color_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_color_format(color_images[i]->cmn.pixel_format));
        pass->gl.color_atts[i].image = color_images[i];

        const sg_pass_attachment_desc* resolve_desc = &desc->resolve_attachments[i];
        if (resolve_desc->image.id != SG_INVALID_ID) {
            SOKOL_ASSERT(0 == pass->gl.resolve_atts[i].image);
            SOKOL_ASSERT(resolve_images[i] && (resolve_images[i]->slot.id == resolve_desc->image.id));
            SOKOL_ASSERT(color_images[i] && (color_images[i]->cmn.pixel_format == resolve_images[i]->cmn.pixel_format));
            pass->gl.resolve_atts[i].image = resolve_images[i];
        }
    }
    SOKOL_ASSERT(0 == pass->gl.ds_att.image);
    const sg_pass_attachment_desc* ds_desc = &desc->depth_stencil_attachment;
    if (ds_desc->image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(ds_image && (ds_image->slot.id == ds_desc->image.id));
        SOKOL_ASSERT(_sg_is_valid_rendertarget_depth_format(ds_image->cmn.pixel_format));
        pass->gl.ds_att.image = ds_image;
    }

    // store current framebuffer binding (restored at end of function)
    GLuint gl_orig_fb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl_orig_fb);

    // create a framebuffer object
    glGenFramebuffers(1, &pass->gl.fb);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->gl.fb);

    // attach color attachments to framebuffer
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        const _sg_image_t* color_img = pass->gl.color_atts[i].image;
        SOKOL_ASSERT(color_img);
        const GLuint gl_msaa_render_buffer = color_img->gl.msaa_render_buffer;
        if (gl_msaa_render_buffer) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0+i), GL_RENDERBUFFER, gl_msaa_render_buffer);
        } else {
            const GLenum gl_att_type = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
            _sg_gl_fb_attach_texture(&pass->gl.color_atts[i], &pass->cmn.color_atts[i], gl_att_type);
        }
    }
    // attach depth-stencil attachement
    if (pass->gl.ds_att.image) {
        const GLenum gl_att = _sg_gl_depth_stencil_attachment_type(&pass->gl.ds_att);
        const _sg_image_t* ds_img = pass->gl.ds_att.image;
        const GLuint gl_msaa_render_buffer = ds_img->gl.msaa_render_buffer;
        if (gl_msaa_render_buffer) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, gl_att, GL_RENDERBUFFER, gl_msaa_render_buffer);
        } else {
            const GLenum gl_att_type = _sg_gl_depth_stencil_attachment_type(&pass->gl.ds_att);
            _sg_gl_fb_attach_texture(&pass->gl.ds_att, &pass->cmn.ds_att, gl_att_type);
        }
    }

    // check if framebuffer is complete
    {
        const GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
            switch (fb_status) {
                case GL_FRAMEBUFFER_UNDEFINED:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNDEFINED);
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT);
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT);
                    break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNSUPPORTED);
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MULTISAMPLE);
                    break;
                default:
                    _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNKNOWN);
                    break;
            }
            return SG_RESOURCESTATE_FAILED;
        }
    }

    // setup color attachments for the framebuffer
    static const GLenum gl_draw_bufs[SG_MAX_COLOR_ATTACHMENTS] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3
    };
    glDrawBuffers(pass->cmn.num_color_atts, gl_draw_bufs);

    // create MSAA resolve framebuffers if necessary
    for (int i = 0; i < pass->cmn.num_color_atts; i++) {
        _sg_gl_attachment_t* gl_resolve_att = &pass->gl.resolve_atts[i];
        if (gl_resolve_att->image) {
            _sg_pass_attachment_t* cmn_resolve_att = &pass->cmn.resolve_atts[i];
            SOKOL_ASSERT(0 == pass->gl.msaa_resolve_framebuffer[i]);
            glGenFramebuffers(1, &pass->gl.msaa_resolve_framebuffer[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, pass->gl.msaa_resolve_framebuffer[i]);
            _sg_gl_fb_attach_texture(gl_resolve_att, cmn_resolve_att, GL_COLOR_ATTACHMENT0);
            // check if framebuffer is complete
            const GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
                switch (fb_status) {
                    case GL_FRAMEBUFFER_UNDEFINED:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNDEFINED);
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT);
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT);
                        break;
                    case GL_FRAMEBUFFER_UNSUPPORTED:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNSUPPORTED);
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MULTISAMPLE);
                        break;
                    default:
                        _SG_ERROR(GL_FRAMEBUFFER_STATUS_UNKNOWN);
                        break;
                }
                return SG_RESOURCESTATE_FAILED;
            }
            // setup color attachments for the framebuffer
            glDrawBuffers(1, &gl_draw_bufs[0]);
        }
    }

    // restore original framebuffer binding
    glBindFramebuffer(GL_FRAMEBUFFER, gl_orig_fb);
    _SG_GL_CHECK_ERROR();
    return SG_RESOURCESTATE_VALID;
#endif
}

LOCAL void rx__glExcuteGraph(rx_Ctx* baseCtx, rx_FrameGraph* frameGraph) {
    ASSERT(baseCtx);
    ASSERT(frameGraph);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;

    // Assume external changes to the gl state so everything needs to set again
    rx_GlStateCache glStateCache = {0};
    rx__glInitStateCache(ctx, &glStateCache);

    rx_SwapChain* swapChain = rx__getSwapChain(baseCtx, baseCtx->defaultSwapChain);
    rx_texture defaultTexture = swapChain->textures[0];
    GLuint defaultFrameBuffer = rx__getTexture(baseCtx, swapChain->textures[0])->gl.handle;

    glBindVertexArray(ctx->vao);
    rx_colorMaskFlags colorMaskFlags[RX_MAX_COLOR_TARGETS] = {0}; // 0
    u32 passCounter = 0;
    for (uint32_t depLvlIdx = 0; depLvlIdx < frameGraph->dependencyLevels.deplevels.count; depLvlIdx++) {
        rx__DependencyLevel* depLvl = frameGraph->dependencyLevels.deplevels.elements + depLvlIdx;

        for (uint32_t depIdx = 0; depIdx < depLvl->depCount; depIdx++) {
            rx_Pass* pass = &baseCtx->passes.elements[depIdx];
            ASSERT(pass->type == rx_passType_render && "Only render passes are supported in OpenGL");

            rx_RenderPass* renderPass = &pass->renderPass;
            // number of color attachments
            const i32 colorAttachmentCount = renderPass->colorAttachmentCount;
            bx isDefaultPass = false;
            i32 width = renderPass->width;
            i32 height = renderPass->height;
            ASSERT(width > 0 && height > 0);
            // execute pass
            for (u32 idx = 0; idx < colorAttachmentCount; idx++) {
                rx_ColorTarget* colorTarget = &renderPass->colorTargets[idx];
                if (colorTarget->target.id == defaultTexture.id) {
                    // default target
                    isDefaultPass = true;

                    #if defined(RX_GLCORE33)
                    glDisable(GL_FRAMEBUFFER_SRGB);
                    #endif

                    #if 0
                    if (_sg.desc.context.gl.default_framebuffer_userdata_cb) {
                        _sg.gl.cur_context->default_framebuffer = _sg.desc.context.gl.default_framebuffer_userdata_cb(_sg.desc.context.gl.user_data);
                    } else if (_sg.desc.context.gl.default_framebuffer_cb) {
                        _sg.gl.cur_context->default_framebuffer = _sg.desc.context.gl.default_framebuffer_cb();
                    }
                    #endif

                    glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);
                } else {
                    // offscreen pass
                    //SOKOL_ASSERT(pass->gl.fb);
                    #if defined(SOKOL_GLCORE33)
                    glEnable(GL_FRAMEBUFFER_SRGB);
                    #endif
                    ASSERT(!"Offscreen rendering not implemented yet!");
                    // Make Framebuffer and so on!

                    // TODO(PJ): Offscreen framebuffer
                    //pass->colorTargets[0]
                    //rx__getTextureView(baseCtx, pass->textureView)->refTexture)->glHandle
                    //GLuint frameBuffer = rx__getTexture(baseCtx, rx__getTextureView(baseCtx, pass->textureView)->refTexture).glHandle;
                    //glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
                }
            }

            // clear color and depth-stencil attachments if needed
            bx clearAnyColor = false;
            for (int idx = 0; idx < colorAttachmentCount; idx++) {
                if (renderPass->colorTargets[idx].loadOp == rx_loadOp_clear) {
                    clearAnyColor = true;
                    break;
                }
            }
            const bool clearDepth   = renderPass->depth.loadOp   == rx_loadOp_clear;
            const bool clearStencil = renderPass->stencil.loadOp == rx_loadOp_clear;

            bx needPipCacheFlush = false;

            if (clearAnyColor) {
                bx needColorMaskFlush = false;
                // NOTE: not a bug to iterate over all possible color attachments
                for (int idx = 0; idx < colorAttachmentCount; idx++) {
                    if (colorMaskFlags[idx] != rx_colorMask_rgba) {
                        needPipCacheFlush = true;
                        needColorMaskFlush = true;
                        colorMaskFlags[idx] = rx_colorMask_rgba;
                    }
                }
                if (needColorMaskFlush) {
                    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                }
            }

            if (clearDepth) {
                if (!glStateCache.depthStencilState.depthWriteEnabled) {
                    needPipCacheFlush = true;
                    glStateCache.depthStencilState.depthWriteEnabled = true;
                    glDepthMask(GL_TRUE);
                }
                if (glStateCache.depthStencilState.depthCompareFunc != rx_compareFunc_always) {
                    needPipCacheFlush = true;
                    glStateCache.depthStencilState.depthCompareFunc = rx_compareFunc_always;
                    glDepthFunc(GL_ALWAYS);
                }
            }

            if (clearStencil) {
                if (glStateCache.depthStencilState.stencilWriteMask != 0xFF) {
                    needPipCacheFlush = true;
                    glStateCache.depthStencilState.stencilWriteMask = 0xFF;
                    glStencilMask(0xFF);
                }
            }

            //if (needPipCacheFlush) {
            //    // we messed with the state cache directly, need to clear cached
            //    // pipeline to force re-evaluation in next sg_apply_pipeline()
            //    stateCache->cur_pipeline = 0;
            //    stateCache->cur_pipeline_id.id = SG_INVALID_ID;
            //}

            for (int i = 0; i < colorAttachmentCount; i++) {
                
                if (renderPass->colorTargets[i].loadOp == rx_loadOp_clear) {
                    glClearBufferfv(GL_COLOR, i, &renderPass->colorTargets[i].clearColor.red);
                }
            }

#if 0
            if ((pass == 0) || (pass->gl.ds_att.image)) {
                if (clearDepth && clearStencil) {
                    glClearBufferfi(GL_DEPTH_STENCIL, 0, action->depth.clear_value, action->stencil.clear_value);
                } else if (clearDepth) {
                    glClearBufferfv(GL_DEPTH, 0, &action->depth.clear_value);
                } else if (clearStencil) {
                    GLint val = (GLint) action->stencil.clear_value;
                    glClearBufferiv(GL_STENCIL, 0, &val);
                }
            }
#endif

            // keep store actions for end-pass
            // for (int i = 0; i < countOf(renderPass->colorTargets); i++) {
            //     _sg.gl.color_store_actions[i] = action->colors[i].store_action;
            // }

            // _sg.gl.depth_store_action = action->depth.store_action;
            // _sg.gl.stencil_store_action = action->stencil.store_action;

            // Execute draw list

            rx__oglCheckErrors();
            rx__glExcuteDrawList(ctx, &glStateCache, renderPass->areas, renderPass->areaCount, renderPass->drawList, width, height);
            rx__oglCheckErrors();

            // Finish pass

            // if (_sg.gl.cur_pass)
            if (!isDefaultPass) {
                ASSERT(!"Implement non default framebuffer!");
                #if 0
                // const _sg_pass_t* pass = _sg.gl.cur_pass;
                //ASSERT(pass->slot.id == _sg.gl.cur_pass_id.id);
                bool fbReadBound = false;
                bool fbDrawBound = false;
                for (int i = 0; i < colorAttachmentCount; i++) {
                    // perform MSAA resolve if needed
                    if (renderPass->gl.msaaResolveFramebuffer[i] != 0) {
                        if (!fbReadBound) {
                            ASSERT(renderPass->gl.frameBuffer);
                            glBindFramebuffer(GL_READ_FRAMEBUFFER, renderPass->gl.frameBuffer);
                            fbReadBound = true;
                        }
                        const int w = renderPass->gl.color_atts[i].image->cmn.width;
                        const int h = renderPass->gl.color_atts[i].image->cmn.height;
                        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pass->gl.msaaResolveFramebuffer[i]);
                        glReadBuffer((GLenum)(GL_COLOR_ATTACHMENT0 + i));
                        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                        fbDrawBound = true;
                    }
                }

                // invalidate framebuffers
                #if defined(RX_GLES3)
                // need to restore framebuffer binding before invalidate if the MSAA resolve had changed the binding
                if (fbDrawBound) {
                    glBindFramebuffer(GL_FRAMEBUFFER, pass->gl.fb);
                }
                GLenum invalidate_atts[SG_MAX_COLOR_ATTACHMENTS + 2] = { 0 };
                int att_index = 0;
                for (int i = 0; i < num_atts; i++) {
                    if (_sg.gl.color_store_actions[i] == SG_STOREACTION_DONTCARE) {
                        invalidate_atts[att_index++] = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
                    }
                }
                if (_sg.gl.depth_store_action == SG_STOREACTION_DONTCARE) {
                    invalidate_atts[att_index++] = GL_DEPTH_ATTACHMENT;
                }
                if (_sg.gl.stencil_store_action == SG_STOREACTION_DONTCARE) {
                    invalidate_atts[att_index++] = GL_STENCIL_ATTACHMENT;
                }
                if (att_index > 0) {
                    glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, att_index, invalidate_atts);
                }
                #else
                unusedVars(fbDrawBound);
                #endif
                #endif
            }


            passCounter += 1;
            rx__oglCheckErrors();


            // TODO(PJ): Handle texture/buffer readbacks
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);

    glFlush();
}

LOCAL void rx__glNextFrame(rx_Ctx* baseCtx) {
    ASSERT(baseCtx);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    //ctx->uniform.streaming.currentOffset = 0;
}

#if 0
LOCAL void rx__glCommit(rx_Ctx* baseCtx) {
    ASSERT(baseCtx);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    //[ctx->nsGlLayer display];
    
    //NSView* view = ctx->nsOpenGlContext.view;
    // 375 668
    u32 width = 375;//ctx->nsGlLayer.bounds.size.width; //view.frame.size.width;
    u32 height = 668;// ctx->nsGlLayer.bounds.size.height; //view.frame.size.height;
    
    
    GLuint defaultFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&defaultFramebuffer);
    
    defaultFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&defaultFramebuffer);
    
    glViewport(0, 0, width, height);

    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    // OpenGL Rendering related code
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glBindVertexArray(ctx->vao);
    glFlush();
    //[ctx->nsGlLayer display];
}
#endif

#endif

#if RX_METAL
typedef struct rx_MtlCtx {
    rx_Ctx base;
} rx_MtlCtx;

#endif

#define rx__valueOrDefault(VAL, DEFAULT) ((VAL) == 0 ? (DEFAULT) : (VAL))
#define rx_callBknFn(FNNAME,...) rx__gl##FNNAME(__VA_ARGS__)

static rx_Ctx* rx__ctx;

void rx_setup(rx_SetupDesc* desc) {
    ASSERT(!rx__ctx);
    ASSERT(desc);

    // set defaults
    rx_SetupDesc descWithDefaults = *desc;
    descWithDefaults.maxRenderShaders = rx_valueOrDefault(descWithDefaults.maxRenderShaders, RX_DEFAULT_MAX_RENDER_SHADERS);
    descWithDefaults.maxBuffers = rx_valueOrDefault(descWithDefaults.maxBuffers, RX_DEFAULT_MAX_BUFFERS);
    descWithDefaults.maxSamplers = rx_valueOrDefault(descWithDefaults.maxBuffers, RX_DEFAULT_MAX_SAMPLERS);
    descWithDefaults.maxTextures = rx_valueOrDefault(descWithDefaults.maxTextures, RX_DEFAULT_MAX_TEXTURES);
    descWithDefaults.maxTextureViews = rx_valueOrDefault(descWithDefaults.maxTextureViews, RX_DEFAULT_MAX_TEXTURE_VIEWS);
    descWithDefaults.maxResGroups = rx_valueOrDefault(descWithDefaults.maxResGroups, RX_DEFAULT_MAX_RES_GROUPS);
    descWithDefaults.maxResGroupLayouts = rx_valueOrDefault(descWithDefaults.maxResGroupLayouts, RX_DEFAULT_MAX_RES_GROUP_LAYOUTS);
    descWithDefaults.maxRenderPipelines = rx_valueOrDefault(descWithDefaults.maxRenderPipelines, RX_DEFAULT_MAX_RENDER_PIPELINES);
    descWithDefaults.maxPasses = rx_valueOrDefault(descWithDefaults.maxPasses, RX_DEFAULT_MAX_PASSES);
    descWithDefaults.maxSwapChains = rx_valueOrDefault(descWithDefaults.maxSwapChains, RX_DEFAULT_MAX_SWAP_CHAINS);
    descWithDefaults.maxPerFramePasses = rx_valueOrDefault(descWithDefaults.maxPerFramePasses, RX_DEFAULT_MAX_SWAP_CHAINS);
    descWithDefaults.maxUniquePasses = rx_valueOrDefault(descWithDefaults.maxUniquePasses, RX_DEFAULT_MAX_UNIQUE_PASSES);
    descWithDefaults.maxPassesPerFrame = rx_valueOrDefault(descWithDefaults.maxPassesPerFrame, RX_DEFAULT_MAX_PASSES_PER_FRAME);
    descWithDefaults.streamingUniformSize = rx_valueOrDefault(descWithDefaults.streamingUniformSize, RX_DEFAULT_MAX_STREAMING_UNIFORM_SIZE);
    descWithDefaults.dynamicUniformSize = rx_valueOrDefault(descWithDefaults.dynamicUniformSize, RX_DEFAULT_MAX_DYNAMIC_UNIFORM_SIZE);

    BaseMemory baseMem = os_getBaseMemory();
    Arena* arena = mem_makeArena(&baseMem, MEGABYTE(6));

    rx_Ctx* ctx = rx__ctx = rx__create(arena, &descWithDefaults);
    ctx->arena = arena;

    rx__poolInit(ctx->arena, &ctx->buffers, descWithDefaults.maxBuffers);
    rx__poolInit(ctx->arena, &ctx->samplers, descWithDefaults.maxSamplers);
    rx__poolInit(ctx->arena, &ctx->textures, descWithDefaults.maxTextures);
    rx__poolInit(ctx->arena, &ctx->renderShaders, descWithDefaults.maxRenderShaders);
    rx__poolInit(ctx->arena, &ctx->renderPipelines, descWithDefaults.maxRenderPipelines);
    rx__poolInit(ctx->arena, &ctx->resGroups, descWithDefaults.maxResGroups);
    rx__poolInit(ctx->arena, &ctx->resGroupLayouts, descWithDefaults.maxResGroupLayouts);
    
    rx__poolInit(ctx->arena, &ctx->swapChains, descWithDefaults.maxSwapChains);

    
    // create default swapchain, should maybe be user driven
    ctx->defaultSwapChain = rx__allocSwapChain(ctx);
    rx_callBknFn(CreateSwapChain, ctx, &ctx->swapChains.elements[ctx->defaultSwapChain.idx]);


    // Frame Related

    // this gets resettet with each frame
    rx_arrInit(ctx->arena, &ctx->passes, descWithDefaults.maxPassesPerFrame);

    u32 maxQueues = 1;

    rx__initFrameGaph(ctx->arena, &ctx->frameGraph, descWithDefaults.maxPassesPerFrame, maxQueues);    

    rx__setupBackend(ctx, &descWithDefaults);
#if 0
    arrMake(ctx->arena, &ctx->buffers, descWithDefaults.maxBuffers);
    arrMake(ctx->arena, &ctx->textures, descWithDefaults.maxTextures);
    arrMake(ctx->arena, &ctx->textureViews, descWithDefaults.maxTextureViews);
    arrMake(ctx->arena, &ctx->passDefs, descWithDefaults.maxPasses);
    arrMake(ctx->arena, &ctx->currentFramePasses, descWithDefaults.maxPerFramePasses);
#endif

}

void rx_shutdown(void) {
    ASSERT(rx__ctx);
    rx_Ctx* ctx = rx__ctx;
    // Destroy arena and free up all alocated memory with it
    mem_destroyArena(ctx->arena);
    rx__ctx = NULL;
}

rx_buffer rx_makeBuffer(const rx_BufferDesc* desc) {
    ASSERT(rx__ctx);
    rx_Ctx* ctx = rx__ctx;

    // fill defaults for bufferDesc
    rx_BufferDesc descWithDefaults = *desc;

    // get free buffer slot
    u32 idx = rx__queuePull(&ctx->buffers.freeIndicies);
    ASSERT(idx != RX__INVALID_INDEX && "No free buffer available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_buffer) {0};
    }

    rx_Buffer* buffer = &ctx->buffers.elements[idx];
    buffer->usage = desc->usage;
    buffer->size = desc->size;
    buffer->gen = maxVal(1, (buffer->gen + 1) % u16_max);

    // call backend specific function
    bx success = rx_callBknFn(MakeBuffer, ctx, buffer, &descWithDefaults);
    ASSERT(success);

    rx_buffer bufferHandle = {
        .idx = idx,
        .gen = buffer->gen
    };

    return bufferHandle;
    #if 0

    ASSERT(rx__ctx);
    ASSERT(desc);
    // get free index
    u32 idx   = rx__queuePull(&rx__ctx->textures.freeIndicies);
    ASSERT(idx != RX__INVALID_INDEX && "No free buffer available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_buffer) {0};
    }
    rx_buffer   handle = {rx_makeHandle(idx, 0)};
    rx__Buffer* buffer = rx__getBuffer(rx__ctx, handle);
    rx__pushGeneration(&buffer->info);
    // create hal buffer
    rx__ctx->halApi.makeBuffer(&buffer->halBuffer, &(rx_HalBufferDesc) {
        .label      = desc->label,
        .device     = &rx__ctx->halDevice,
        .usage      = desc->usage | rx_bufferUsage_copyDst,
        .size       = desc->size,
    });
    // we do alloc in a specific upload phase
    // for legacy APIs if there is data we would upload it right away
    buffer->allocHandle.id = 0;

    // should we really do it like this? maybe we should have more specific buffers
    // for specific usecases like the legacy APIs
    rx_allocForBuffer(&buffer->halBuffer, &rx__ctx->bufferMem);
    #endif
}

void rx_updateBuffer(rx_buffer buffer, mms offset, rx_Range range) {
    ASSERT(rx__ctx);
    ASSERT(range.content && "upload ptr is NULL");
    ASSERT(range.size > 0 && "upload size is zero");
    rx_Ctx* ctx = rx__ctx;
    rx_Buffer* bufferContent = &ctx->buffers.elements[buffer.idx];
    ASSERT(bufferContent->gen == buffer.gen);
    ASSERT(bufferContent->size > (offset) && "upload offset is bigger then the buffer");
    ASSERT(bufferContent->size >= (offset + range.size) && "upload size does not fit in the buffer");
    rx_callBknFn(UpdateBuffer, ctx, bufferContent, offset, range);
}

LOCAL rx_SamplerDesc rx__samplerDescDefaults(const rx_SamplerDesc* desc) {
    rx_SamplerDesc outDesc;
    outDesc.addressModeU      = rx__valueOrDefault(outDesc.addressModeU, rx_addressMode_repeat);
    outDesc.addressModeV      = rx__valueOrDefault(outDesc.addressModeV, rx_addressMode_repeat);
    outDesc.addressModeW      = rx__valueOrDefault(outDesc.addressModeW, rx_addressMode_repeat);
    outDesc.magFilter         = rx__valueOrDefault(outDesc.magFilter, rx_filterMode_nearest);
    outDesc.minFilter         = rx__valueOrDefault(outDesc.minFilter, rx_filterMode_nearest);
    outDesc.mipmapMode        = rx__valueOrDefault(outDesc.mipmapMode, rx_mipmapMode_linear);
    //outDesc.lodMinClamp       = rx__valueOrDefault(outDesc.lodMinClamp, float);
    outDesc.lodMaxClamp       = rx__valueOrDefault(outDesc.lodMaxClamp, f32_max);
    outDesc.compare           = rx__valueOrDefault(outDesc.compare, rx_compareFunc_always);
    outDesc.maxAnisotropy     = rx__valueOrDefault(outDesc.maxAnisotropy, 1);
    return outDesc;
}

rx_sampler rx_makeSampler(const rx_SamplerDesc* desc) {
    ASSERT(rx__ctx && desc);
    rx_Ctx* ctx = rx__ctx;

    rx_SamplerDesc descWithDefaults = rx__samplerDescDefaults(desc);

    // get free sampler slot
    u32 idx = rx__queuePull(&ctx->samplers.freeIndicies);
    ASSERT(idx != RX__INVALID_INDEX && "No free sampler available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_sampler) {0};
    }

    rx_Sampler* sampler = &ctx->samplers.elements[idx];
    sampler->gen = maxVal(1, (sampler->gen + 1) % u16_max);
    rx_callBknFn(MakeSampler, ctx, sampler, &descWithDefaults);

    rx_sampler handle = {0};
    handle.idx = idx;
    handle.gen = sampler->gen;

    return handle;
#if 0
    typedef struct rx_SamplerDesc {
        Str8           label;
        rx_addressMode addressModeU;
        rx_addressMode addressModeV;
        rx_addressMode addressModeW;
        rx_filterMode  magFilter;
        rx_filterMode  minFilter;
        rx_mipmapMode  mipmapMode;
        f32            lodMinClamp;
        f32            lodMaxClamp;
        rx_CompareFunc compare;
        u16            maxAnisotropy;
    } rx_SamplerDesc;
#endif
}

rx_renderShader rx_makeRenderShader(const rx_RenderShaderDesc* desc) {
    ASSERT(rx__ctx && desc);
    rx_Ctx* ctx = rx__ctx;
    ASSERT(desc->vs.source.size > 0 || desc->vs.byteCode.size > 0);
    ASSERT(desc->fs.source.size > 0 || desc->fs.byteCode.size > 0);


    // get free sampler slot
    u32 idx = rx__queuePull(&ctx->renderShaders.freeIndicies);
    ASSERT(idx != RX__INVALID_INDEX && "No free sampler available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_renderShader) {0};
    }

    rx_RenderShader* renderShader = &ctx->renderShaders.elements[idx];
    renderShader->gen = maxVal(1, (renderShader->gen + 1) % u16_max);

    bx success = rx_callBknFn(MakeRenderShader, ctx, renderShader, desc);
    ASSERT(success);

    if (!success) {
        rx_renderShader handle = {0};
        return handle;
    }

    rx_renderShader handle = {0};
    handle.idx = idx;
    handle.gen = renderShader->gen;

    return handle;
}

LOCAL rx_ResGroupLayoutDesc rx__resGroupLayoutDescDefaults(const rx_ResGroupLayoutDesc* desc) {
    rx_ResGroupLayoutDesc descWithDefaults = *desc;

    u32 computedOffset[countOf(descWithDefaults.resources)] = {0};
    bx useComputedSlotPoints = true;
    for (u32 idx = 0; idx < countOf(descWithDefaults.resources); idx++) {
        // only if all offsets are zero we compute offesets automatically
        if (descWithDefaults.resources[idx].slot != 0) {
            useComputedSlotPoints = false;
            break;
        }
    }

    if (useComputedSlotPoints) {
        for (u32 idx = 0; idx < countOf(descWithDefaults.resources); idx++) {
            descWithDefaults.resources[idx].slot = idx;
        }
    }

    return descWithDefaults;
}

rx_resGroupLayout rx_makeResGroupLayout(rx_ResGroupLayoutDesc* desc) {
    ASSERT(rx__ctx);
    ASSERT(desc);

    rx_Ctx* ctx = rx__ctx;
    rx_resGroupLayout resGroupLayoutHandle = rx__allocResGroupLayout(ctx);

    if (resGroupLayoutHandle.id == 0) {
        return resGroupLayoutHandle;
    }

    rx_ResGroupLayout* resGroupLayout = rx__getResGroupLayout(ctx, resGroupLayoutHandle);
    resGroupLayout->uniformSize = desc->uniformSize;

    rx_ResGroupLayoutDesc descWithDefaults = rx__resGroupLayoutDescDefaults(desc);

    rx_callBknFn(MakeResGroupLayout, ctx, resGroupLayout, &descWithDefaults);

    return resGroupLayoutHandle;
}

rx_resGroup rx_makeResGroup(rx_ResGroupDesc* desc) {
    ASSERT(rx__ctx);
    ASSERT(desc);
    ASSERT(desc->layout.id != 0);

    rx_Ctx* ctx = rx__ctx;
    rx_resGroup resGroupHandle = rx__allocResGroup(ctx);

    if (resGroupHandle.id == 0) {
        return resGroupHandle;
    }

    //rx_ResGroupDesc descWithDefaults = rx__resGroupDescDefaults(desc);

    rx_ResGroup* resGroup = rx__getResGroup(ctx, resGroupHandle);
    resGroup->layout = desc->layout;

    rx_callBknFn(MakeResGroup, ctx, resGroup, desc);

    return resGroupHandle;
}

void rx_updateResGroup(rx_resGroup handle, rx_ResGroupUpdateDesc* desc) {
    ASSERT(rx__ctx);
    ASSERT(desc);
    ASSERT(handle.id);
    rx_Ctx* ctx = rx__ctx;

    rx_ResGroup* resGroup = rx__getResGroup(ctx, handle);

    rx_callBknFn(UpdateResGroup, ctx, resGroup, desc, false);
}

flags64 rx_getResGroupPassDepFlags(rx_resGroup resGroupHandle) {
    ASSERT(rx__ctx && resGroupHandle.id);
    rx_Ctx* ctx = rx__ctx;
    if (!resGroupHandle.hasPassDep) {
        return 0;
    }
    rx_ResGroup* resGroup = rx__getResGroup(ctx, resGroupHandle);
    ASSERT(resGroup->lastUpdateFrameIdx == ctx->frameIdx);
    
    return resGroup->passDepFlags;
}


LOCAL rx_RenderPipelineDesc rx__renderPipelineDescDefaults(const rx_RenderPipelineDesc* desc) {
    rx_RenderPipelineDesc descWithDefaults = *desc;

    descWithDefaults.primitiveTopology = rx__valueOrDefault(descWithDefaults.primitiveTopology, rx_primitiveTopology_triangleList);
    
    descWithDefaults.sampleMask = descWithDefaults.sampleMask == 0 ? 0xFFFFFFFF : 0;
    
    descWithDefaults.colorTargetCount = clampVal(desc->colorTargetCount, 1, countOf(desc->colorTargets));
    for (uint32_t idx = 0; idx < descWithDefaults.colorTargetCount; idx++) {
        const rx_ColorTargetState* colorTarget = desc->colorTargets + idx;
        rx_ColorTargetState* outColorTarget = descWithDefaults.colorTargets + idx;
        if (colorTarget->format == rx_textureFormat__invalid) {
            break;
#if 0
            if (idx > 0) {
            }
            // one color target is the default settings
            outColorTarget->format = rx__ctx->defaultColorFormat;
#endif
        }
        outColorTarget->blend.srcFactorRgb = rx__valueOrDefault(colorTarget->blend.srcFactorRgb, rx_blendFactor_srcAlpha);
        outColorTarget->blend.dstFactorRgb = rx__valueOrDefault(colorTarget->blend.dstFactorRgb, rx_blendFactor_oneMinusSrcAlpha);
        outColorTarget->blend.opRgb = rx__valueOrDefault(colorTarget->blend.opRgb, rx_blendOp_add);
        outColorTarget->blend.srcFactorAlpha = rx__valueOrDefault(colorTarget->blend.srcFactorAlpha,  rx_blendFactor_one);
        outColorTarget->blend.dstFactorAlpha = rx__valueOrDefault(colorTarget->blend.dstFactorAlpha,  rx_blendFactor_zero);
        outColorTarget->blend.opAlpha = rx__valueOrDefault(colorTarget->blend.opAlpha, rx_blendOp_add);
        if (colorTarget->blend.colorWriteMask == rx_colorMask_none) {
            outColorTarget->blend.colorWriteMask = 0;
        } else {
            outColorTarget->blend.colorWriteMask = (uint8_t) rx__valueOrDefault(colorTarget->blend.colorWriteMask, rx_colorMask_rgba);
        }
        //colorTarget->blend.colorAttachmentCount = _sg_def(def.blend.color_attachment_count, 1);
        //colorTarget->blend.color_format = rx__valueOrDefault(def.blend.color_format, _sg.desc.context.color_format);
        //colorTarget->blend.depth_format = rx__valueOrDefault(def.blend.depth_format, _sg.desc.context.depth_format);
    }

    descWithDefaults.depthStencil.stencilFront.failOp = rx__valueOrDefault(descWithDefaults.depthStencil.stencilFront.failOp, rx_stencilOp_keep);
    descWithDefaults.depthStencil.stencilFront.depthFailOp = rx__valueOrDefault(descWithDefaults.depthStencil.stencilFront.depthFailOp, rx_stencilOp_keep);
    descWithDefaults.depthStencil.stencilFront.passOp = rx__valueOrDefault(descWithDefaults.depthStencil.stencilFront.passOp, rx_stencilOp_keep);
    descWithDefaults.depthStencil.stencilFront.compareFunc = rx__valueOrDefault(descWithDefaults.depthStencil.stencilFront.compareFunc, rx_compareFunc_always);
    descWithDefaults.depthStencil.stencilBack.failOp = rx__valueOrDefault(descWithDefaults.depthStencil.stencilBack.failOp, rx_stencilOp_keep);
    descWithDefaults.depthStencil.stencilBack.depthFailOp = rx__valueOrDefault(descWithDefaults.depthStencil.stencilBack.depthFailOp, rx_stencilOp_keep);
    descWithDefaults.depthStencil.stencilBack.passOp = rx__valueOrDefault(descWithDefaults.depthStencil.stencilBack.passOp, rx_stencilOp_keep);
    descWithDefaults.depthStencil.stencilBack.compareFunc = rx__valueOrDefault(descWithDefaults.depthStencil.stencilBack.compareFunc, rx_compareFunc_always);
    descWithDefaults.depthStencil.depthCompareFunc = rx__valueOrDefault(descWithDefaults.depthStencil.depthCompareFunc, rx_compareFunc_always);

    descWithDefaults.rasterizer.cullMode = rx__valueOrDefault(descWithDefaults.rasterizer.cullMode, rx_cullMode_none);
    descWithDefaults.rasterizer.faceWinding = rx__valueOrDefault(descWithDefaults.rasterizer.faceWinding, rx_faceWinding_clockwise);
    descWithDefaults.sampleCount = rx__valueOrDefault(descWithDefaults.sampleCount, rx_sampleCount_1bit);

    // vertex attribute offsets
    u32 computedOffset[countOf(desc->layout.buffers)] = {0};
    bx useComputedOffset = true;
    for (u32 idx = 0; idx < countOf(descWithDefaults.layout.attrs); idx++) {
        // only if all offsets are zero we compute offesets automatically
        if (descWithDefaults.layout.attrs[idx].offset != 0) {
            useComputedOffset = false;
            break;
        }
    }

    for (u32 idx = 0; idx < countOf(descWithDefaults.layout.attrs); idx++) {
        rx_VertexAttrDesc* attrDesc = descWithDefaults.layout.attrs + idx;
        if (attrDesc->format == rx_vertexFormat__invalid) {
            continue;
        }
        ASSERT((attrDesc->bufferIndex >= 0) && (attrDesc->bufferIndex < countOf(desc->layout.buffers)));

        if (useComputedOffset) {
            descWithDefaults.layout.attrs[idx].offset = computedOffset[attrDesc->bufferIndex];
        }
        computedOffset[attrDesc->bufferIndex] += rx__vertexFormatBytesize(attrDesc->format);
    }

    // compute vertex strides
    for (uint32_t idx = 0; idx < countOf(desc->layout.buffers); idx++) {
        rx_BufferLayoutDesc* bufferDesc = descWithDefaults.layout.buffers + idx;
        if (bufferDesc->stride == 0) {
            bufferDesc->stride = computedOffset[idx];
        }
    }

    return descWithDefaults;
}

rx_renderPipeline rx_makeRenderPipeline(rx_RenderPipelineDesc* desc) {
    ASSERT(rx__ctx);
    ASSERT(desc);
    rx_Ctx* ctx = rx__ctx;

    // get free sampler slot
    u32 idx = rx__queuePull(&ctx->renderPipelines.freeIndicies);
    ASSERT(idx != RX__INVALID_INDEX && "No free renderPipeline available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_renderPipeline) {0};
    }

    rx_RenderPipelineDesc descWithDefaults = rx__renderPipelineDescDefaults(desc);

    rx_RenderPipeline* renderPipeline = &ctx->renderPipelines.elements[idx];
    renderPipeline->gen = maxVal(1, (renderPipeline->gen + 1) % u16_max);

    
    bx success = rx_callBknFn(MakeRenderPipeline, ctx, renderPipeline, &descWithDefaults);
    ASSERT(success);

    if (!success) {
        rx_renderPipeline handle = {0};
        return handle;
    }

    rx_renderPipeline handle = {0};
    handle.idx = idx;
    handle.gen = renderPipeline->gen;

    return handle;
}

#define rx__passFrameGen(CTX) (maxVal(1, (CTX->frameIdx + 1) % u16_max))
rx_renderPass rx_makeRenderPass(rx_RenderPassDesc* renderPassDesc, rx_RenderPassResult* resultTextureViews) {
    ASSERT(rx__ctx && renderPassDesc);
    rx_Ctx* ctx = rx__ctx;
    ASSERT((ctx->passes.count + 1) <= ctx->passes.capacity && "Exceeded maximum passes per frame");

    u32 passIdx = ctx->passes.count++;
    rx_Pass* pass = &ctx->passes.elements[passIdx];
    mem_structSetZero(pass);
    pass->type = rx_passType_render;
    rx_RenderPass* renderPass = &pass->renderPass;

    flags64 readDependencies = 0;
    flags64 writeDependencies = 0;

    // Pass draws to exernal canvas so we don't have control over it
    //bx isDefaultPass = renderPassDesc->colorTargets.target.idx == textureView.idx;
    u32 colorAttachmentCount = 0;
    int width  = renderPassDesc->width;
    int height = renderPassDesc->height;
    for (u32 idx = 0; idx < countOf(renderPassDesc->colorTargets); idx++) {
        rx_RenderPassColorTargetDesc* colorTargetDesc = &renderPassDesc->colorTargets[idx];
        rx_ColorTarget* colorTarget = &renderPass->colorTargets[idx];
        if (colorTargetDesc->target.id == 0) {
            colorTarget->target.id = 0;
            continue;
        }

        if (colorTargetDesc->target.passIdx > 0) {
            flags64 passIdxFlag = 1 <<(colorTargetDesc->target.passIdx - 1);
            readDependencies  |= passIdxFlag;
            writeDependencies |= passIdxFlag;
        }
        colorTarget->target = colorTargetDesc->target;
        colorTarget->loadOp = rx__valueOrDefault(colorTargetDesc->loadOp, rx_loadOp_clear);
        colorTarget->storeOp = rx__valueOrDefault(colorTargetDesc->storeOp, rx_storeOp_store);
        colorTarget->clearColor = colorTargetDesc->clearColor;
        colorAttachmentCount += 1;
    }

    ASSERT(width != 0 && height != 0 && "Implement ofscreen pass!");
    renderPass->colorAttachmentCount = colorAttachmentCount;
    renderPass->width = width;
    renderPass->height = height;

    ctx->frameGraph.dependencies.elements[passIdx].passReadDeps = readDependencies;
    ctx->frameGraph.dependencies.elements[passIdx].passWriteDeps = writeDependencies;

    u64 passDefHash = 0;

    // build pass def hash
    // can be used to check of used pipelines are compatible with the pass
    // Vulkan can use it to find fitting pass def



    // write return texture view handles that contain the current pass
    if (resultTextureViews) {
        mem_structSetZero(resultTextureViews);
        u32 fromPass = passIdx + 1;
        for (u32 idx = 0; idx < countOf(renderPass->colorTargets); idx++) {
            rx_ColorTarget* colorTarget = &renderPass->colorTargets[idx];
            if (colorTarget->target.id == 0) {
                colorTarget->target.id = 0;
                continue;
            }
            rx_texture target = colorTarget->target;
            target.passIdx = fromPass;

            colorTarget->target = target;
        }
    }

    //rx_callBknFn(MakeRenderPass, ctx, renderPass, &descWithDefaults);

    rx_renderPass handle = {0};
    handle.idx = passIdx;
    handle.gen = rx__passFrameGen(ctx);

    return handle;
}

rx_texture rx_getCurrentSwapTexture(void) {
    ASSERT(rx__ctx);
    rx_Ctx* ctx = rx__ctx;
    rx_SwapChain* swapChain = &ctx->swapChains.elements[ctx->defaultSwapChain.idx];
    rx_texture texture = rx_callBknFn(GetCurrentSwapTexture, ctx, swapChain);
    return texture;
}

void rx_setRenderPassDrawList(rx_renderPass renderPass, rx_DrawArea* arenas, u32 areaCount, rx_DrawList* drawList) {
    ASSERT(rx__ctx);
    rx_Ctx* ctx = rx__ctx;
    ASSERT(renderPass.id);
    ASSERT(ctx->passes.count > renderPass.idx);
    ASSERT(rx__passFrameGen(ctx) == renderPass.gen);
    rx_Pass* pass = &ctx->passes.elements[renderPass.idx];
    ASSERT(ctx->passes.elements[renderPass.idx].type == rx_passType_render);
    ASSERT(!pass->renderPass.drawList && "draw List has been set before");
    pass->renderPass.areas = arenas;
    pass->renderPass.areaCount = areaCount;
    pass->renderPass.drawList = drawList;

    ctx->frameGraph.dependencies.elements[renderPass.idx].passReadDeps |= drawList->passIdxDepFlags;
}

LOCAL void rx__excuteGraph(rx_Ctx* ctx, rx_FrameGraph* frameGraph) {
    ASSERT(ctx);
    ASSERT(frameGraph);

}

LOCAL void rx__buildAndPresent(rx_Ctx* ctx) {
    ASSERT(ctx);
    if (ctx->passes.count == 0) {
        // no pass set, nothing to do for the gpu this frame
        return;
    }
    rx__passGraphBuild(&ctx->frameGraph, ctx->passes.count);

    rx_callBknFn(ExcuteGraph, ctx, &ctx->frameGraph);
}

LOCAL void rx__resetFrameData(rx_Ctx* ctx) {
    ASSERT(ctx);
    ctx->passes.count = 0;
}

LOCAL void rx__nextFrame(rx_Ctx* ctx) {
    ctx->frameIdx += 1;
    rx__resetFrameData(ctx);
    rx_callBknFn(NextFrame, ctx);
}

API void rx_commit(void) {
    ASSERT(rx__ctx);
    rx_Ctx* ctx = rx__ctx;
    rx__buildAndPresent(ctx);
    rx__nextFrame(ctx);
    ctx->frameIdx += 1;
}


