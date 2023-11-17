#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"
#include "base/base_args.h"
#include "os/os.h"

#include "shd/shd.h"


i32 main(i32 argc, char* argv[]) {

// enable for debug
#if 0
    char* debugArgV[] = {
        (char*) "-s",
        (char*) PROJECT_ROOT "/engine_test/ui_shaders.hlsl",
        (char*) "-h",
        (char*) PROJECT_ROOT "/engine_test/ui_shaders.hlsl.h"
    };
    i32 debugArgc = countOf(debugArgV);

    argv = (char**) debugArgV;
    argc = debugArgc;
#endif

#if 0
    arg_Opt options[] = {
        {str8("shader"), 's', arg_optType_string, arg_flag_required | arg_flag_requiredValue, STR_EMPTY, str8("SHD shader file path."), str8("Absolute path to the shader file.")},
        {str8("header"), 'h', arg_optType_string, arg_flag_none, STR_EMPTY, str8("generated header output file path."), str8("Absolute path where the generated header should be created.")},
        {str8("prefix"), 'p', arg_optType_string, arg_flag_none, str_lit("shd_"), str8("prefix used in generated header"), str8("Add user defined prefix to variables and functions in the generated header.\nThe default shader is \"shd_\"")},
    };
#endif



    BaseMemory baseMem = os_getBaseMemory();
    Arena* mainArena = mem_makeArena(&baseMem, MEGABYTE(8));

    Str8 targetPath = s8("/Users/peterjakobs/pjako/base_kit/_examples/sample_texture.hlsl");

    Str8 fileContent = os_fileRead(mainArena, targetPath);

    Str8 headerFileContent = shd_generateShader(mainArena, targetPath, fileContent, shd_variation_ogl40);

    Str8 headerPath = s8("/Users/peterjakobs/pjako/base_kit/_examples/sample_texture.hlsl.h");

    os_writeFile(headerPath, headerFileContent);

    return 0;
}