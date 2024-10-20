#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <unknwn.h>
#include <windows.h>
#include <atlbase.h>
#define ModuleHandle HMODULE
#undef max
#undef min
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#else
#define __EMULATE_UUID 1
#include <arpa/inet.h>
#include "WinAdapter.h"
#include <dlfcn.h>
#define LoadLibraryA(name)   dlopen(name, RTLD_NOW)
#define GetProcAddress(h, n) dlsym(h, n)
#define FreeModule(h)        dlclose(h)
#define ModuleHandle         void*
#endif
#include "dxcapi.h"

#include <functional>
#include <type_traits>

#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"

#include "os/os.h"
//#include "app/app.h" 
#include "log/log.h"

#include "dxc_wrapper.h"

#include "assert.h"


#if COMPILER_MSVC
#define str_toStrWchar str_toS16
#else
#define str_toStrWchar str_toS32
#endif

#if __cplusplus > 201402L
// #error "This code requires at least C++17"
#endif

#define DXC_FAILED(hr) (((HRESULT)(hr)) < 0)
#define IFT(x)      { HRESULT __hr = (x); if (DXC_FAILED(__hr)) ASSERT(!"FAILED!"); }


class DxCompiler {
public:
    CComPtr<IDxcLinker> CreateLinker() const {
        CComPtr<IDxcLinker> linker;
        IFT(m_createInstanceFunc(CLSID_DxcLinker, __uuidof(IDxcLinker), reinterpret_cast<void**>(&linker)));
        return linker;
    }
    DxCompiler(Arena* arena) {
        mem_scoped(mem, arena) {
            //execPath = s8(""); // os_execPath(mem.arena);
            S8 binaryPath = os_filepath(mem.arena, os_systemPath_binary);
            S8 paths[] = {
                str_join(mem.arena, binaryPath, s8("/"), s8("dxcompiler")),
                str_join(mem.arena, binaryPath, s8("/"), s8("libdxcompiler"), s8(".dylib")),
                s8("libdxcompiler.dylib"),
                s8("/dxcompiler.dll"),
                s8("/Users/peterjakobs/pjako/base_kit/bin_tools/libdxcompiler.so"),
                // str_join(mem.arena, execPath, s8("/"), s8("libdxcompiler"), s8(".dylib")),
                // str_join(mem.arena, execPath, s8("/"), s8("dxcompiler"), s8(".dll")),
                // str_join(mem.arena, execPath, s8("/"), s8("libdxcompiler"), s8(".so")),
                // s8("libdxcompiler"),
                // s8("libdxcompiler.dylib"),
                // s8("libdxcompiler.so"),
                // s8("dxcompiler.dll")
            };

            u32 count = countOf(paths);
            for (u32 idx = 0; idx < count; idx++) {
                this->m_dxcompilerDll = (HMODULE) os_dlOpen(paths[idx]);
                if (this->m_dxcompilerDll != nullptr) break;
            }
        }

        assert(this->m_dxcompilerDll);

        if (!this->m_dxcompilerDll) {
            return;
        }

        m_createInstanceFunc = (DxcCreateInstanceProc) os_dlSym((os_Dl*) this->m_dxcompilerDll, "DxcCreateInstance");

        if (m_createInstanceFunc != nullptr) {
                IFT(m_createInstanceFunc(CLSID_DxcLibrary, __uuidof(IDxcUtils), reinterpret_cast<void**>(&m_utils)));
                IFT(m_createInstanceFunc(CLSID_DxcCompiler, __uuidof(IDxcCompiler), reinterpret_cast<void**>(&m_compiler)));
                IFT(m_createInstanceFunc(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection),
                                            reinterpret_cast<void**>(&m_containerReflection)));
        } else {
            //this->Destroy();

            //throw std::runtime_error(std::string("COULDN'T get ") + functionName + " from dxcompiler.");
        }

        //m_linkerSupport = (CreateLinker() != nullptr);

    }

public:
    virtual ~DxCompiler() {
        ASSERT(!"Wrong!");
    }
    const char* fooo = "DONE SO!";
    HMODULE m_dxcompilerDll = nullptr;
    DxcCreateInstanceProc m_createInstanceFunc = nullptr;

    CComPtr<IDxcUtils> m_utils = nullptr;
    CComPtr<IDxcCompiler> m_compiler = nullptr;
    CComPtr<IDxcContainerReflection> m_containerReflection = nullptr;
    bool m_linkerSupport = false;
};

#if 0
class ScIncludeHandler : public IDxcIncludeHandler {
public:
    explicit ScIncludeHandler(DxCompiler* dxCompiler, Arena* tmpArena, S8 (*loadIncludeCallback)(S8 includeName, void* userPtr), void* userPtr) :
        m_dxCompiler(dxCompiler), m_tmpArena(tmpArena), m_loadIncludeCallback(loadIncludeCallback), m_userPtr(userPtr) {
        
    }
    virtual ~ScIncludeHandler() {}

    HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR fileName, IDxcBlob** includeSource) override {
        if ((fileName[0] == L'.') && (fileName[1] == L'/'))
        {
            fileName += 2;
        }
        S16 fileNameU16;
        fileNameU16.size = wcslen(fileName);
        fileNameU16.content = (u16*) fileName;
        S8 fileNameU8 = str_fromS16(this->m_tmpArena, fileNameU16);
        std::string utf8FileName;

        S8 fileContent = m_loadIncludeCallback(fileNameU8, this->m_userPtr);

        *includeSource = nullptr;
        return this->m_dxCompiler->m_utils->CreateBlobWithEncodingOnHeapCopy(fileContent.content, fileContent.size, CP_UTF8,
                                                                                    reinterpret_cast<IDxcBlobEncoding**>(includeSource));
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        ++m_ref;
        return m_ref;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        --m_ref;
        ULONG result = m_ref;
        if (result == 0)
        {
            delete this;
        }
        return result;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override
    {
        if (IsEqualIID(iid, __uuidof(IDxcIncludeHandler)))
        {
            *object = dynamic_cast<IDxcIncludeHandler*>(this);
            this->AddRef();
            return S_OK;
        }
        else if (IsEqualIID(iid, __uuidof(IUnknown)))
        {
            *object = dynamic_cast<IUnknown*>(this);
            this->AddRef();
            return S_OK;
        }
        else
        {
            return E_NOINTERFACE;
        }
    }

private:
    DxCompiler* m_dxCompiler = nullptr;
    Arena* m_tmpArena = nullptr;
    S8 (*m_loadIncludeCallback)(S8 includeName, void* userPtr) = nullptr;
    void* m_userPtr = nullptr;
    std::atomic<ULONG> m_ref = 0;
};
#endif

void rx__convertDxcResult(Arena* arena, dxc_CompileResult& result, IDxcOperationResult* dxcResult, dxc_binaryShader targetLanguage, bool asModule) {
    HRESULT status;
    IFT(dxcResult->GetStatus(&status));

    result.errorWarningMsg.content = NULL;
    result.errorWarningMsg.size = 0;

    result.target.content = NULL;
    result.target.size = 0;

    CComPtr<IDxcBlobEncoding> errors;
    IFT(dxcResult->GetErrorBuffer(&errors));
    
    if (errors != nullptr) {
        //BOOL known = false;
        //UINT32 pCodePage = 0;
        //errors->GetEncoding(&known, &pCodePage);
        result.errorWarningMsg.size = (u64) errors->GetBufferSize();
        if (result.errorWarningMsg.size > 0) {
            result.errorWarningMsg.content = (u8*) mem_arenaPush(arena, result.errorWarningMsg.size);
            mem_setZero(result.errorWarningMsg.content, result.errorWarningMsg.size);
            mem_copy(result.errorWarningMsg.content, (u8*) errors->GetBufferPointer(), result.errorWarningMsg.size);
        }
        errors = nullptr;
    }

    result.hasError = true;
    if (SUCCEEDED(status)) {
        CComPtr<IDxcBlob> program;
        IFT(dxcResult->GetResult(&program));
        dxcResult = nullptr;
        if (program != nullptr) {
            result.target.size = (u64) program->GetBufferSize();
            result.target.content = (u8*) mem_arenaPush(arena, result.target.size);
            mem_copy(result.target.content, (u8*) program->GetBufferPointer(), result.target.size);
            result.hasError = false;
        }

#if OS_WINDOWS
        if ((targetLanguage == dxc_binaryShader_dxil) && !asModule) {
            // Gather reflection information only for ShadingLanguage::Dxil
            ShaderReflection(result.reflection, program);
        }
#else
        unusedVars(targetLanguage);
        unusedVars(asModule);
#endif
    }
}


DxCompiler* dxc_create(Arena* arena) {
    ASSERT(arena);
    static DxCompiler* compiler;
    if (!compiler) {
        compiler = new DxCompiler(arena);
    }
    return compiler;
}

dxc_CompileResult dxc_compileHlslToSpv(Arena* arena, dxc_CompileDesc* desc) {
    ASSERT(arena && desc);
    ASSERT(desc->dxCompiler);
    ASSERT(desc->source.content && desc->source.size > 0);
    dxc_CompileResult result = {0};
    result.hasError = true;
    dxc_CompileOptions options = desc->options;
    CComPtr<IDxcBlobEncoding> sourceBlob = nullptr;

    IFT(desc->dxCompiler->m_utils->CreateBlobFromPinned( // DXC_CP_ACP DXC_CP_UTF8
            desc->source.content, (u32) desc->source.size, DXC_CP_UTF8, &sourceBlob)
    );

    //char ff[] = {'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a'};
    //CComPtr<IDxcBlobEncoding> sourceBlob2 = nullptr;
    //IFT(desc->dxCompiler->m_utils->CreateBlobFromPinned(
    //    (u8*) ff, (u32) sizeof(ff), DXC_CP_ACP, &sourceBlob2)
    //);
    u32 sourceBlobSize = sourceBlob->GetBufferSize();
    ASSERT(sourceBlobSize >= 4);
    //return dxcInstance->libraryInstance->CreateBlobWithEncodingFromPinned(source.content, (u32) source.size, 0, ptr);

    CComPtr<IDxcOperationResult> compileResult = nullptr;

    mem_scoped(mem, arena) {

        u32 minor = 1;
        u32 major = 6;

        if (options.shaderModel.major != 0) {
            minor = options.shaderModel.minor;
            major = options.shaderModel.major;
        }

        S8 targetProfile = [desc]() {
            switch (desc->type) {
                case dxc_shaderType_vertex:     return s8("vs");
                case dxc_shaderType_pixel:      return s8("ps");
                case dxc_shaderType_compute:    return s8("cs");
                default: s8("");
            }
            return s8("");
        }();

        if (options.asModule) {
            ASSERT(options.binaryShaderType == dxc_binaryShader_dxil && "Spirv does not support modules");
            targetProfile = s8("lib_6_x");
        } else {
            targetProfile = str_join(arena, targetProfile, s8("_"), major, s8("_"), minor);
        }


        DxcDefine* dxcDefines = nullptr;
        if (desc->defineCount > 0) {
            dxcDefines = mem_arenaPushArray(mem.arena, DxcDefine, desc->defineCount);
            for (u32 idx = 0; idx < desc->defineCount; idx++) {
                dxcDefines[idx] = DxcDefine {(wchar_t*) str_toStrWchar(mem.arena, desc->defines[idx].left).content, desc->defines[idx].right.size == 0 ? nullptr : ((wchar_t*) str_toStrWchar(mem.arena, desc->defines[idx].right).content)};
            }
        }
        wchar_t** wParams = mem_arenaPushArray(mem.arena, wchar_t*, 36);
        u32 paramCount = 0;

        // HLSL matrices are translated into SPIR-V OpTypeMatrixs in a transposed manner,
        // See also https://web.archive.org/web/20201014022629/https://antiagainst.github.io/post/hlsl-for-vulkan-matrices/
        if (options.packMatricesInRowMajor) {
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("-Zpr")).content;
        } else {
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("-Zpc")).content;
        }

        if (options.enable16bitTypes) {
            if (major > 6 || (major == 6 && minor >= 2)) {
                wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("-enable-16bit-types")).content;
            } else {
                ASSERT("16-bit types requires shader model 6.2 or up.");
            }
        }

        if (true) {
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("-Zi")).content;
        }

        if (options.disableOptimizations) {
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("-Od")).content;
        } else {
            if (options.optimizationLevel < 4)  {
                wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, str_join(arena, s8("-O"), options.optimizationLevel, s8("\0"))).content;
            } else {
                ASSERT("Invalid optimization level.");
            }
        }



        if (options.shiftAllCBuffersBindings > 0) {
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("-fvk-b-shift")).content;
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, str_join(arena, options.shiftAllCBuffersBindings)).content;
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("all")).content;
        }

        if (options.shiftAllUABuffersBindings > 0) {
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("-fvk-u-shift")).content;
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, str_join(arena, options.shiftAllUABuffersBindings)).content;
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("all")).content;
        }

        if (options.shiftAllSamplersBindings > 0) {
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("-fvk-s-shift")).content;
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, str_join(arena, options.shiftAllSamplersBindings)).content;
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("all")).content;
        }

        if (options.shiftAllTexturesBindings > 0) {
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("-fvk-t-shift")).content;
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, str_join(arena, options.shiftAllTexturesBindings)).content;
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("all")).content;
        }

        if (options.binaryShaderType == dxc_binaryShader_spirv) {
            wParams[paramCount++] = (wchar_t*) str_toStrWchar(mem.arena, s8("-spirv")).content;
        }

        const wchar_t* wInputFileName = (const wchar_t*) str_toStrWchar(mem.arena, desc->inputFileName).content;
        const wchar_t* wEntryPoint = (const wchar_t*) str_toStrWchar(mem.arena, desc->entryPoint).content;
        const wchar_t* wTargetPofile = (const wchar_t*) str_toStrWchar(mem.arena, targetProfile).content;
#if 0
        CComPtr<IDxcIncludeHandler> includeHandler = new ScIncludeHandler(
            desc->dxCompiler,
            arena,
            desc->includeLoadCallback,
            desc->userPtr
        );
#endif
        int size = sizeof(wchar_t);
        // log_debug(arena, desc->source);
        HRESULT dxCompileResult = desc->dxCompiler->m_compiler->Compile(
            sourceBlob,
            wInputFileName,
            wEntryPoint,
            wTargetPofile,
            (const wchar_t**) wParams,
            paramCount,
            dxcDefines,
            desc->defineCount,
            nullptr,
            &compileResult
        );
        if (DXC_FAILED(dxCompileResult)) ASSERT(!"FAILED!");
#if 0
  virtual HRESULT STDMETHODCALLTYPE Compile(
      _In_ IDxcBlob *pSource,         // Source text to compile.
      _In_opt_z_ LPCWSTR pSourceName, // Optional file name for pSource. Used in
                                      // errors and include handlers.
      _In_opt_z_ LPCWSTR pEntryPoint, // Entry point name.
      _In_z_ LPCWSTR pTargetProfile,  // Shader profile to compile.
      _In_opt_count_(argCount)
          LPCWSTR *pArguments, // Array of pointers to arguments.
      _In_ UINT32 argCount,    // Number of arguments.
      _In_count_(defineCount) const DxcDefine *pDefines, // Array of defines.
      _In_ UINT32 defineCount,                           // Number of defines.
      _In_opt_ IDxcIncludeHandler
          *pIncludeHandler, // User-provided interface to handle #include
                            // directives (optional).
      _COM_Outptr_ IDxcOperationResult *
          *ppResult // Compiler output status, buffer, and errors.
      ) = 0;
#endif
    }

    rx__convertDxcResult(arena, result, compileResult, options.binaryShaderType, options.asModule);

    return result;
}
