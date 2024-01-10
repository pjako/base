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

// limits
enum {
    RX_MAX_COLOR_TARGETS = 4,
    RX_MAX_SHADERSTAGE_BINDING = 16,
    RX_MAX_SHADERSTAGE_BUFFERS = 16,
    RX_MAX_VERTEX_ATTRIBUTES = 16,
    RX_MAX_SWAP_TEXTURES = 4
};

typedef enum rx_error {
    rx_error_ok = 0,
    rx_error_shaderCompilationFailed,
    rx_error_shaderLinkFailed,
    rx_error__forceU32 = RX_U32_MAX
} rx_error;

typedef struct rx_Range {
    mms size;
    void* ptr;
} rx_Range;

typedef union rx_buffer {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx     : 16;
        u32 gen     : 7;
        u32 passIdx : 9;
    };
#endif
} rx_buffer;

typedef union rx_sampler {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_sampler;

typedef union rx_texture {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_texture;

typedef union rx_textureView {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u32 gen     : 7;
        u32 passIdx : 9;
    };
#endif
} rx_textureView;


typedef union rx_swapChain {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_swapChain;

typedef union rx_renderShader {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_renderShader;

typedef union rx_resGroupLayout {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_resGroupLayout;
typedef union rx_resGroup {
    u32 id;
//#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u32 gen         : 15;
        bx hasPassDep   : 1;
    };
//#endif
} rx_resGroup;

typedef union rx_dynResGroup {
    u32 id;
//#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u32 gen         : 15;
        bx hasPassDep   : 1;
    };
//#endif
} rx_dynResGroup;

typedef union rx_renderPass {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_renderPass;

typedef union rx_computePass {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_computePass;

typedef union rx_renderPipeline {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_renderPipeline;

#ifndef RX_DEFAULT_MAX_BUFFERS
#define RX_DEFAULT_MAX_BUFFERS 64
#endif
#ifndef RX_DEFAULT_MAX_SAMPLERS
#define RX_DEFAULT_MAX_SAMPLERS 64
#endif
#ifndef RX_DEFAULT_MAX_TEXTURES
#define RX_DEFAULT_MAX_TEXTURES 64
#endif
#ifndef RX_DEFAULT_MAX_TEXTURE_VIEWS
#define RX_DEFAULT_MAX_TEXTURE_VIEWS 64
#endif
#ifndef RX_DEFAULT_MAX_RENDER_SHADERS
#define RX_DEFAULT_MAX_RENDER_SHADERS 64
#endif
#ifndef RX_DEFAULT_MAX_PASSES
#define RX_DEFAULT_MAX_PASSES 64
#endif
#ifndef RX_DEFAULT_MAX_PER_FRAME_PASSES
#define RX_DEFAULT_MAX_PER_FRAME_PASSES 64
#endif
#ifndef RX_DEFAULT_MAX_RENDER_PIPELINES
#define RX_DEFAULT_MAX_RENDER_PIPELINES 64
#endif
#ifndef RX_DEFAULT_MAX_SWAP_CHAINS
#define RX_DEFAULT_MAX_SWAP_CHAINS 64
#endif

#ifndef RX_DEFAULT_MAX_UNIQUE_PASSES
#define RX_DEFAULT_MAX_UNIQUE_PASSES 64
#endif
#ifndef RX_DEFAULT_MAX_PASSES_PER_FRAME
#define RX_DEFAULT_MAX_PASSES_PER_FRAME 64
#endif
#ifndef RX_DEFAULT_MAX_PER_RES_GROUPS
#define RX_DEFAULT_MAX_PER_RES_GROUPS 128
#endif

typedef struct rx_SetupDesc {
    struct {
        struct {
            void* appleCaOpenGlLayer;
        } gl;
    } context;
    u32 sampleCount;
    u32 maxSamplers;
    u32 maxBuffers;
    u32 maxTextures;
    u32 maxTextureViews;
    u32 maxPasses;
    u32 maxPerFramePasses;
    u32 maxRenderShaders;
    u32 maxRenderPipelines;
    u32 maxResGroups;
    u32 maxSwapChains;
    u32 maxUniquePasses;
    u32 maxPassesPerFrame;
} rx_SetupDesc;

API void rx_setup(rx_SetupDesc* desc);
API void rx_shutdown(void);
API void rx_reset(void);

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
    rx_bufferUsage_immutable               = 1 << 12,
    rx_bufferUsage_dynamic                 = 1 << 13,
    rx_bufferUsage_stream                  = 1 << 14,
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

// Sampler

typedef enum rx_addressMode {
    rx_addressMode__default,
    rx_addressMode_repeat,
    rx_addressMode_mirrorRepeat,
    rx_addressMode_clampToEdge,
    rx_addressMode__forceU32 = RX_U32_MAX
} rx_addressMode;

typedef enum rx_filterMode {
    rx_filterMode__default,
    rx_filterMode_nearest,
    rx_filterMode_linear,
    rx_filterMode__forceU32 = RX_U32_MAX
} rx_filterMode;

typedef enum rx_mipmapMode {
    rx_mipmapMode__default,
    rx_mipmapMode_nearest,
    rx_mipmapMode_linear,
    rx_mipmapMode__forceU32 = RX_U32_MAX
} rx_mipmapMode;

typedef enum rx_compareFunc {
    rx_compareFunc__default,
    rx_compareFunc_never,
    rx_compareFunc_less,
    rx_compareFunc_equal,
    rx_compareFunc_lessEqual,
    rx_compareFunc_greater,
    rx_compareFunc_notEqual,
    rx_compareFunc_greaterEqual,
    rx_compareFunc_always,
    rx_compareFunc__count,
    rx_compareFunc__forceU32 = RX_U32_MAX
} rx_compareFunc;

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
    rx_compareFunc compare;
    u16            maxAnisotropy;
} rx_SamplerDesc;

API rx_sampler rx_makeSampler(const rx_SamplerDesc* desc);

// Render Shader

typedef enum rx_shaderStage {
    rx_shaderStage__invalid  = 0,
    rx_shaderStage_vertex    = 1,
    rx_shaderStage_fragment  = 2,
    rx_shaderStage_all       = rx_shaderStage_vertex | rx_shaderStage_fragment,
    rx_shaderStage__forceU32 = RX_U32_MAX
} rx_shaderStage;
typedef flags32 rx_shaderStageFlags;

typedef struct rx_ShaderStageDesc {
    Str8 source;
    Str8 byteCode;
    Str8 entry;
} rx_ShaderStageDesc;

typedef struct rx_RenderShaderDesc {
    Str8 label;
    // TODO: attributes
    rx_ShaderStageDesc vs;
    rx_ShaderStageDesc fs;
} rx_RenderShaderDesc;

API rx_renderShader rx_makeRenderShader(const rx_RenderShaderDesc* desc);


// Texture

typedef enum rx_textureFormat {
    rx_textureFormat__invalid,
    // 8-bit formats
    rx_textureFormat_r8unorm,
    rx_textureFormat_r8snorm,
    rx_textureFormat_r8uint,
    rx_textureFormat_r8sint,
    // 16-bit formats
    rx_textureFormat_r16uint,
    rx_textureFormat_r16sint,
    rx_textureFormat_r16float,
    rx_textureFormat_rg8unorm,
    rx_textureFormat_rg8snorm,
    rx_textureFormat_rg8uint,
    rx_textureFormat_rg8sint,
    // 32-bit formats
    rx_textureFormat_r32uint,
    rx_textureFormat_r32sint,
    rx_textureFormat_r32float,
    rx_textureFormat_rg16uint,
    rx_textureFormat_rg16sint,
    rx_textureFormat_rg16float,
    rx_textureFormat_rgba8unorm,
    rx_textureFormat_rgba8unormSrgb,
    rx_textureFormat_rgba8snorm,
    rx_textureFormat_rgba8uint,
    rx_textureFormat_rgba8sint,
    rx_textureFormat_bgra8unorm,
    rx_textureFormat_bgra8unormSrgb,
    // Packed 32-bit formats
    rx_textureFormat_rgb9e5ufloat,
    rx_textureFormat_rgb10a2unorm,
    rx_textureFormat_rg11b10ufloat,
    // 64-bit formats
    rx_textureFormat_rg32uint,
    rx_textureFormat_rg32sint,
    rx_textureFormat_rg32float,
    rx_textureFormat_rgba16uint,
    rx_textureFormat_rgba16sint,
    rx_textureFormat_rgba16float,
    // 128-bit formats
    rx_textureFormat_rgba32uint,
    rx_textureFormat_rgba32sint,
    rx_textureFormat_rgba32float,
    // Depth and stencil formats
//    rx_textureFormat_stencil8,
    rx_textureFormat_depth16unorm,
    rx_textureFormat_depth24plus,
    rx_textureFormat_depth24plusStencil8,
    rx_textureFormat_depth32float,
    // BC compressed formats usable if "texture-compression-bc" is both
    // supported by the device/user agent and enabled in requestDevice.
    rx_textureFormat_bc1RgbaUnorm,
    rx_textureFormat_bc1RgbaUnormSrgb,
    rx_textureFormat_bc2RgbaUnorm,
    rx_textureFormat_bc2RgbaUnormSrgb,
    rx_textureFormat_bc3RgbaUnorm,
    rx_textureFormat_bc3RgbaUnormSrgb,
    rx_textureFormat_bc4RUnorm,
    rx_textureFormat_bc4RSnorm,
    rx_textureFormat_bc5RgUnorm,
    rx_textureFormat_bc5RgSnorm,
    rx_textureFormat_bc6hRgbuFloat,
    rx_textureFormat_bc6hRgbFloat,
    rx_textureFormat_bc7RgbaUnorm,
    rx_textureFormat_bc7RgbaUnormSrgb,
    // "depth24unorm-stencil8" feature
    rx_textureFormat_depth24unormStencil8,
    // "depth32float-stencil8" feature
    rx_textureFormat_depth32floatStencil8,
    rx_textureFormat_swapChain, // always matches current swapchain
    rx_textureFormat__count,
    rx_textureFormat__forceU32 = RX_U32_MAX
} rx_textureFormat;

typedef enum rx_textureUsage {
    rx_textureUsage__default                  = 0,
    rx_textureUsage_copy                      = 1 << 0,
    rx_textureUsage_copySrc                   = 1 << 1,
    rx_textureUsage_copyDst                   = 1 << 2,
    rx_textureUsage_sampled                   = 1 << 3,
    rx_textureUsage_storage                   = 1 << 4,
    rx_textureUsage_renderAttachment          = 1 << 5,
    rx_textureUsage_renderQueue               = 1 << 6,
    rx_textureUsage_computeQueue              = 1 << 7,
    rx_textureUsage_readOnlyStorageTexture    = 1 << 8,
    rx_textureUsage_present                   = 1 << 9,
    rx_textureUsage__forceU32 = RX_U32_MAX
} rx_textureUsage;
typedef flags32 rx_textureUsageFlags;

typedef enum rx_textureViewDimension {
    rx_textureViewDimesion__default,
    rx_textureViewDimesion_1d,
    rx_textureViewDimesion_2d,
    rx_textureViewDimesion_array2d,
    rx_textureViewDimesion_cube,
    rx_textureViewDimesion_cubeArray,
    rx_textureViewDimesion_3d,
    rx_textureViewDimesion__forceU32 = RX_U32_MAX
} rx_textureViewDimension;

typedef enum rx_sampleCount {
    rx_sampleCount__default,
    rx_sampleCount_1bit      = 1,
    rx_sampleCount_2bit      = 2,
    rx_sampleCount_4bit      = 4,
    rx_sampleCount_8bit      = 8,
    rx_sampleCount_16bit     = 16,
    rx_sampleCount_32bit     = 32,
    rx_sampleCount_64bit     = 64,
    rx_sampleCount__forceU32 = RX_U32_MAX
} rx_sampleCount;



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

API flags64 rx_getResGroupPassDepFlags(rx_resGroup resGroup);

// RenderPipeline

// Topology type of the mesh
typedef enum rx_primitiveTopology {
    rx_primitiveTopology__default,
    rx_primitiveTopology_pointList,
    rx_primitiveTopology_lineList,
    rx_primitiveTopology_lineStrip,
    rx_primitiveTopology_triangleList,
    rx_primitiveTopology_triangleStrip,
    rx_primitiveTopology_triangleFan,
    rx_primitiveTopology_lineListWithAdjancy,
    rx_primitiveTopology_lineStripWithAdjancy,
    rx_primitiveTopology_triangleListWithAdjancy,
    rx_primitiveTopology_triangleStripWithAdjancy,
    rx_primitiveTopology_patchList,
    rx_primitiveTopology__count,
    rx_primitiveTopology__forceU32 = RX_U32_MAX,
} rx_primitiveTopology;

/*
    sg_cull_mode
    The face-culling mode, this is used in the
    sg_pipeline_desc.rasterizer.cull_mode member when creating a
    pipeline object.
    The default cull mode is SG_CULLMODE_NONE
*/
typedef enum rx_cullMode {
    rx_cullMode__default,
    rx_cullMode_none,
    rx_cullMode_front,
    rx_cullMode_back,
    rx_cullMode__count,
    rx_cullMode__forceU32 = RX_U32_MAX
} rx_cullMode;

/*
    sg_face_winding
    The vertex-winding rule that determines a front-facing primitive. This
    is used in the member sg_pipeline_desc.rasterizer.face_winding
    when creating a pipeline object.
    The default winding is SG_FACEWINDING_CW (clockwise)
*/
typedef enum rx_faceWinding {
    rx_faceWinding__default,
    rx_faceWinding_clockwise,
    rx_faceWinding_counterClockwise,
    rx_faceWinding__count,
    rx_faceWinding__forceU32 = RX_U32_MAX
} rx_faceWinding;

typedef struct rx_RasterizerState {
    rx_cullMode cullMode;
    rx_faceWinding faceWinding;
    float depthBias;
    float depthBiasSlopeScale;
    float depthBiasClamp;
} rx_RasterizerState;


typedef enum rx_stencilOp {
    rx_stencilOp__default,
    rx_stencilOp_keep,
    rx_stencilOp_zero,
    rx_stencilOp_replace,
    rx_stencilOp_incrClamp,
    rx_stencilOp_decrClamp,
    rx_stencilOp_invert,
    rx_stencilOp_incrWrap,
    rx_stencilOp_decrWrap,
    rx_stencilOp__count,
    rx_stencilOp__forceU32 = RX_U32_MAX
} rx_stencilOp;

typedef struct rx_StencilState {
    rx_stencilOp failOp;
    rx_stencilOp depthFailOp;
    rx_stencilOp passOp;
    rx_compareFunc compareFunc;
} rx_StencilState;

typedef struct rx_DepthStencilState {
    bool disabled;
    rx_textureFormat format;
    rx_StencilState stencilFront;
    rx_StencilState stencilBack;
    rx_compareFunc depthCompareFunc;
    bool depthWriteEnabled;
    bool stencilEnabled;
    uint8_t stencilReadMask;
    uint8_t stencilWriteMask;
    uint8_t stencilRef;
} rx_DepthStencilState;

typedef enum rx_colorMask {
    rx_colorMask__default  = 0,
    rx_colorMask_none      = 0x10,   // all channels disabled
    rx_colorMask_r         = 0x1,
    rx_colorMask_g         = 0x2,
    rx_colorMask_b         = 0x4,
    rx_colorMask_a         = 0x8,
    rx_colorMask_rgb       = 0x7,
    rx_colorMask_rgba      = 0xF,
    rx_colorMask__forceU32 = RX_U32_MAX
} rx_colorMask;
typedef flags32 rx_colorMaskFlags;

typedef enum rx_blendFactor {
    rx_blendFactor__default,
    rx_blendFactor_zero,
    rx_blendFactor_one,
    rx_blendFactor_srcColor,
    rx_blendFactor_oneMinusSrcColor,
    rx_blendFactor_srcAlpha,
    rx_blendFactor_oneMinusSrcAlpha,
    rx_blendFactor_dstColor,
    rx_blendFactor_oneMinusDstColor,
    rx_blendFactor_dstAlpha,
    rx_blendFactor_oneMinusDstAlpha,
    rx_blendFactor_srcAlphaSaturated,
    rx_blendFactor_blendColor,
    rx_blendFactor_oneMinusBlendColor,
    rx_blendFactor_blendAlpha,
    rx_blendFactor_oneMinusBlendAlpha,
    rx_blendFactor__count,
    rx_blendFactor__forceU32 = RX_U32_MAX
} rx_blendFactor;

typedef enum rx_blendOp {
    rx_blendOp__default,
    rx_blendOp_add,
    rx_blendOp_substract,
    rx_blendOp_reverseSubstract,
    rx_blendOp_min,
    rx_blendOp_max,
    rx_blendOp__count,
    rx_blendOp__forceU32 = RX_U32_MAX
} rx_blendOp;

typedef struct rx_TargetBlendState {
    bx enabled : 1;
    rx_blendFactor srcFactorRgb : 4;
    rx_blendFactor dstFactorRgb : 4;
    rx_blendOp opRgb : 4;
    rx_blendFactor srcFactorAlpha : 4;
    rx_blendFactor dstFactorAlpha : 4;
    rx_blendOp opAlpha : 4;
    u8 colorWriteMask;
    Rgba blendColor;
    //float blendColor[4];
} rx_TargetBlendState;

typedef struct rx_ColorTargetState {
    rx_textureFormat format : 8;
    rx_TargetBlendState blend;
} rx_ColorTargetState;

typedef enum rx_indexType {
    rx_indexType_none,
    rx_indexType_u16,
    rx_indexType_u32,
    rx_indexType__count,
    rx_indexType__forceU32 = RX_U32_MAX
} rx_indexType;

typedef enum rx_vertexStep {
    rx_vertexStep_default,
    rx_vertexStep_perVertex,
    rx_vertexStep_perInstance,
    rx_vertexStep__count,
    rx_vertexStep__forceU32 = RX_U32_MAX
} rx_vertexStep;

typedef struct rx_BufferLayoutDesc {
    uint32_t stride;
    rx_vertexStep stepFunc;
    uint32_t stepRate;
} rx_BufferLayoutDesc;

typedef enum rx_vertexFormat {
    rx_vertexFormat__invalid,
    rx_vertexFormat_u8x2,
    rx_vertexFormat_u8x4,
    rx_vertexFormat_s8x2,
    rx_vertexFormat_s8x4,
    rx_vertexFormat_u8x2n,
    rx_vertexFormat_u8x4n,
    rx_vertexFormat_s8x2n,
    rx_vertexFormat_s8x4n,
    rx_vertexFormat_u2x10x10x10,
    rx_vertexFormat_u16x2,
    rx_vertexFormat_u16x4,
    rx_vertexFormat_s16x2,
    rx_vertexFormat_s16x4,
    rx_vertexFormat_u16x2n,
    rx_vertexFormat_u16x4n,
    rx_vertexFormat_s16x2n,
    rx_vertexFormat_s16x4n,
    rx_vertexFormat_f16x2,
    rx_vertexFormat_f16x4,
    rx_vertexFormat_u32x1,
    rx_vertexFormat_u32x2,
    rx_vertexFormat_u32x3,
    rx_vertexFormat_u32x4,
    rx_vertexFormat_s32x1,
    rx_vertexFormat_s32x2,
    rx_vertexFormat_s32x3,
    rx_vertexFormat_s32x4,
    rx_vertexFormat_f32x1,
    rx_vertexFormat_f32x2,
    rx_vertexFormat_f32x3,
    rx_vertexFormat_f32x4,
    rx_vertexFormat__count,
    rx_vertexFormat__forceU32 = RX_U32_MAX
} rx_vertexFormat;

typedef struct rx_VertexAttrDesc {
    Str8 name;
    i32 bufferIndex;
    i32 offset;
    rx_vertexFormat format;
} rx_VertexAttrDesc;

typedef struct rx_LayoutDesc {
    rx_BufferLayoutDesc buffers[RX_MAX_SHADERSTAGE_BUFFERS];
    rx_VertexAttrDesc attrs[RX_MAX_VERTEX_ATTRIBUTES];
} rx_LayoutDesc;

typedef enum rx_bufferBindingType {
    rx_bufferBindingType__invalid,
    rx_bufferBindingType_uniform,
    rx_bufferBindingType_storage,
    rx_bufferBindingType_readOnlyStorage,
    rx_bufferBindingType__forceU32 = RX_U32_MAX
} rx_bufferBindingType;

typedef struct rx_BufferBindingLayout {
    rx_bufferBindingType type;
    bx hasDynamicOffset;
    u64 minBindingSize;
} rx_BufferBindingLayout;

typedef enum rx_bindingType {
    rx_bindingType__invalid,
    rx_bindingType_sampler,
    rx_bindingType_combinedTextureSampler,
    rx_bindingType_sampledTexture,
    rx_bindingType_storageTexture,
    rx_bindingType_uniformTexelBuffer,
    rx_bindingType_storageTexelBuffer,
    rx_bindingType_uniformBuffer,
    rx_bindingType_storageBuffer,
    rx_bindingType_uniformBufferDynamic,
    rx_bindingType_storageBufferDynamic,
    rx_bindingType_inputAttachment,
    rx_bindingType__count,
    rx_bindingType__forceU32 = RX_U32_MAX
} rx_bindingType;

typedef enum rx_samplerBindingType {
    rx_samplerBindingType__invalid,
    rx_samplerBindingType_filtering,
    rx_samplerBindingType_nonFiltering,
    rx_samplerBindingType_comparison,
    rx_samplerBindingType__forceU32 = RX_U32_MAX
} rx_samplerBindingType;

typedef struct rx_SamplerBindingLayout {
    rx_samplerBindingType type;
} rx_SamplerBindingLayout;

typedef enum rx_textureSampleType {
    rx_textureSampleType__invalid,
    rx_textureSampleType_float,
    rx_textureSampleType_unfilterableFloat,
    rx_textureSampleType_depth,
    rx_textureSampleType_sint,
    rx_textureSampleType_uint,
    rx_textureSampleType__forceU32 = RX_U32_MAX
} rx_textureSampleType;

typedef struct rx_TextureBindingLayout {
    rx_textureSampleType sampleType;
    rx_textureViewDimension viewDimension;
    bx multisampled;
} rx_TextureBindingLayout;

typedef enum rx_storageTextureAccess {
    rx_storageTextureAccess__invalid,
    rx_storageTextureAccess_readOnly,
    rx_storageTextureAccess_writeOnly,
    rx_storageTextureAccess__forceU32 = RX_U32_MAX
} rx_storageTextureAccess;

typedef struct rx_StorageTextureBindingLayout {
    rx_storageTextureAccess access;
    rx_textureFormat format;
    rx_textureViewDimension viewDimension;
} rx_StorageTextureBindingLayout;

typedef struct rx_BindingLayoutDesc {
    // this is for the bindless stuff
    rx_bindingType type;
    u32 binding;
    // this is for the bindless stuff
    u32 count;
    rx_shaderStageFlags visibility;
    rx_BufferBindingLayout buffer;
    rx_SamplerBindingLayout sampler;
    rx_TextureBindingLayout texture;
    rx_StorageTextureBindingLayout storageTexture;
} rx_BindingLayoutDesc;

typedef struct rx_RenderPipelineDesc {
    Str8                    label;
    rx_primitiveTopology    primitiveTopology;
    rx_indexType            indexFormat;
    struct {
        rx_renderShader shader;
        Str8            vsEntryPoint;
        Str8            fsEntryPoint;
    } program;
    rx_LayoutDesc           layout;
    rx_RasterizerState      rasterizer;
    rx_DepthStencilState    depthStencil;
    u32                     colorTargetCount;
    rx_ColorTargetState     colorTargets[RX_MAX_COLOR_TARGETS];
    u32                     sampleCount;
    u32                     sampleMask;
    bx                      alphaToCoverageEnabled;
    rx_BindingLayoutDesc    bindings[RX_MAX_SHADERSTAGE_BINDING];

    rx_resGroupLayout       resourceGroups[6];

    u32                     dynamicConstantBlockSize;
} rx_RenderPipelineDesc;

API rx_renderPipeline rx_makeRenderPipeline(rx_RenderPipelineDesc* desc);



// in the future there may be multiple swap chains so this needs to be called with a specific one
API rx_textureView rx_getCurrentSwapTextureView(void);

typedef struct rx_ScissorRect {
    struct { uint16_t x, y; } offset;
    struct { uint16_t x, y; } extend;
} rx_ScissorRect;

typedef struct rx_ViewPort {
    float x, y, width, height, minDepth, maxDepth;
} rx_ViewPort;

typedef struct rx_DrawArea {
    rx_ViewPort viewPort;
    rx_ScissorRect scissor;
    rx_resGroup resGroup0;
    uint32_t drawOffset;
    uint32_t drawCount;
} rx_DrawArea;


// FrameGraph

typedef enum rx_loadOp {
    rx_loadOp__default,
    rx_loadOp_clear,
    rx_loadOp_load,
    rx_loadOp_dontCare,
    rx_loadOp__count,
    rx_loadOp__forceU32 = RX_U32_MAX
} rx_loadOp;

typedef enum rx_storeOp {
    rx_storeOp__default,
    rx_storeOp_store,
    rx_storeOp_clear,
    rx_storeOp__count,
    rx_storeOp__forceU32 = RX_U32_MAX
} rx_storeOp;

typedef struct rx_RenderPassColorTargetDesc {
    rx_textureView view;
    rx_loadOp loadOp;
    rx_storeOp storeOp;
    Rgba clearColor;
} rx_RenderPassColorTargetDesc;

typedef struct rx_RenderPassDesc {
    Str8 name;
    rx_RenderPassColorTargetDesc colorTargets[RX_MAX_COLOR_TARGETS];
    struct {
        bx               active;
        rx_textureFormat format;
        rx_loadOp        depthLoadOp;
        rx_loadOp        stencilLoadOp;
    } depthStencil;
    rx_sampleCount sampleCount;
    // these can be left to zero unless you are using the default (system) pass running on a legacy Gfx API (OpenGL, WebGL)
    f32 width;
    f32 height;
} rx_RenderPassDesc;

typedef struct rx_RenderPassResult {
    rx_textureView colorTargets[RX_MAX_COLOR_TARGETS];
} rx_RenderPassResult;

API rx_renderPass rx_makeRenderPass(rx_RenderPassDesc* renderPassDesc, rx_RenderPassResult* resultTextureViews);

typedef enum rx_renderCmds {
    rx_renderCmd_pipeline          = 1 << 0,
    rx_renderCmd_vertexBuffer0     = 1 << 1,
    rx_renderCmd_vertexBuffer1     = 1 << 2,
    rx_renderCmd_vertexBuffer2     = 1 << 3,
    rx_renderCmd_indexBuffer       = 1 << 4,
    rx_renderCmd_resGroup1         = 1 << 5,
    rx_renderCmd_resGroup2         = 1 << 6,
    rx_renderCmd_resGroup3         = 1 << 7,
    rx_renderCmd_dynResGroup0      = 1 << 8,
    rx_renderCmd_dynResGroup1      = 1 << 9,
    rx_renderCmd_instanceOffset    = 1 << 10,
    rx_renderCmd_instanceCount     = 1 << 11,
} rx_renderCmds;

typedef struct rx_DrawList {
    flags64 passIdxDepFlags;
    u32 count;
    u32 commands[0];
} rx_DrawList;

// areas/drawList memory has to be kept alive till after rx_commit was called
API void rx_setRenderPassDrawList(rx_renderPass renderPass, rx_DrawArea* arenas, u32 areaCount, rx_DrawList* drawList);

API void rx_commit(void);

#if 0
API void rx_setRenderPassCmds(rx_RenderPassCmds* cmds);
API void rx_setComputePassCmds(rx_ComputePassCmds* cmds);
#endif


#ifdef __cplusplus
}
#endif