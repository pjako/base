// this is quick-and-dirty gfx lib to get something on the screen, OpenGL only that could be removed on iOS at any point.

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RX_USE_MODERN_ONLY
#define RX_USE_MODERN_ONLY 0
#else
#undef RX_USE_MODERN_ONLY
#define RX_USE_MODERN_ONLY 1
#endif


enum {
    RX_U32_MAX = 0x7FFFFFFF,
};

typedef struct rx_Range {
    mms size;
    void* ptr;
} rx_Range;

typedef union rx_buffer {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u32 idx     : 16;
        u32 gen     : 7;
        u32 passIdx : 9;
    };
#endif
} rx_buffer;

typedef union rx_texture {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u32 idx     : 16;
        u32 gen     : 16;
    };
#endif
} rx_texture;

typedef union rx_textureView {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u32 idx     : 16;
        u32 gen     : 7;
        u32 passIdx : 9;
    };
#endif
} rx_textureView;

typedef struct rx_shader {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u32 idx     : 16;
        u32 gen     : 16;
    };
#endif
} rx_shader;

typedef struct rx_resGroupLayout {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u32 idx     : 16;
        u32 gen     : 16;
    };
#endif
} rx_resGroupLayout;
typedef struct rx_resGroup {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u32 idx     : 16;
        u32 gen     : 16;
    };
#endif
} rx_resGroup;

typedef struct rx_renderPass {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u32 idx     : 16;
        u32 gen     : 16;
    };
#endif
} rx_renderPass;

typedef struct rx_computePass {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u32 idx     : 16;
        u32 gen     : 16;
    };
#endif
} rx_computePass;

typedef struct rx_renderPipeline {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u32 idx     : 16;
        u32 gen     : 16;
    };
#endif
} rx_renderPipeline;

#if 0
typedef struct rx_RenderPipelineDesc {
    Str8                    label;
//    rx_pass                 renderPass;
    rx_primitiveTopology    primitiveTopology;
    rx_indexType            indexFormat;
    struct {
        struct {
            rx_Shader shader;
            Str8      entryPoint;
        } vs;
        struct {
            rx_Shader shader;
            Str8      entryPoint;
        } fs;
    } program;
    rx_LayoutDesc           layout;
    rx_RasterizerState      rasterizer;
    rx_DepthStencilState    depthStencil;
    u32                colorTargetCount;
    rx_ColorTargetState     colorTargets[RX_MAX_COLOR_TARGETS];
    u32                sampleCount;
    u32                sampleMask;
    bool                    alphaToCoverageEnabled;
    rx_BindingLayoutDesc    bindings[RX_MAX_SHADERSTAGE_BINDING];

    rx_resGroupLayout resourceGroups[6];

    u32 dynamicConstantBlockSize;
} rx_RenderPipelineDesc;

API rx_renderPipeline rx_makeRenderPipeline(rx_RenderPipelineDesc* desc);
#endif

#ifndef RX_DEFAULT_MAX_BUFFERS
#define RX_DEFAULT_MAX_BUFFERS 64
#endif
#ifndef RX_DEFAULT_MAX_TEXTURES
#define RX_DEFAULT_MAX_TEXTURES 64
#endif
#ifndef RX_DEFAULT_MAX_TEXTURE_VIEWS
#define RX_DEFAULT_MAX_TEXTURE_VIEWS 64
#endif
#ifndef RX_DEFAULT_MAX_PASSES
#define RX_DEFAULT_MAX_PASSES 64
#endif
#ifndef RX_DEFAULT_MAX_PER_FRAME_PASSES
#define RX_DEFAULT_MAX_PER_FRAME_PASSES 64
#endif

typedef struct rx_SetupDesc {
    struct {
        struct {
            void* appleCaOpenGlLayer;
        } gl;
    } context;
    u32 sampleCount;
    u32 maxBuffers;
    u32 maxTextures;
    u32 maxTextureViews;
    u32 maxPasses;
    u32 maxPerFramePasses;
} rx_SetupDesc;

API void rx_setup(rx_SetupDesc* desc);

API void rx_nextFrame(void);

// Resources

// Buffer
typedef enum rx_bufferUsage {
    rx_bufferUsage__invalid                = 0,
    rx_bufferUsage_mapRead                 = 1 << 0,
    rx_bufferUsage_mapWrite                = 1 << 1,
    rx_bufferUsage_copySrc                 = 1 << 2,
    rx_bufferUsage_copyDst                 = 1 << 3,
    rx_bufferUsage_index                   = 1 << 4,
    rx_bufferUsage_vertex                  = 1 << 5,
    rx_bufferUsage_uniform                 = 1 << 6,
    rx_bufferUsage_storage                 = 1 << 7,
    rx_bufferUsage_indirect                = 1 << 8,
    rx_bufferUsage_queryResolve            = 1 << 9,
    rx_bufferUsage__readOnlyStorageBuffer  = 1 << 10,
    rx_bufferUsage__readOnlyStorageTexture = 1 << 11,
    rx_bufferUsage_graphicsQueue           = 1 << 12,
    rx_bufferUsage_computeQueue            = 1 << 13,
    rx_bufferUsage_immutable               = 1 << 14,
    rx_bufferUsage_dynamic                 = 1 << 15,
    rx_bufferUsage_stream                  = 1 << 16,
    rx_bufferUsage__forceU32 = RX_U32_MAX
} rx_bufferUsage;
typedef flags32 rx_bufferUsageFlags;

typedef struct rx_BufferDesc {
    Str8 label;
    rx_bufferUsageFlags usage;
    mms size;
    rx_Range data;
} rx_BufferDesc;


API rx_buffer rx_makeBuffer(const rx_BufferDesc* desc);
API void rx_updateBuffer(rx_buffer buffer, mms offset, rx_Range range);

// Texture

// ResGroup

typedef struct rx_ResLayoutDesc {
    u32 type : 4;
    u32 countOrSize : 16;
} rx_ResDesc;

typedef struct rx_ResGroupLayoutDesc {
    rx_ResDesc resources[12];
} rx_ResGroupLayoutDesc;

typedef struct rx_ResGroupDesc {
    rx_resGroupLayout layout;
    struct {
        u32 offset;
        rx_buffer buffer;
        rx_texture texture;
    } resources[12];
} rx_ResGroupDesc;

typedef struct rx_TextureSlice {
    u32 count;
    rx_texture* textures;
} rx_TextureSlice;

typedef struct rx_BufferSlice {
    u32 count;
    rx_texture* textures;
} rx_BufferSlice;


API rx_texture rx_getCurrentSwapTexture(void);

API rx_renderPass rx_runRenderPass(rx_TextureSlice* outTextures);
API rx_computePass rx_runComputePass(rx_BufferSlice* outBuffers);

#if 0
API void rx_setRenderPassCmds(rx_RenderPassCmds* cmds);
API void rx_setComputePassCmds(rx_ComputePassCmds* cmds);
#endif


#ifdef __cplusplus
}
#endif