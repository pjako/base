#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_math.h"

#include <math.h>

i32 main(i32 argc, char* argv[]) {

    f32 logF = logf(38.26f);
    f32 logF32 = f32_log(38.26f);
    ASSERT(logF == logF32);

    f32 ldexpF = ldexpf(0.773183345f, 3.0f);
    f32 ldexpF32 = f32_ldexp(0.773183345f, 3.0f);
    ASSERT(ldexpF == ldexpF32);
    

    f32 expF = expf(0.5f * logF32);
    f32 expF32 = f32_exp(0.5f * logF32);
    ASSERT(expF == expF32);
    // 38.26 -0.5
    //INLINE f32 f32_pow(f32 num, f32 _b) {
    //    return f32_exp(_b * f32_log(num));
    //}


    f32 floorF = floorf(-0.15);
    f32 floorF32 = f32_floor(-0.15);
    ASSERT(floorF == floorF32);
    f32 cosF = cosf(1.334 - f32_piHalf);
    f32 cosF32 = f32_cos(1.334 - f32_piHalf);
    ASSERT(f32_equal(cosF, cosF32, 0.00001));


    f32 scosF = cosf(0.334 - f32_piHalf);
    f32 scosF32 = f32_cos(0.334 - f32_piHalf);
    ASSERT(f32_equal(scosF, scosF32, 0.00001));


    f32 sinF = sinf(0.334);
    f32 sinF32 = f32_sin(0.334);
    ASSERT(f32_equal(sinF, sinF32, 0.00001));


    return 0;
}


