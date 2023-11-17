#ifndef DXC_WRAPPER_H
#define DXC_WRAPPER_H

typedef enum dxc_shaderType {
    dxc_shaderType_vertex,
    dxc_shaderType_pixel,
    dxc_shaderType_compute,
} dxc_shaderType;

typedef enum dxc_binaryShader {
    dxc_binaryShader_spirv = 0,
    dxc_binaryShader_dxil
} dxc_binaryShader;

typedef struct StrPair {
    Str8 left;
    Str8 right;
} StrPair;

typedef struct DxCompiler DxCompiler;

API DxCompiler* dxc_create(Arena* arena);

typedef struct dxc_CompileOptions {
    struct {
        u32 minor;
        u32 major;
    } shaderModel;
    dxc_binaryShader binaryShaderType;
    u32 packMatricesInRowMajor;
    bx  enable16bitTypes;
    bx  enableDebugInfo;
    u32 disableOptimizations;
    u32 optimizationLevel;
    u32 shiftAllCBuffersBindings;
    u32 shiftAllUABuffersBindings;
    u32 shiftAllSamplersBindings;
    u32 shiftAllTexturesBindings;
    //dxil only
    bx asModule;
} dxc_CompileOptions;

typedef struct dxc_CompileDesc {
    DxCompiler* dxCompiler;
    dxc_shaderType type;
    Str8 source;
    Str8 inputFileName;
    Str8 entryPoint;
    StrPair* defines;
    u32 defineCount;
    Str8 (*includeLoadCallback)(Str8 includeName, void* userPtr);
    void* userPtr;
    dxc_CompileOptions options;
} dxc_CompileDesc;

typedef struct dxc_ReflectionResultDesc {
    Str8 descs; // The underneath type is ReflectionDesc
    uint32_t descCount = 0;
    uint32_t instructionCount = 0;
} dxc_ReflectionResultDesc;

typedef struct dxc_CompileResult {
    Str8 target;

    Str8 errorWarningMsg;
    bool hasError;

    dxc_ReflectionResultDesc reflection;
} dxc_CompileResult;

dxc_CompileResult dxc_compileHlslToSpv(Arena* arena, dxc_CompileDesc* desc);

#endif