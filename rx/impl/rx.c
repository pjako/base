
#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"

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

#else
#error "Undefined platform"
#endif
#endif

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
        } gl;
#endif
    };
    u32                 gen : 16;
    u64                 alignment;
    u64                 allocatedSize;
    u64                 size;
    rx_bufferUsageFlags usage;
} rx_Buffer;

typedef struct rx_Texture {
    union {
#if RX_OGL
        struct {
            GLuint handle;
        } gl;
#endif
    };
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

typedef struct rx_Sampler {
    union {
#if RX_OGL
        GLuint handle;
#endif
    };
    u32 gen : 16;
} rx_Sampler;

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

#define rx__poolDef(TYPE) struct {TYPE* elements; u32 capacity; rx_IndexQueue freeIndicies;}
#define rx__poolInit(ARENA, POOL, COUNT) (POOL)->elements = mem_arenaPush((ARENA), sizeOf((POOL)->elements[0]) * (COUNT) ); (POOL)->capacity = (COUNT); rx__queueInit((ARENA), &(POOL)->freeIndicies, (COUNT), (COUNT))
// rx_poolInit(ctx->.arena, &ctx->buffers, descWithDefaults.maxBuffers);

typedef struct rx_Ctx {
    rx_api api;
    Arena* arena;
    rx__poolDef(rx_Buffer) buffers;
    //rx__poolDef(rx_Texture) textures;
} rx_Ctx;


#define rx_valueOrDefault(VAR, DEFAULT) (VAR) != 0 ? (VAR) : (DEFAULT)

// Shared code

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



#if RX_VULKAN

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
} rx_VulkanCtx;

#endif

#if RX_OGL
#define rx__oglCheckErrors() { ASSERT(glGetError() == GL_NO_ERROR); }


#if OS_OSX
#define RXGLLayer NSOpenGLLayer
#else
#define RXGLLayer CAEAGLLayer
#endif
typedef struct rx_OpenGlCtx {
    rx_Ctx base;
#if OS_WINDOWS
    os_Dl* openGlLib;
#endif
#if OS_APPLE
    RXGLLayer* nsGlLayer;
#endif
#if OS_OSX
    NSOpenGLContext* nsOpenGlContext;
#endif
    GLuint vao;
    GLuint defaultFramebuffer;
    struct {
        GLuint vertexBuffer;
        GLuint indexBuffer;
        GLuint storedVertexBuffer;
        GLuint storedIndexBuffer;
    } cache;
} rx_OpenGlCtx;

#if 0
LOCAL rx_renderPipeline rx__makeRenderPipeline(rx_RenderPipelineDesc* desc) {

}
#endif


#if defined(RX_GLCORE33)
LOCAL void rx__initCapsGlCore33(rx_OpenGlCtx* ctx) {
    #if 0
    _sg.backend = SG_BACKEND_GLCORE33;

    _sg.features.origin_top_left = false;
    _sg.features.image_clamp_to_border = true;
    _sg.features.mrt_independent_blend_state = false;
    _sg.features.mrt_independent_write_mask = true;

    // scan extensions
    bool has_s3tc = false;  // BC1..BC3
    bool has_rgtc = false;  // BC4 and BC5
    bool has_bptc = false;  // BC6H and BC7
    bool has_pvrtc = false;
    bool has_etc2 = false;
    GLint num_ext = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
    for (int i = 0; i < num_ext; i++) {
        const char* ext = (const char*) glGetStringi(GL_EXTENSIONS, (GLuint)i);
        if (ext) {
            if (strstr(ext, "_texture_compression_s3tc")) {
                has_s3tc = true;
            } else if (strstr(ext, "_texture_compression_rgtc")) {
                has_rgtc = true;
            } else if (strstr(ext, "_texture_compression_bptc")) {
                has_bptc = true;
            } else if (strstr(ext, "_texture_compression_pvrtc")) {
                has_pvrtc = true;
            } else if (strstr(ext, "_ES3_compatibility")) {
                has_etc2 = true;
            } else if (strstr(ext, "_texture_filter_anisotropic")) {
                _sg.gl.ext_anisotropic = true;
            }
        }
    }

    // limits
    _sg_gl_init_limits();

    // pixel formats
    const bool has_bgra = false;    // not a bug
    const bool has_colorbuffer_float = true;
    const bool has_colorbuffer_half_float = true;
    const bool has_texture_float_linear = true; // FIXME???
    const bool has_float_blend = true;
    _sg_gl_init_pixelformats(has_bgra);
    _sg_gl_init_pixelformats_float(has_colorbuffer_float, has_texture_float_linear, has_float_blend);
    _sg_gl_init_pixelformats_half_float(has_colorbuffer_half_float);
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
#endif


LOCAL void rx__setupBackend(rx_Ctx* baseCtx, const rx_SetupDesc* desc) {
    ASSERT(baseCtx);
    ASSERT(desc);
    //ASSERT(desc->context.gl.default_framebuffer_cb == 0 || desc->context.gl.default_framebuffer_userdata_cb == 0);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;

    // assumes that _sg.gl is already zero-initialized
    //_sg.gl.valid = true;

    #if defined(_SOKOL_USE_WIN32_GL_LOADER)
    _sg_gl_load_opengl();
    #endif


    // clear initial GL error state
    #if defined(SOKOL_DEBUG)
        while (glGetError() != GL_NO_ERROR);
    #endif
    #if defined(RX_GLCORE33)
        rx__initCapsGlCore33(ctx);
    #elif defined(SOKOL_GLES3)
        _sg_gl_init_caps_gles3(ctx);
    #endif
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

#if OS_OSX
    {
        NSOpenGLPixelFormatAttribute attrs[32];
        int i = 0;
        attrs[i++] = NSOpenGLPFAAccelerated;
        attrs[i++] = NSOpenGLPFADoubleBuffer;
        attrs[i++] = NSOpenGLPFAOpenGLProfile;
        const int glVersion = 41; //_sapp.desc.gl_major_version * 10 + _sapp.desc.gl_minor_version;
        switch(glVersion) {
            case 10: attrs[i++] = NSOpenGLProfileVersionLegacy;  break;
            case 32: attrs[i++] = NSOpenGLProfileVersion3_2Core; break;
            case 41: attrs[i++] = NSOpenGLProfileVersion4_1Core; break;
            default: ASSERT(!"MACOS_INVALID_NSOPENGL_PROFILE");//_SAPP_PANIC(MACOS_INVALID_NSOPENGL_PROFILE);
        }
        attrs[i++] = NSOpenGLPFAColorSize; attrs[i++] = 24;
        attrs[i++] = NSOpenGLPFAAlphaSize; attrs[i++] = 8;
        attrs[i++] = NSOpenGLPFADepthSize; attrs[i++] = 24;
        attrs[i++] = NSOpenGLPFAStencilSize; attrs[i++] = 8;
        if (desc->sampleCount > 1) {
            attrs[i++] = NSOpenGLPFAMultisample;
            attrs[i++] = NSOpenGLPFASampleBuffers;
            attrs[i++] = 1;
            attrs[i++] = NSOpenGLPFASamples;
            attrs[i++] = (NSOpenGLPixelFormatAttribute) desc->sampleCount;
        }
        else {
            attrs[i++] = NSOpenGLPFASampleBuffers; attrs[i++] = 0;
        }
        attrs[i++] = 0;
        NSOpenGLPixelFormat* glpixelformatObj = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        
        ctx->nsOpenGlContext = [[NSOpenGLContext alloc] initWithFormat:glpixelformatObj shareContext: nil];
        ctx->nsGlLayer.openGLContext = ctx->nsOpenGlContext;
        [ctx->nsOpenGlContext makeCurrentContext];
        NSView* view = ctx->nsGlLayer.view;
        [ctx->nsOpenGlContext setView:view];
        [ctx->nsOpenGlContext flushBuffer];

        RX_OBJC_RELESE(glpixelformatObj);
    }
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
    ASSERT((target == GL_ARRAY_BUFFER) || (target == GL_ELEMENT_ARRAY_BUFFER));
    if (target == GL_ARRAY_BUFFER) {
        if (ctx->cache.vertexBuffer != buffer) {
            ctx->cache.vertexBuffer = buffer;
            glBindBuffer(target, buffer);
            //_sg_stats_add(gl.num_bind_buffer, 1);
        }
    } else {
        if (ctx->cache.indexBuffer != buffer) {
            ctx->cache.indexBuffer = buffer;
            glBindBuffer(target, buffer);
            //_sg_stats_add(gl.num_bind_buffer, 1);
        }
    }
}

LOCAL void rx__glCacheStoreBufferBinding(rx_OpenGlCtx* ctx, GLenum target) {
    if (target == GL_ARRAY_BUFFER) {
        ctx->cache.storedVertexBuffer = ctx->cache.vertexBuffer;
    } else {
        ctx->cache.storedIndexBuffer = ctx->cache.indexBuffer;
    }
}

LOCAL void rx__glCacheRestoreBufferBinding(rx_OpenGlCtx* ctx, GLenum target) {
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
    rx__oglCheckErrors();

    rx__glCacheStoreBufferBinding(ctx, glTarget);
    rx__glCacheBindBuffer(ctx, glTarget, glBuffer);
    glBufferData(glTarget, desc->size, 0, glUsage);
    rx__oglCheckErrors();
    //_sg_gl_cache_restore_buffer_binding(gl_target);
    //rx__oglCheckErrors();

    return true;
}

LOCAL void rx__glUpdateBuffer(rx_Ctx* baseCtx, rx_Buffer* buffer, mms offset, rx_Range range) {
    ASSERT(baseCtx && buffer);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    GLenum glTarget = rx__glBufferTarget(buffer->usage);
    rx__glCacheStoreBufferBinding(ctx, glTarget);
    rx__glCacheBindBuffer(ctx, glTarget, buffer->gl.handle);
    glBufferSubData(glTarget, offset, (GLsizeiptr)range.size, range.ptr);
    rx__oglCheckErrors();
    rx__glCacheRestoreBufferBinding(ctx, glTarget);
    rx__oglCheckErrors();
}

#endif

#if RX_METAL
typedef struct rx_MtlCtx {
    rx_Ctx base;
} rx_MtlCtx;

#endif

#define rx_callBknFn(FNNAME,...) rx__gl##FNNAME(__VA_ARGS__)

static rx_Ctx* rx__ctx;

void rx_setup(rx_SetupDesc* desc) {
    ASSERT(!rx__ctx);
    ASSERT(desc);

    // set defaults
    rx_SetupDesc descWithDefaults = *desc;

    descWithDefaults.maxBuffers = rx_valueOrDefault(descWithDefaults.maxBuffers, RX_DEFAULT_MAX_BUFFERS);
    descWithDefaults.maxTextures = rx_valueOrDefault(descWithDefaults.maxTextures, RX_DEFAULT_MAX_TEXTURES);
    descWithDefaults.maxTextureViews = rx_valueOrDefault(descWithDefaults.maxTextureViews, RX_DEFAULT_MAX_TEXTURE_VIEWS);
    descWithDefaults.maxPasses = rx_valueOrDefault(descWithDefaults.maxPasses, RX_DEFAULT_MAX_PASSES);
    descWithDefaults.maxPerFramePasses = rx_valueOrDefault(descWithDefaults.maxPerFramePasses, RX_DEFAULT_MAX_PER_FRAME_PASSES);

    BaseMemory baseMem = os_getBaseMemory();
    Arena* arena = mem_makeArena(&baseMem, MEGABYTE(6));

    rx_Ctx* ctx = rx__ctx = rx__create(arena, &descWithDefaults);
    rx__setupBackend(ctx, &descWithDefaults);
    ctx->arena = arena;

    rx__poolInit(ctx->arena, &ctx->buffers, descWithDefaults.maxBuffers);

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
    rx_callBknFn(DiscardContext, ctx);
    // Destroy arena and free up all alocated memory with it
    mem_destroyArena(ctx->arena);
    rx__ctx = NULL;
}

API rx_buffer rx_makeBuffer(const rx_BufferDesc* desc) {
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
    buffer->gen = maxVal(1, buffer->gen + 1);

    // call backend specific function
    bx success = rx_callBknFn(MakeBuffer, ctx, buffer, &descWithDefaults);
    ASSERT(success);

    rx_buffer bufferHandle = {
        .idx = idx,
        .gen = buffer->gen
    };

    return bufferHandle;
    #if 0

    RX_ASSERT(rx__ctx);
    RX_ASSERT(desc);
    // get free index
    u32 idx   = rx__queuePull(&rx__ctx->textures.freeIndicies);
    RX_ASSERT(idx != RX__INVALID_INDEX && "No free buffer available");
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

API void rx_updateBuffer(rx_buffer buffer, mms offset, rx_Range range) {
    ASSERT(rx__ctx);
    ASSERT(range.ptr && "upload ptr is NULL");
    ASSERT(range.size > 0 && "upload size is zero");
    rx_Ctx* ctx = rx__ctx;
    rx_Buffer* bufferContent = &ctx->buffers.elements[buffer.idx];
    ASSERT(bufferContent->gen == buffer.gen);
    ASSERT(bufferContent->size > (offset) && "upload offset is bigger then the buffer");
    ASSERT(bufferContent->size >= (offset + range.size) && "upload size does not fit in the buffer");
    rx_callBknFn(UpdateBuffer, ctx, bufferContent, offset, range);
}


#if 0
LOCAL rx_renderPipeline rx_makeRenderPipeline(rx_RenderPipelineDesc* desc) {
    ASSERT(rx__ctx);
    ASSERT(desc);

}
#endif