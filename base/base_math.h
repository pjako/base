#ifndef _BASE_MATH_
#define _BASE_MATH_
#ifdef __cplusplus
extern "C" {
#endif
static const i16 i16_max        = 0xFFFF;
static const u16 u16_max        = 0xFFFF;
static const i32 i32_max        = 0x7FFFFFFF;
static const u32 u32_max        = 0x7FFFFFFF;
static const i64 i64_max        = 0xFFFFFFFFFFFFFFFF;
static const u64 u64_max        = 0xFFFFFFFFFFFFFFFF;

static const f32 f32_pi         = 3.1415926535897932384626433832795f;
static const f32 f32_pi2        = 6.2831853071795864769252867665590f;
static const f32 f32_invPi      = 0.3183098733425140380859375f; // 1.0f / f32_pi;
static const f32 f32_piHalf     = 1.5707963267948966192313216916398f;
static const f32 f32_piQuarter  = 0.7853981633974483096156608458199f;
static const f32 f32_sqrt2      = 1.4142135623730950488016887242097f;
static const f32 f32_logNat10   = 2.3025850929940456840179914546844f;
static const f32 f32_invLogNat2 = 1.4426950408889634073599246810019f;
static const f32 f32_logNat2Hi  = 0.6931471805599453094172321214582f;
static const f32 f32_logNat2Lo  = 1.90821492927058770002e-10f;
static const f32 f32_e          = 2.7182818284590452353602874713527f;
static const f32 f32_nearZero   = 0.0000000037252902984619140625f; // 1.0f / f32_cast(1 << 28);
static const f32 f32_min        = 1.175494e-38f;
static const f32 f32_max        = 3.402823e+38f;

static const f16 f16_infinity = {0x7C00};

static const u8  f32_signNumBits      = 1;
static const u8  f32_signBitShift     = 31;
static const u32 f32_signMask         = u32_val(0x80000000);
static const u8  f32_exponentNumBits  = 8;
static const u8  f32_exponentBitShift = 23;
static const u32 f32_exponentMask     = u32_val(0x7f800000);
static const u32 f32_exponentBias     = 127;
static const u8  f32_mantissaNumBits  = 23;
static const u8  f32_mantissaBitShift = 0;
static const u32 f32_mantissaMask     = u32_val(0x007fffff);

// Smallest normalized positive floating-point number.
static const f32 f32_smallestValue  = 1.175494351e-38f;

// Maximum representable floating-point number.
static const f32 f32_largestValue   = 3.402823466e+38f;

static const union { u32 __valU32; f32 value; } f32__infinity = { 255 << 23 };//{0x7f800000};
#define f32_infinity (f32__infinity.value)
#define f32_infinityNegative (-f32__infinity.value)

static const union { u32 __valU32; f32 value; } f32__nan = { f32_exponentMask | f32_mantissaMask };
#define f32_nan (f32__nan.value)

FORCE_INLINE f16 f16_fromF32(f32 input) {
    f16 o = { 0 };

    union {
        u32 u;
        f32 f;
        struct {
            u32 mantissa : 23;
            u32 exponent : 8;
            u32 sign : 1;
        };
    } f;
    f.f = input;

    // Based on ISPC reference code (with minor modifications)
    if (f.exponent == 255) {// Inf or NaN (all exponent bits set)
        o.exponent = 31;
        o.mantissa = f.mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
    } else { // Normalized number
        // Exponent unbias the single, then bias the halfp
        int newexp = f.exponent - 127 + 15;
        if (newexp >= 31) { // Overflow, return signed infinity
            o.exponent = 31;
        } else if (newexp <= 0) {// Underflow
            if ((14 - newexp) <= 24) {// Mantissa might be non-zero
                u32 mant = f.mantissa | 0x800000; // Hidden 1 bit
                o.mantissa = mant >> (14 - newexp);
                if ((mant >> (13 - newexp)) & 1) {// Check for rounding
                    o.val++; // Round, might overflow into exp bit, but this is OK
                }
            }
        } else {
            o.exponent = newexp;
            o.mantissa = f.mantissa >> 13;
            if (f.mantissa & 0x1000) {// Check for rounding
                o.val++; // Round, might overflow to inf, this is OK
            }
        }
    }

    o.sign = f.sign;
    return o;
}

FORCE_INLINE bx f16_isInfinity(f16 input) {
    return input.exponent == f16_infinity.exponent && input.mantissa == f16_infinity.mantissa;
}


#ifdef SIMD_USE_SSE

#ifndef USE_SIMD
#define USE_SIMD 1
#endif


// SSE SIMD (x86)

FORCE_INLINE f32x4 f32x4_make(f32 x, f32 y, f32 z, f32 w) {
    f32x4 result;
    result.simd = _mm_setr_ps(x, y, z, w);
    return result;
}

FORCE_INLINE f32x4 f32x4_add(f32x4 left, f32x4 right) {
    f32x4 result;
    result.simd = _mm_add_ps(left, right);
    return result;
}

FORCE_INLINE f32x4 f32x4_sub(f32x4 left, f32x4 right) {
    f32x4 result;
    result.simd = _mm_sub_ps(left, right);
    return result;
}

FORCE_INLINE f32x4 f32x4_cos(f32x4 in) {
    float32x4_t result;
    result.simd = _mm_cos_ps(in.simd);
    return result;
}

FORCE_INLINE f16x4 f16x4_fromF32x4(f32x4 from) {
    __m128i mask_fabs  = _mm_set1_epi32(0x7fffffff);
    __m128i c_f32infty = _mm_set1_epi32((255 << 23));
    __m128i c_expinf   = _mm_set1_epi32((255 ^ 31) << 23);
    __m128i c_f16max   = _mm_set1_epi32((127 + 16) << 23);
    __m128i c_magic    = _mm_set1_epi32(15 << 23);

    __m128  mabs        = _mm_castsi128_ps(mask_fabs);
    __m128  fabs        = _mm_and_ps(mabs, from.simd);
    __m128  justsign    = _mm_xor_ps(from.simd, fabs);

    __m128  f16max      = _mm_castsi128_ps(c_f16max);
    __m128  expinf      = _mm_castsi128_ps(c_expinf);
    __m128  infnancase  = _mm_xor_ps(expinf, fabs);
    __m128  clamped     = _mm_min_ps(f16max, fabs);
    __m128  b_notnormal = _mm_cmpnlt_ps(fabs, _mm_castsi128_ps(c_f32infty));
    __m128  scaled      = _mm_mul_ps(clamped, _mm_castsi128_ps(c_magic));

    __m128  merge1      = _mm_and_ps(infnancase, b_notnormal);
    __m128  merge2      = _mm_andnot_ps(b_notnormal, scaled);
    __m128  merged      = _mm_or_ps(merge1, merge2);

    __m128i shifted     = _mm_srli_epi32(_mm_castps_si128(merged), 13);
    __m128i signshifted = _mm_srli_epi32(_mm_castps_si128(justsign), 16);

    union {
        struct {
            f16x4 f16x4;
            f16x4 __f16x4;
        };
        i32x4 i32x4;

    } result;
    result.i32x4.simd = _mm_or_si128(shifted, signshifted);
    return result.f16x4;
}

#elif defined(SIMD_USE_NEON)

#ifndef USE_SIMD
#define USE_SIMD 1
#endif

// Neon SIMD (Arm)

FORCE_INLINE f32x4 f32x4_make(f32 x, f32 y, f32 z, f32 w) {
    ALIGN_DECL(16, f32) data[4] = {w, z, y, x};
    f32x4 result;
    result.simd = vld1q_f32(data);
    return result;
}

FORCE_INLINE f32x4 f32x4_add(f32x4 left, f32x4 right) {
    f32x4 result;
    result.simd = vaddq_f32(left.simd, right.simd);
    return result;
}

FORCE_INLINE f32x4 f32x4_sub(f32x4 left, f32x4 right) {
    f32x4 result;
    result.simd = vsubq_f32(left.simd, right.simd);
    return result;
}

FORCE_INLINE f16x4 f16x4_fromF32x4(f32x4 from) {
    f16x4 result;
    result.simd = vcvt_f16_f32(from.simd);
    return result;
}

FORCE_INLINE f32x4 f32x4_cos(f32x4 in) {
    float32x4_t result;
    result.simd = vcosq_f32(in.simd);
    return result;
}

#else

#ifndef USE_SIMD
#define USE_SIMD 0
#endif

// Fallback non simd version

FORCE_INLINE f32x4 f32x4_make(f32 x, f32 y, f32 z, f32 w) {
    f32x4 result;
    result.values[0] = x;
    result.values[1] = y;
    result.values[2] = z;
    result.values[3] = w;
    return result;
}

FORCE_INLINE f32x4 f32x4_add(f32x4 left, f32x4 right) {
    f32x4 result;
    result.values[0] = left.values[0] + right.values[0];
    result.values[1] = left.values[1] + right.values[1];
    result.values[2] = left.values[2] + right.values[2];
    result.values[3] = left.values[3] + right.values[3];
    return result;
}

FORCE_INLINE f32x4 f32x4_sub(f32x4 left, f32x4 right) {
    f32x4 result;
    result.values[0] = left.values[0] - right.values[0];
    result.values[1] = left.values[1] - right.values[1];
    result.values[2] = left.values[2] - right.values[2];
    result.values[3] = left.values[3] - right.values[3];
    return result;
}

FORCE_INLINE f16x4 f16x4_fromF32x4(f32x4 from) {
    f16x4 result;
    result.values[0] = f16_fromF32(from.values[0]);
    result.values[1] = f16_fromF32(from.values[1]);
    result.values[2] = f16_fromF32(from.values[2]);
    result.values[3] = f16_fromF32(from.values[3]);
    return result;
}

#if 0
FORCE_INLINE f32x4 f32x4_cos(f32x4 in) {
    float32x4_t result;
    result.values[0] = f32_cos(in.values[0]);
    result.values[1] = f32_cos(in.values[1]);
    result.values[2] = f32_cos(in.values[2]);
    result.values[3] = f32_cos(in.values[3]);
    return result;
}
#endif

#endif

INLINE f32 f32_equal(f32 val, f32 cmp, f32 delta) {
    return ((((val)-(cmp))> -(delta))&&(((val)-(cmp))<(delta)));
}

INLINE f32 f32_fromBits(u32 bits) {
    union { u32 u32; f32 f32;} result;
    result.u32 = bits;
    return result.f32;
}

INLINE u32 f32_toBits(f32 val) {
    union { u32 u32; f32 f32;} result;
    result.f32 = val;
    return result.u32;
}

INLINE f32 f32_trunc(f32 a) {
    return f32_cast(i32_cast(a));
}

INLINE f32 f32_frac(f32 a) {
    return a - f32_trunc(a);
}

INLINE bx f32_isNaN(f32 a) {
    const u32 tmp = f32_toBits(a) & i32_max;
    return tmp > u32_cast(0x7f800000);
}

INLINE bx f32_isInfite(f32 a) {
    const u32 tmp = f32_toBits(a) & i32_max;
    return tmp == u32_cast(0x7f800000);
}

INLINE bx f32_isFinite(f32 a) {
    const u32 tmp = f32_toBits(a) & i32_max;
    return tmp < u32_cast(0x7f800000);
}

INLINE f32 f32_toRad(f32 deg) {
    return deg * f32_pi / 180.0f;
}

INLINE f32 f32_toDeg(f32 rad) {
    return rad * 180.0f / f32_pi;
}

INLINE f32 f32_floor(f32 a) {
    if (a < 0.0f) {
        const f32 fr = f32_frac(-a);
        const f32 result = - a - fr;

        return -(0.0f != fr ? result + 1.0f : result);
    }

    return a - f32_frac(a);
}

INLINE f32 f32_ceil(f32 a) {
    return -f32_floor(-a);
}

INLINE f32 f32_round(f32 a) {
    return f32_floor(a + 0.5f);
}

INLINE f32 f32_sign(f32 a) {
    return a < 0.0f ? -1.0f : 1.0f;
}

INLINE f32 f32_abs(f32 a) {
    return a < 0.0f ? -a : a;
}

INLINE f32 f32_square(f32 a) {
    return a * a;
}

INLINE f32 f32_mad(f32 a, f32 b, f32 c) {
    return (a * b) + c;
}


INLINE i32 i32_random(i32 min, i32 max, u32* seed) {
    ASSERT(seed);
    if (min > max) {
        i32 temp = min;
        min = max;
        max = temp;
    }

    u32 range = max - min + 1;
    *seed ^= (*seed << 13);
    *seed ^= (*seed >> 17);
    *seed ^= (*seed << 5);
    return min + (i32)(i32_cast(*seed) % range);
}

INLINE f32 f32_random(f32 min, f32 max, u32* seed) {
    ASSERT(seed);
    if (min > max) {
        f32 temp = min;
        min = max;
        max = temp;
    }

    *seed ^= (*seed << 13);
    *seed ^= (*seed >> 17);
    *seed ^= (*seed << 5);
    f32 randomValue = (f32) (*seed);
    f32 normalizedValue = randomValue / f32_cast(u32_max);

    return min + normalizedValue * (max - min);
}

#define u32_and(A, B)    u32_cast(u32_cast(A) & u32_cast(B))
#define u32_or(A, B)     u32_cast(u32_cast(A) | u32_cast(B))
#define u32_sra(A, STR)  u32_cast(i32_cast(A) >> (STR))
#define u32_i32Add(A, B) u32_cast(i32_cast(A) + i32_cast(B))
#define u32_sll(A, SA)   u32_cast(u32_cast(A) << (SA))
#define u32_srl(A, SA)   u32_cast(u32_cast(A) >> (SA))

INLINE f32 f32_ldexp(f32 num, i32 _b) {
    const u32 ftob        = f32_toBits(num);
    const u32 masked      = u32_and(ftob, f32_signMask | f32_exponentMask);
    const u32 expsign0    = u32_sra(masked, 23);
    const u32 tmp         = u32_i32Add(expsign0, _b);
    const u32 expsign1    = u32_sll(tmp, 23);
    const u32 mantissa    = u32_and(ftob, f32_mantissaMask);
    const u32 bits        = u32_or(mantissa, expsign1);
    const f32    result   = f32_fromBits(bits);

    return result;
}

INLINE f32 f32_exp(f32 num) {
    static const f32 f32__expC0  =  1.66666666666666019037e-01f;
    static const f32 f32__expC1  = -2.77777777770155933842e-03f;
    static const f32 f32__expC2  =  6.61375632143793436117e-05f;
    static const f32 f32__expC3  = -1.65339022054652515390e-06f;
    static const f32 f32__expC4  =  4.13813679705723846039e-08f;

    if (f32_abs(num) <= f32_nearZero) {
        return num + 1.0f;
    }

    const f32 kk     = f32_round(num*f32_invLogNat2);
    const f32 hi     = num - kk*f32_logNat2Hi;
    const f32 lo     =       kk*f32_logNat2Lo;
    const f32 hml    = hi - lo;
    const f32 hmlsq  = f32_square(hml);
    const f32 tmp0   = f32_mad(f32__expC4, hmlsq, f32__expC3);
    const f32 tmp1   = f32_mad(tmp0,   hmlsq, f32__expC2);
    const f32 tmp2   = f32_mad(tmp1,   hmlsq, f32__expC1);
    const f32 tmp3   = f32_mad(tmp2,   hmlsq, f32__expC0);
    const f32 tmp4   = hml - hmlsq * tmp3;
    const f32 tmp5   = hml*tmp4/(2.0f-tmp4);
    const f32 tmp6   = 1.0f - ( (lo - tmp5) - hi);
    const f32 result = f32_ldexp(tmp6, i32_cast(kk) );

    return result;
}

INLINE f32 f32_frexp(f32 num, i32* _outExp) {
    const u32 ftob     = f32_toBits(num);
    const u32 masked0  = u32_and(ftob, u32_cast(0x7f800000) );
    const u32 exp0     = u32_srl(masked0, 23);
    const u32 masked1  = u32_and(ftob,   u32_cast(0x807fffff) );
    const u32 bits     = u32_or(masked1, u32_cast(0x3f000000) );
    const f32 result   = f32_fromBits(bits);

    *_outExp = i32_cast(exp0 - 0x7e);

    return result;
}

INLINE f32 f32_log(f32 num) {
    static const f32 f32__logC0 = 6.666666666666735130e-01f;
    static const f32 f32__logC1 = 3.999999999940941908e-01f;
    static const f32 f32__logC2 = 2.857142874366239149e-01f;
    static const f32 f32__logC3 = 2.222219843214978396e-01f;
    static const f32 f32__logC4 = 1.818357216161805012e-01f;
    static const f32 f32__logC5 = 1.531383769920937332e-01f;
    static const f32 f32__logC6 = 1.479819860511658591e-01f;

    int32_t exp;
    f32 ff = f32_frexp(num, &exp);
    if (ff < f32_sqrt2*0.5f) {
        ff *= 2.0f;
        --exp;
    }

    ff -= 1.0f;
    const f32 kk     = f32_cast(exp);
    const f32 hi     = kk*f32_logNat2Hi;
    const f32 lo     = kk*f32_logNat2Lo;
    const f32 ss     = ff / (2.0f + ff);
    const f32 s2     = f32_square(ss);
    const f32 s4     = f32_square(s2);

    const f32 tmp0   = f32_mad(f32__logC6, s4, f32__logC4);
    const f32 tmp1   = f32_mad(tmp0,   s4, f32__logC2);
    const f32 tmp2   = f32_mad(tmp1,   s4, f32__logC0);
    const f32 t1     = s2*tmp2;

    const f32 tmp3   = f32_mad(f32__logC5, s4, f32__logC3);
    const f32 tmp4   = f32_mad(tmp3,   s4, f32__logC1);
    const f32 t2     = s4*tmp4;

    const f32 t12    = t1 + t2;
    const f32 hfsq   = 0.5f * f32_square(ff);
    const f32 result = hi - ( (hfsq - (ss*(hfsq+t12) + lo) ) - ff);

    return result;
}


INLINE f32 f32_pow(f32 num, f32 _b) {
    return f32_exp(_b * f32_log(num));
}

INLINE f32 f32_sqrt(f32 num) {
    if (num < 0.0f) {
        ASSERT(!"num is negative, you probably never want this...");
        return f32_nan;
    } else if (num < f32_nearZero) {
        return 0;
    }
#ifdef SIMD_USE_SSE
    __m128 In = _mm_set_ss(num);
    __m128 Out = _mm_sqrt_ss(In);
    return _mm_cvtss_f32(Out);
#else
    return num * f32_pow(num, -0.5f);
#endif
}

INLINE f32 f32_mod(f32 a, f32 b) {
    return a - b * f32_floor(a / b); 
}

INLINE f32 f32_cos(f32 a) {
#if USE_SIMD
    f32x4 inx4 = f32x4_make(a, 0, 0, 0);
    return f32x4_cos(inx4).x;
#else
    static const f32 f32__sinC2  = -0.16666667163372039794921875f;
    static const f32 f32__sinC4  =  8.333347737789154052734375e-3f;
    static const f32 f32__sinC6  = -1.9842604524455964565277099609375e-4f;
    static const f32 f32__sinC8  =  2.760012648650445044040679931640625e-6f;
    static const f32 f32__sinC10 = -2.50293279435709337121807038784027099609375e-8f;

    static const f32 f32__cosC2  = -0.5f;
    static const f32 f32__cosC4  =  4.166664183139801025390625e-2f;
    static const f32 f32__cosC6  = -1.388833043165504932403564453125e-3f;
    static const f32 f32__cosC8  =  2.47562347794882953166961669921875e-5f;
    static const f32 f32__cosC10 = -2.59630184018533327616751194000244140625e-7f;

    const f32 scaled = a * 2.0f * f32_invPi;
    const f32 real   = f32_floor(scaled);
    const f32 xx     = a - real * f32_piHalf;
    const i32 reali  = i32_cast(real);
    const i32 bits   = reali & 3;

    f32 c0, c2, c4, c6, c8, c10;

    if (bits == 0 || bits == 2) {
        c0  = 1.0f;
        c2  = f32__cosC2;
        c4  = f32__cosC4;
        c6  = f32__cosC6;
        c8  = f32__cosC8;
        c10 = f32__cosC10;
    } else {
        c0  = xx;
        c2  = f32__sinC2;
        c4  = f32__sinC4;
        c6  = f32__sinC6;
        c8  = f32__sinC8;
        c10 = f32__sinC10;
    }

    const f32 xsq    = f32_square(xx);
    const f32 tmp0   = f32_mad(c10,  xsq, c8 );
    const f32 tmp1   = f32_mad(tmp0, xsq, c6 );
    const f32 tmp2   = f32_mad(tmp1, xsq, c4 );
    const f32 tmp3   = f32_mad(tmp2, xsq, c2 );
    const f32 tmp4   = f32_mad(tmp3, xsq, 1.0);
    const f32 result = tmp4 * c0;

    return (bits == 1 || bits == 2) ? -result : result;
#endif
}

INLINE f32 f32_sin(f32 a) {
    return f32_cos(a - f32_piHalf);
}

INLINE f32 f32_acos(f32 num) {
    static const f32 f32__aCosC0 =  1.5707288f;
    static const f32 f32__aCosC1 = -0.2121144f;
    static const f32 f32__aCosC2 =  0.0742610f;
    static const f32 f32__aCosC3 = -0.0187293f;

    const f32 absa   = absVal(num);
    const f32 tmp0   = f32_mad(f32__aCosC3, absa, f32__aCosC2);
    const f32 tmp1   = f32_mad(tmp0,    absa, f32__aCosC1);
    const f32 tmp2   = f32_mad(tmp1,    absa, f32__aCosC0);
    const f32 tmp3   = tmp2 * f32_sqrt(1.0f - absa);
    const f32 negate = f32_cast(num < 0.0f);
    const f32 tmp4   = tmp3 - 2.0f*negate*tmp3;
    const f32 result = negate * f32_pi + tmp4;

    return result;
}

INLINE f32 f32_tan(f32 a) {
    return f32_sin(a) / f32_cos(a);
}

INLINE f32 f32_atan2(f32 _y, f32 _x) {
    static const f32 f32__aTtan2C0 = -0.013480470f;
    static const f32 f32__aTtan2C1 =  0.057477314f;
    static const f32 f32__aTtan2C2 = -0.121239071f;
    static const f32 f32__aTtan2C3 =  0.195635925f;
    static const f32 f32__aTtan2C4 = -0.332994597f;
    static const f32 f32__aTtan2C5 =  0.999995630f;

    const f32 ax     = absVal(_x);
    const f32 ay     = absVal(_y);
    const f32 maxaxy = maxVal(ax, ay);
    const f32 minaxy = minVal(ax, ay);

    if (maxaxy == 0.0f) {
        return 0.0f * f32_sign(_y);
    }

    const f32 mxy    = minaxy / maxaxy;
    const f32 mxysq  = f32_square(mxy);
    const f32 tmp0   = f32_mad(f32__aTtan2C0, mxysq, f32__aTtan2C1);
    const f32 tmp1   = f32_mad(tmp0,     mxysq, f32__aTtan2C2);
    const f32 tmp2   = f32_mad(tmp1,     mxysq, f32__aTtan2C3);
    const f32 tmp3   = f32_mad(tmp2,     mxysq, f32__aTtan2C4);
    const f32 tmp4   = f32_mad(tmp3,     mxysq, f32__aTtan2C5);
    const f32 tmp5   = tmp4 * mxy;
    const f32 tmp6   = ay > ax   ? f32_piHalf - tmp5 : tmp5;
    const f32 tmp7   = _x < 0.0f ? f32_pi     - tmp6 : tmp6;
    const f32 result = f32_sign(_y) * tmp7;

    return result;
}

INLINE f32 f32_toRadians(f32 degrees) {
    return degrees * (f32_pi / 180.0f);
}

#define f32_invSqrt(VAL) (1.0f / f32_sqrt(f32Val))

INLINE f32 f32_lerp(f32 a, f32 time, f32 b) {
    return (1.0f - time) * a + time * b;
}

INLINE f32 f32_clamp(f32 min, f32 value, f32 max) {
    f32 result = value;

    if (result < min) {
        result = min;
    }

    if (result > max) {
        result = max;
    }

    return result;
}


#pragma mark - Vec2

INLINE Vec2 v2_make(f32 x, f32 y) {
    Vec2 result;
    result.x = x;
    result.y = y;

    return result;
}

INLINE Vec2 v2_makeZero(void) {
    Vec2 result;
    result.x = 0;
    result.y = 0;

    return result;
}

INLINE Vec2 v2_makeF32(f32 val) {
    Vec2 result;
    result.x = val;
    result.y = val;

    return result;
}

INLINE Vec2 v2_max(Vec2 left, Vec2 right) {
    Vec2 result;
    result.x = maxVal(left.x, right.x);
    result.y = maxVal(left.y, right.y);

    return result;
}

INLINE Vec2 v2_min(Vec2 left, Vec2 right) {
    Vec2 result;
    result.x = minVal(left.x, right.x);
    result.y = minVal(left.y, right.y);

    return result;
}

INLINE Vec2 v2_zero(void) {
    Vec2 vec;
    vec.x = 0;
    vec.y = 0;

    return vec;
}

INLINE f32 v2_leng(Vec2 left) {
    return f32_sqrt(left.x * left.x + left.y * left.y);
}

INLINE Vec2 v2_add(Vec2 left, Vec2 right) {
    left.x += right.x;
    left.y += right.y;

    return left;
}

INLINE Vec2 v2_sub(Vec2 left, Vec2 right) {
    left.x -= right.x;
    left.y -= right.y;

    return left;

}

INLINE Vec2 v2_neg(Vec2 left) {
    left.x = -left.x;
    left.y = -left.y;

    return left;
}

INLINE Vec2 v2_mult(Vec2 left, Vec2 right) {
    left.x *= right.x;
    left.y *= right.y;

    return left;
}

INLINE Vec2 v2_multF32(Vec2 left, f32 scale) {
    left.x *= scale;
    left.y *= scale;

    return left;
}
INLINE Vec2 v2_div(Vec2 left, Vec2 right) {
    left.x /= right.x;
    left.y /= right.y;

    return left;
}


#pragma mark - Vec3

INLINE Vec3 vec3_make(f32 x, f32 y, f32 z) {
    Vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

INLINE Vec3 vec3_zero(void) {
    Vec3 result;
    result.x = 0;
    result.y = 0;
    result.z = 0;

    return result;
}

INLINE Vec3 vec3_splat(f32 val) {
    Vec3 result;
    result.x = val;
    result.y = val;
    result.z = val;

    return result;
}

INLINE f32 vec3_length(Vec3 left) {
    return f32_sqrt(left.x * left.x + left.y * left.y + left.z * left.z);
}

INLINE Vec3 vec3_add(Vec3  left, Vec3 right) {
    left.x += right.x;
    left.y += right.y;
    left.z += right.z;

    return left;
}

INLINE Vec3 vec3_subtract(Vec3 left, Vec3 right) {
    left.x -= right.x;
    left.y -= right.y;
    left.z -= right.z;

    return left;
}

INLINE Vec3 vec3_negative(Vec3 left) {
    left.x = -left.x;
    left.y = -left.y;
    left.z = -left.z;

    return left;
}

INLINE Vec3 vec3_multiply(Vec3 left, Vec3 right) {
    left.x *= right.x;
    left.y *= right.y;
    left.z *= right.z;

    return left;
}

INLINE Vec3 vec3_normalize(Vec3 A) {
    Vec3 result = vec3_zero();

    f32 vectorLength = vec3_length(A);

    if (vectorLength != 0.0f) {
        result.x = A.x * (1.0f / vectorLength);
        result.y = A.y * (1.0f / vectorLength);
        result.z = A.z * (1.0f / vectorLength);
    }
    
    return result;
}

INLINE Vec3 vec3_cross(Vec3 vecOne, Vec3 vecTwo) {
    Vec3 result = vec3_zero();

    result.x = (vecOne.y * vecTwo.z) - (vecOne.z * vecTwo.y);
    result.y = (vecOne.z * vecTwo.x) - (vecOne.x * vecTwo.z);
    result.z = (vecOne.x * vecTwo.y) - (vecOne.y * vecTwo.x);

    return result;
}

INLINE f32 vec3_dotVec3(Vec3 vecOne, Vec3 vecTwo) {
    f32 result = 0.0f;

    result = (vecOne.x * vecTwo.x) + (vecOne.y * vecTwo.y) + (vecOne.z * vecTwo.z);
    
    return result;
}


#pragma mark - Vec4

INLINE Vec4 v4_make(f32 x, f32 y, f32 z, f32 w) {
    Vec4 result;
    result.simd = f32x4_make(x, y, z, w);

    return result;
}

INLINE Vec4 v4_makeF32(f32 x) {
    Vec4 result;
    result.simd = f32x4_make(x, x, x, x);

    return result;
}

INLINE Vec4 v4_zero(void) {
    Vec4 result;
    result.simd = f32x4_make(0, 0, 0, 0);

    return result;
}

INLINE Vec4 v4_add(Vec4 left, Vec4 right) {
    Vec4 result;
    result.simd = f32x4_add(left.simd, right.simd);

    return result;
}

INLINE Vec4 v4_sub(Vec4 left, Vec4 right) {
    Vec4 result;
    result.simd = f32x4_sub(left.simd, right.simd);

    return result;
}


#pragma mark - Mat4

INLINE Mat4 mat4_zero(void) {
    Mat4 result = {{0}};

    return result;
}

INLINE Mat4 mat4_diag(f32 diagonal) {
    Mat4 result = mat4_zero();

    result.store4x4[0][0] = diagonal;
    result.store4x4[1][1] = diagonal;
    result.store4x4[2][2] = diagonal;
    result.store4x4[3][3] = diagonal;

    return result;
}

INLINE Mat4 mat4_multiply(Mat4 left, Mat4 right) {
    Mat4 result = mat4_zero();

#ifdef SIMD_USE_SSE
#error "Not implemented"
    for (int i = 0; i < 4; i++) {
        // Load a row of matrix A into a SIMD register
        __m128 rowA = _mm_loadu_ps(&A[i][0]);

        for (int j = 0; j < 4; j++) {
            // Load a column of matrix B into a SIMD register
            __m128 colB = _mm_set_ps(B[0][j], B[1][j], B[2][j], B[3][j]);

            // Multiply corresponding elements and accumulate
            __m128 prod = _mm_mul_ps(rowA, colB);

            // Horizontal addition of the four values in the SIMD register
            // This results in the dot product of the row and column
            __m128 sum = _mm_hadd_ps(prod, prod);
            sum = _mm_hadd_ps(sum, sum);

            // Store the result in the corresponding position in the result matrix
            result.store4x4[i][j] = _mm_cvtss_f32(sum);
        }
    }
#elif defined(SIMD_USE_NEON)
#error "Not implemented"
    for (int i = 0; i < 4; i++) {
        float4 rowA = A->row[i];

        for (int j = 0; j < 4; j++) {
            float4 colB = {B->row[0][j], B->row[1][j], B->row[2][j], B->row[3][j]};

            // Perform element-wise multiplication
            float4 prod = vmulq_f32(rowA, colB);

            // Perform horizontal addition
            float32x2_t sum2 = vadd_f32(vget_low_f32(prod), vget_high_f32(prod));
            float32x2_t sum = vpadd_f32(sum2, sum2);

            // Store the result in the corresponding position in the result matrix
            result->row[i][j] = vget_lane_f32(sum, 0);
        }
    }
#else
    int columns;
    for (columns = 0; columns < 4; ++columns) {
        int rows;
        for (rows = 0; rows < 4; ++rows) {
            f32 sum = 0;
            int currentMatrice;
            for (currentMatrice = 0; currentMatrice < 4; ++currentMatrice) {
                sum += left.store4x4[currentMatrice][rows] * right.store4x4[columns][currentMatrice];
            }
            result.store4x4[columns][rows] = sum;
        }
    }
#endif 
    return result;
}
INLINE Mat4 mat4_perspective(f32 fov, f32 aspectRatio, float near, float far) {
    Mat4 result = mat4_diag(1.0f);

    f32 tanThetaOver2 = f32_tan(fov * (f32_pi / 360.0f));
    
    result.store4x4[0][0] = 1.0f / tanThetaOver2;
    result.store4x4[1][1] = aspectRatio / tanThetaOver2;
    result.store4x4[2][3] = -1.0f;
    result.store4x4[2][2] = (near + far) / (near - far);
    result.store4x4[3][2] = (2.0f * near * far) / (near - far);
    result.store4x4[3][3] = 0.0f;

    return result;
}

INLINE Mat4 mat4_lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Mat4 result = mat4_zero();

    Vec3 f = vec3_normalize(vec3_subtract(center, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);

    result.store4x4[0][0] =  s.x;
    result.store4x4[0][1] =  u.x;
    result.store4x4[0][2] = -f.x;

    result.store4x4[1][0] =  s.y;
    result.store4x4[1][1] =  u.y;
    result.store4x4[1][2] = -f.y;

    result.store4x4[2][0] =  s.z;
    result.store4x4[2][1] =  u.z;
    result.store4x4[2][2] = -f.z;

    result.store4x4[3][0] = -vec3_dotVec3(s, eye);
    result.store4x4[3][1] = -vec3_dotVec3(u, eye);
    result.store4x4[3][2] =  vec3_dotVec3(f, eye);
    result.store4x4[3][3] = 1.0f;

    return result;
}

INLINE Mat4 mat4_rotate(f32 Angle, Vec3 axis) {
    Mat4 result = mat4_diag(1.0f);
    
    axis = vec3_normalize(axis);
    
    f32 sinTheta = f32_sin(f32_toRadians(Angle));
    f32 cosTheta = f32_cos(f32_toRadians(Angle));
    f32 cosValue = 1.0f - cosTheta;
    
    result.store4x4[0][0] = (axis.x * axis.x * cosValue) + cosTheta;
    result.store4x4[0][1] = (axis.x * axis.y * cosValue) + (axis.z * sinTheta);
    result.store4x4[0][2] = (axis.x * axis.z * cosValue) - (axis.y * sinTheta);
    
    result.store4x4[1][0] = (axis.y * axis.x * cosValue) - (axis.z * sinTheta);
    result.store4x4[1][1] = (axis.y * axis.y * cosValue) + cosTheta;
    result.store4x4[1][2] = (axis.y * axis.z * cosValue) + (axis.x * sinTheta);
    
    result.store4x4[2][0] = (axis.z * axis.x * cosValue) + (axis.y * sinTheta);
    result.store4x4[2][1] = (axis.z * axis.y * cosValue) - (axis.x * sinTheta);
    result.store4x4[2][2] = (axis.z * axis.z * cosValue) + cosTheta;
    
    return result;
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // _BASE_MATH_