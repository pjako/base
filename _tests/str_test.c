#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"

#include "os/os.h"

i32 main(i32 argc, char* argv[]) {
    BaseMemory baseMemory = os_getBaseMemory();

    Arena* arena = mem_makeArena(&baseMemory, MEGABYTE(1));



    Str8 str1 = str_fmt(arena, s8("{} {} {}"), 1, 2, 3);

    os_log(str1);

    mem_destroyArena(arena);
    return 0;
}


