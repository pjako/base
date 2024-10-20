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
    RX_COUNT_SHADER_STAGES = 2,
    RX_MAX_COLOR_TARGETS = 4,
    RX_MAX_RESGROUP_UNIFORM_MEMBERS = 12,
    RX_MAX_SHADERSTAGE_BINDING = 16,
    RX_MAX_SHADERSTAGE_BUFFERS = 3,
    RX_MAX_VERTEX_ATTRIBUTES = 16,
    RX_MAX_INFLIGHT_FRAMES = 4,
    RX_MAX_RESOURCES_PER_RES_GROUP = 12,
    RX_MAX_SHADERSTAGE_TEXTURE_SAMPLER_PAIRS = 12,
    RX_MAX_MIPMAPS = 16,
};

typedef enum rx_error {
    rx_error_ok = 0,
    rx_error_shaderCompilationFailed,
    rx_error_shaderLinkFailed,
    rx_error__forceU32 = RX_U32_MAX
} rx_error;

typedef struct rx_Range {
    mms size;
    void* content;
} rx_Range;

typedef union rx_buffer {
    u64 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx      : 16;
        u16 gen      : 16;
        u16 passIdx  : 16;
        u16 __unused : 16;
    };
#endif
} rx_buffer;

typedef union rx_bumpAllocator {
    uint32_t id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_bumpAllocator;

typedef union rx_sampler {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_sampler;

#pragma pack(push, 1)
typedef union rx_texture {
    u64 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
        u16 passIdx;
        u16 __unused;
    };
#endif
} rx_texture;
#pragma pack(pop)


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
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen : 16;
       // bx hasPassDep : 1;
    };
#endif
} rx_resGroup;

typedef union rx_renderPipeline {
    u32 id;
#ifdef RX_INTERNAL
    struct {
        u16 idx;
        u16 gen;
    };
#endif
} rx_renderPipeline;

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


#ifndef RX_DEFAULT_MAX_BUFFERS
#define RX_DEFAULT_MAX_BUFFERS 64
#endif
#ifndef RX_DEFAULT_MAX_BUMP_ALLOCATORS
#define RX_DEFAULT_MAX_BUMP_ALLOCATORS 48
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
#ifndef RX_DEFAULT_MAX_RES_GROUPS
#define RX_DEFAULT_MAX_RES_GROUPS 64
#endif
#ifndef RX_DEFAULT_MAX_RES_GROUP_LAYOUTS
#define RX_DEFAULT_MAX_RES_GROUP_LAYOUTS 64
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
#ifndef RX_DEFAULT_MAX_STREAMING_UNIFORM_SIZE
#define RX_DEFAULT_MAX_STREAMING_UNIFORM_SIZE MEGABYTE(5)
#endif
#ifndef RX_DEFAULT_MAX_DYNAMIC_UNIFORM_SIZE
#define RX_DEFAULT_MAX_DYNAMIC_UNIFORM_SIZE MEGABYTE(5)
#endif

typedef enum rx_backend {
    rx_backend__default,
    rx_backend_gl400,
    rx_backend_gles3,
    rx_backend_metal,
    rx_backend_vulkan,
    rx_backend_vulkanBindless,
    rx_backend_dx12,
    rx_backend__count,
} rx_backend;

typedef struct rx_SetupDesc {
    struct {
        struct {
            bool oglWasInitialized;
            void* appleCaOpenGlLayer;
            void(*userInitOpenGL)(void* userPtr);
            void* userPtr;
        } gl;
    } context;
    u32 sampleCount;
    u32 maxSamplers;
    u32 maxBuffers;
    u32 maxBumpAllocators;
    u32 maxTextures;
    u32 maxTextureViews;
    u32 maxPasses;
    u32 maxPerFramePasses;
    u32 maxRenderShaders;
    u32 maxRenderPipelines;
    u32 maxResGroups;
    u32 maxResGroupLayouts;
    u32 maxSwapChains;
    u32 maxUniquePasses;
    u32 maxPassesPerFrame;
    u32 streamingUniformSize;
    u32 dynamicUniformSize;
    rx_backend backendPreference[rx_backend__count];
} rx_SetupDesc;

API void rx_setup(rx_SetupDesc* desc);

typedef struct rx_Ctx rx_Ctx;
API rx_Ctx* rx_getContext(void);
API void rx_setContext(rx_Ctx* ctx);
API rx_backend rx_queryBackend(void);
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
    S8 label;
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
    S8           label;
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

// Texture



typedef enum rx_textureFormat {
    rx_textureFormat__invalid,
    // 8-bit formats
    rx_textureFormat_r8unorm,
    rx_textureFormat_r8snorm,
    rx_textureFormat_r8uint,
    rx_textureFormat_r8sint,
    // 16-bit formats
    // rx_textureFormat_r16unorm,
    // rx_textureFormat_r16snorm,
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
    // rx_textureFormat_rg16unorm,
    // rx_textureFormat_rg16snorm,
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
    rx_textureFormat_swapChain, // always matches current swapchain texture format
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

typedef struct rx_Extend3D {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} rx_Extend3D;

typedef struct rx_Origin3D {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} rx_Origin3D;

typedef enum rx_textureDimension {
    rx_textureDimension__default,
  //rx_textureDimension_1d,
    rx_textureDimension_2d,
    rx_textureDimension_3d,
    rx_textureDimension_cube,
    rx_textureDimension_array,
    rx_textureDimension__forceU32 = RX_U32_MAX
} rx_textureDimension;


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

typedef enum sg_cube_face {
    rx_cubeFace_posX,
    rx_cubeFace_negX,
    rx_cubeFace_posY,
    rx_cubeFace_negY,
    rx_cubeFace_posZ,
    rx_cubeFace_negZ,
    rx_cubeFace__count,
    rx_cubeFace__forceU32  = RX_U32_MAX
} sg_cube_face;

typedef struct rx_ImageData {
    rx_Range subimage[rx_cubeFace__count][RX_MAX_MIPMAPS];
} rx_ImageData;

typedef struct rx_TextureDesc {
    const char* label;
    rx_textureUsageFlags usage;
    rx_textureDimension dimension;
    rx_Extend3D size;
    rx_textureFormat format;
    uint32_t mipLevelCount;
    uint32_t arrayLayerCount;
    rx_sampleCount sampleCount;
    rx_ImageData data;
} rx_TextureDesc;

API rx_texture rx_makeTexture(const rx_TextureDesc* desc);

typedef enum rx_aspect {
    rx_aspect_none = 0x0,
    rx_aspect_color = 0x1,
    rx_aspect_depth = 0x2,
    rx_aspect_stencil = 0x4,
    // Aspects used to select individual planes in a multi-planar format.
    rx_aspect_plane0 = 0x8,
    rx_aspect_plane1 = 0x10,
    // An aspect for that represents the combination of both the depth and stencil aspects. It
    // can be ignored outside of the Vulkan backend.
    rx_aspect_combinedDepthStencil = 0x20,
    rx_aspect__count,
    rx_aspect__forceU32 = RX_U32_MAX
} rx_aspect;
typedef flags32 rx_aspectFlags;

typedef struct rx_TextureCopy {
    uint32_t mipLevel;
    rx_Origin3D origin;  // Texels / array layer
    rx_aspect aspect;
} rx_TextureCopy;

typedef struct rx_TextureDataLayout {
    uint32_t bytesPerRow;
    uint32_t rowsPerTexture;
} rx_TextureDataLayout;

typedef struct rx_TextureUploadDesc {
    rx_TextureDataLayout    layout;
    rx_TextureCopy          copy;
    rx_Extend3D             extend;
    rx_ImageData            data;
} rx_TextureUploadDesc;

API void rx_updateTexture(rx_texture texture, rx_TextureUploadDesc* desc);

// TextureView

typedef enum rx_textureAspect {
    rx_textureAspect_all,
    rx_textureAspect_stencilOnly,
    rx_textureAspect_depthOnly,
    rx_textureAspect_forceU32 = RX_U32_MAX
} rx_textureAspect;

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

typedef struct rx_TextureViewDesc {
    S8 label;
    rx_texture texture;
    rx_textureFormat format;
    rx_textureViewDimension dimension;
    uint32_t baseMipLevel;
    uint32_t mipLevelCount;
    uint32_t baseArrayLayer;
    uint32_t arrayLayerCount;
    rx_textureAspect aspect;
} rx_TextureViewDesc;

API rx_texture rx_makeTextureView(const rx_TextureViewDesc* desc);

// Render Shader

typedef enum rx_shaderStage {
    rx_shaderStage__invalid  = 0,
    rx_shaderStage_vertex    = 1,
    rx_shaderStage_fragment  = 2,
    rx_shaderStage_all       = rx_shaderStage_vertex | rx_shaderStage_fragment,
    rx_shaderStage__forceU32 = RX_U32_MAX
} rx_shaderStage;
typedef flags32 rx_shaderStageFlags;



typedef enum rx_uniformType {
    rx_uniformType__invalid,
    rx_uniformType_f32x1,
    rx_uniformType_f32x2,
    rx_uniformType_f32x3,
    rx_uniformType_f32x4,
    rx_uniformType_i32x1,
    rx_uniformType_i32x2,
    rx_uniformType_i32x3,
    rx_uniformType_i32x4,
    rx_uniformType_mat4x4,
    rx_uniformType_buffer,
    rx_uniformType_tex2d,
    rx_uniformType_tex3d,
    rx_uniformType_cube,
    rx_uniformType_sampler,
    rx_uniformType__count,
    rx_uniformType__forceU32 = RX_U32_MAX
} rx_uniformType;

typedef struct rx_ShaderUniformDesc {
    const char* name;
    rx_uniformType type;
    u32 arrayCount;
} rx_ShaderUniformDesc;

#if 0
typedef enum rx_uniformLayout {
    rx_uniformLayout__default,
    rx_uniformLayout_native,
    rx_uniformLayout_std140,
    rx_uniformLayout__count,
    rx_uniformLayout__forceU32 = RX_U32_MAX
} rx_uniformLayout;

typedef struct rx_ShaderUniformBlockDesc {
    u32 resGroupRef;
    u32 size;
    rx_uniformLayout layout;
    rx_ShaderUniformDesc uniforms[RX_MAX_RESGROUP_UNIFORM_MEMBERS];
} sg_shader_uniform_block_desc;
#endif

typedef enum rx_textureSampleType {
    rx_textureSampleType__invalid,
    rx_textureSampleType_float,
    rx_textureSampleType_unfilterableFloat,
    rx_textureSampleType_depth,
    rx_textureSampleType_sint,
    rx_textureSampleType_uint,
    rx_textureSampleType__forceU32 = RX_U32_MAX
} rx_textureSampleType;

typedef struct rx_ShaderTextureDesc {
    bx used;
    bx multisampled;
    rx_textureFormat textureFormat;
    rx_textureSampleType sampleType;
} rx_ShaderTextureDesc;

typedef struct rx_ShaderSamplerDesc {
    bx used;
    rx_textureSampleType samplerType;
} rx_ShaderSamplerDesc;

// The design of this may change when compute shaders get implemented
typedef struct rx_ShaderStorageBufferDesc {
    bx used;
    u32 arrayCount;
} rx_ShaderStorageBufferDesc;

typedef struct rx_ShaderResGroupDesc {
    u32 bindSlot;
    const char* name;
    u32 size;
    rx_ShaderUniformDesc uniforms[RX_MAX_RESGROUP_UNIFORM_MEMBERS];
    rx_ShaderTextureDesc textures[4];
    rx_ShaderSamplerDesc samplers[4];
    rx_ShaderStorageBufferDesc buffers[4];
} rx_ShaderResGroupDesc;

typedef struct rx_ShaderDynamicConstantsDesc {
    u32 size;
    u32 bindGroupIdx;
    u32 bindGroupSlot;
} rx_ShaderDynamicConstantsDesc;

typedef struct rx_ShaderTextureSamplerPairDesc {
    bx used;
    i32 slot;

    i32 texResGroupIdx;
    i32 texResIdx;
    //i32 imageSlot;

    i32 samplerResGroupIdx;
    i32 samplerResIdx;

    //i32 samplerSlot;
    const char* glslName;
} rx_ShaderTextureSamplerPairDesc;

// {true, 0,  0, 1,  0, 0,  1, 0,  "rx_myTexture_rx_mySampler"}

typedef struct rx_ShaderStageDesc {
    const char* source;
    rx_Range byteCode;
    const char* entry;
    rx_ShaderTextureSamplerPairDesc textureSamplerPairs[RX_MAX_SHADERSTAGE_TEXTURE_SAMPLER_PAIRS];



#if 0
    const char* source;
    sg_range bytecode;
    const char* entry;
    const char* d3d11_target;
    sg_shader_uniform_block_desc uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    sg_shader_image_desc images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_shader_sampler_desc samplers[SG_MAX_SHADERSTAGE_SAMPLERS];
    sg_shader_image_sampler_pair_desc image_sampler_pairs[SG_MAX_SHADERSTAGE_IMAGESAMPLERPAIRS];
#endif

} rx_ShaderStageDesc;

typedef struct rx_RenderShaderDesc {
    const char* name;
    // TODO: attributes
    rx_ShaderStageDesc vs;
    rx_ShaderStageDesc fs;
    rx_ShaderResGroupDesc resGroups[6];
    rx_ShaderDynamicConstantsDesc dynamicConstants[2];
} rx_RenderShaderDesc;

API rx_renderShader rx_makeRenderShader(const rx_RenderShaderDesc* desc);

// ARENA

typedef struct rx_BumpAllocatorDesc {
    // memory for the staging buffer (backend dependent)
    // memory amount allocated is of the size of the gpuBuffer
    Arena* arena;
    // should be big enough to hold data for four frames
    rx_buffer gpuBuffer;
    uint32_t alignment;
} rx_BumpAllocatorDesc;

// The gpu arena is a ringbuffer, depending on api, gpu and buffer type it usese a constantly mapped gpu buffer
// or cpu staging buffer to store & push data to the gpu.
// The passed gpu buffer
API rx_bumpAllocator rx_makeBumpAllocator(const rx_BumpAllocatorDesc* desc);
API u64 rx_bumpAllocatorPushData(rx_bumpAllocator arena, rx_Range data);
// resets the arena
API void rx_bumpAllocatorReset(rx_bumpAllocator arena);


// ResGroup

typedef enum rx_resType {
    rx_resType__invalid,
    rx_resType_dynUniformBuffer,
    rx_resType_texture2d,
    rx_resType_textureCube,
    rx_resType_texture3d,
    rx_resType_sampler,
    rx_resType_readOnlyBuffer,
    rx_resType_rwBuffer,
    rx_resType__forceU32 = RX_U32_MAX
} rx_resType;

typedef struct rx_ResLayoutDesc {
    rx_resType type;
    u32 countOrSize;
    u32 slot;
} rx_ResLayoutDesc;

typedef struct rx_ResGroupLayoutDesc {
    u32 uniformSize;
    rx_ResLayoutDesc resources[12];
} rx_ResGroupLayoutDesc;

API rx_resGroupLayout rx_makeResGroupLayout(rx_ResGroupLayoutDesc* desc);

API rx_resGroupLayout rx_getResGroupLayoutForDynBuffers(void);

typedef struct rx_ResUpdate {
    rx_buffer buffer;
    rx_texture texture;
    rx_sampler sampler;
} rx_ResUpdate;

typedef struct rx_ResGroupUpdateDesc {
    rx_bumpAllocator storeArena;
    u64 targetOffset;
    rx_Range uniformContent;
    rx_ResUpdate resources[RX_MAX_RESOURCES_PER_RES_GROUP];
} rx_ResGroupUpdateDesc;


typedef enum rx_resGroupUsage {
    rx_resGroupUsage_dynamic,
    rx_resGroupUsage_streaming
} rx_resGroupUsage;

typedef struct rx_ResGroupDesc {
    rx_resGroupLayout layout;
    // u32 dynBuffer0Idx;
    // u32 dynBuffer1Idx;
    rx_resGroupUsage usage;
    rx_ResGroupUpdateDesc initalContent;
} rx_ResGroupDesc;

API rx_resGroup rx_makeResGroup(rx_ResGroupDesc* desc);
API rx_resGroup rx_makeDynamicBufferResGroup(rx_buffer dynBuffer0, rx_buffer dynBuffer1);
API void rx_updateResGroup(rx_resGroup resGroup, rx_ResGroupUpdateDesc* desc);

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
    bool depthWriteEnabled;
    rx_compareFunc depthCompareFunc;
    bool stencilEnabled;
    rx_StencilState stencilFront;
    rx_StencilState stencilBack;
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
    S8 name;
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
    S8                    label;
    rx_primitiveTopology    primitiveTopology;
    rx_indexType            indexFormat;
    struct {
        rx_renderShader shader;
        S8            vsEntryPoint;
        S8            fsEntryPoint;
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

typedef enum rx_presentMode {
    rx_presentMode__default = 0,
    // The presentation of the image to the user waits for the next vertical blanking period to update in a first-in, first-out manner.
    // Tearing cannot be observed and frame-loop will be limited to the display's refresh rate.
    // This is the only mode that's always available.
    rx_presentMode_fifo,
    // The presentation of the image to the user tries to wait for the next vertical blanking period but may decide to not wait if a frame is presented late.
    // Tearing can sometimes be observed but late-frame don't produce a full-frame stutter in the presentation.
    // This is still a first-in, first-out mechanism so a frame-loop will be limited to the display's refresh rate.
    rx_presentMode_fifoRelaxed,
    // The presentation of the image to the user is updated immediately without waiting for a vertical blank.
    // Tearing can be observed but latency is minimized.
    rx_presentMode_immediate,
    rx_presentMode_mailbox,
    rx_presentMode__forceU32 = RX_U32_MAX
} rx_presentMode;


typedef enum rx_swapStrategy {
    rx_swapStrategy__default = 0,
    rx_swapStrategy_single,
    rx_swapStrategy_double,
    rx_swapStrategy_tripple,
} rx_swapStrategy;

typedef struct rx_SwapChainDesc {
    rx_presentMode presentMode;
    rx_swapStrategy swapStrategy;
    struct {
        void* hWnd;
    } windows;
} rx_SwapChainDesc;

API rx_swapChain rx_makeSwapChain(rx_SwapChainDesc* desc);
API rx_texture rx_getCurrentSwapTexture(rx_swapChain swapChain);

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
    rx_resGroup resGroupDynamicOffsetBuffers;
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
    rx_texture target;
    rx_loadOp loadOp;
    rx_storeOp storeOp;
    Rgba clearColor;
} rx_RenderPassColorTargetDesc;

typedef struct rx_RenderPassDesc {
    S8 name;
    rx_swapChain swapChain;
    rx_RenderPassColorTargetDesc colorTargets[RX_MAX_COLOR_TARGETS];
    struct {
        bx               active;
        rx_textureFormat format;
        rx_loadOp        depthLoadOp;
        f32              depthClearValue;
        rx_loadOp        stencilLoadOp;
        u8               stencilClearValue;
    } depthStencil;
    rx_sampleCount sampleCount;
    // these can be left to zero unless you are using the default (system) pass running on a legacy Gfx API (OpenGL, WebGL)
    f32 width;
    f32 height;
} rx_RenderPassDesc;

typedef struct rx_RenderPassResult {
    rx_texture colorTargets[RX_MAX_COLOR_TARGETS];
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
    rx_renderCmd_dynResGroup0      = 1 << 7,
    rx_renderCmd_dynResGroup1      = 1 << 8,
    rx_renderCmd_instanceOffset    = 1 << 9,
    rx_renderCmd_instanceCount     = 1 << 10,
} rx_renderCmds;

typedef struct rx_DrawList {
    flags64 passIdxDepFlags;
    u32 count;
    u32 commands[0];
} rx_DrawList;

// areas/drawList memory has to be kept alive till after rx_commit was called
API void rx_setRenderPassDrawList(rx_renderPass renderPass, rx_DrawArea* arenas, u32 areaCount, rx_DrawList* drawList);

#if 0
// TODO: compute pass
// - needs to access buffer and texture in read/write
// - Resgroups need to manage know which passes they read to but also which resources MAY BE WRITTEN 

LOCAL void runRenderPass(void) {
    rx_computePass computePass = rx_makeComputePass();

    rx_buffer myMutatedBuffer = rx_accesBufferAfterComputePass(computePass, myBuffer);

    rx_updateResGroup(resGroup, &(MyResGroupBlock) {.myWriteBuffer = myBuffer});
}
#endif



API void rx_commit(void);

#if 0
API void rx_setRenderPassCmds(rx_RenderPassCmds* cmds);
API void rx_setComputePassCmds(rx_ComputePassCmds* cmds);
#endif

#if 0
// Context provides all public functions as pointer on itself
// should be the first value that appears on rx_Ctx so it can be easily castet
typedef struct rx_Api {
    rx_renderPipeline (*makeRenderPipeline)(rx_RenderPipelineDesc* desc);
    rx_texture (*getCurrentSwapTexture)(void);
    void (*rx_setRenderPassDrawList)(rx_renderPass renderPass, rx_DrawArea* arenas, u32 areaCount, rx_DrawList* drawList);
    void (*commit)(void);
} rx_Api;
#endif

#ifdef __cplusplus
}
#endif