#include "base/base.h"
#include "base/base_types.h"
#include "base/base_math.h"
#include "base/base_time.h"

#if OS_WIN

#else
#include "time.h"

DateTime tm_nowGm(void) {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    struct tm tt;
    struct tm* res = gmtime_r(&spec.tv_sec, &tt);
    ASSERT(res != NULL);
    DateTime dateTime = {0};
    dateTime.year  = res->tm_year;
    dateTime.mon   = res->tm_mon;
    dateTime.day   = res->tm_mday;
    dateTime.hour  = res->tm_hour;
    dateTime.sec   = res->tm_sec;
    dateTime.msec  = roundVal(spec.tv_nsec, 1000000);

    return dateTime;
}

DateTime tm_nowLocal(void) {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    struct tm tt;
    struct tm* res = localtime_r(&spec.tv_sec, &tt);
    ASSERT(res != NULL);
    DateTime dateTime = {0};
    dateTime.year  = res->tm_year;
    dateTime.mon   = res->tm_mon;
    dateTime.day   = res->tm_mday;
    dateTime.hour  = res->tm_hour;
    dateTime.sec   = res->tm_sec;
    dateTime.msec  = roundVal(spec.tv_nsec, 1000000);

    return dateTime;
}

#endif // OS_WIN


DenseTime tm_toDenseTime(DateTime in) {
    u32 yearEncode = u32_cast(i32_cast(in.year+ 0x8000));
    u64 result = 0;
    result += yearEncode;
    result *= 12;
    result += in.month;
    result *= 31;
    result += in.day;
    result *= 24;
    result += in.hour;
    result *= 60;
    result += in.minute;
    result *= 61;
    result += in.second;
    result *= 1000;
    result += in.milliSecond;
    DenseTime denseTime = {result};
    return denseTime;
}

DateTime tm_fromDenseTime(DenseTime denseTime) {
    u64 in = denseTime.value;
    DateTime result;
    result.milliSecond = in % 1000;
    in /= 1000;
    result.second = in % 61;
    in /= 61;
    result.minute = in % 60;
    in /= 60;
    result.hour = in % 24;
    in /= 24;
    result.day = in % 31;
    in /= 31;
    result.month = in % 12;
    in /= 12;
    i32 yearEncoded = i32_cast(in);
    result.year = (yearEncoded - 0x8000);
    return result;
}

#if OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // WIN32_LEAN_AND_MEAN
#elif OS_ANDROID
#include <time.h>
#elif OS_EMSCRIPTEN
#include <emscripten.h>
#elif OS_APPLE
#include <mach/mach_time.h>
#else
#include <sys/time.h>
#endif 

u64 tm_currentCount(void) {
    u64 counter = u64_max;
#if OS_WIN
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    counter = li.QuadPart;
#elif OS_ANDROID
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    counter = now.tv_sec*INT64_C(1000000000) + now.tv_nsec;
#elif OS_EMSCRIPTEN
    counter = i64_cast(1000.0f * emscripten_get_now());
#elif OS_APPLE
    counter = mach_absolute_time();
#else
    struct timeval now;
    gettimeofday(&now, 0);
    counter = now.tv_sec * i64_cast(1000000) + now.tv_usec;
#endif
    return counter;
}

 tm_FrequencyInfo tm_getPerformanceFrequency(void) {
    tm_FrequencyInfo freq;
    freq.frequency = i64_val(1000000000);
#if  OS_WIN
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
	freq.frequency = li.QuadPart;
#elif OS_ANDROID
    freq.frequency = i64_val(1000000000);
#elif OS_EMSCRIPTEN
	freq.frequency = i64_val(1000000);
#elif OS_APPLE
    mach_timebase_info_data_t info = {0};
    kern_return_t result = mach_timebase_info(&info);
    if (result == KERN_SUCCESS) {
        freq.numer = info.numer;
        freq.denom = info.denom;
    }
#endif
    return freq;
}

INLINE i32 tm__i64Muldiv(i64 value, i64 numer, i64 denom) {
    i64 q = value / denom;
    i64 r = value % denom;
    return q * numer + r * numer / denom;
}

u64 tm_countToNanoseconds( tm_FrequencyInfo info, i64 count) {
    u64 now = 0;
#if OS_WIN
    now = (u64) tm__i64Muldiv(count, 1000000000, info.frequency);
#elif OS_APPLE
    now = u64_cast(((count * i64_cast(info.numer))) / i64_cast(info.denom));
#elif OS_EMSCRIPTEN
    f64 js_now = count;
    now = u64_cast(count * info.frequency) / 1000;
#else
    now = count * info.frequency;
#endif
    return now;
}


static const u64 tm__refreshRates[][2] = {
    { 16666667, 1000000 },  //  60 Hz: 16.6667 +- 1ms
    { 13888889,  250000 },  //  72 Hz: 13.8889 +- 0.25ms
    { 13333333,  250000 },  //  75 Hz: 13.3333 +- 0.25ms
    { 11764706,  250000 },  //  85 Hz: 11.7647 +- 0.25
    { 11111111,  250000 },  //  90 Hz: 11.1111 +- 0.25ms
    { 10000000,  500000 },  // 100 Hz: 10.0000 +- 0.5ms
    {  8333333,  500000 },  // 120 Hz:  8.3333 +- 0.5ms
    {  6944445,  500000 },  // 144 Hz:  6.9445 +- 0.5ms
    {  4166667, 1000000 },  // 240 Hz:  4.1666 +- 1ms
};

u64 tm_roundToCommonRefreshRate(u64 count) {
    i32 i = 0;
    for (i32 i = 0; countOf(tm__refreshRates) > i; i++) {
        u64 ns = tm__refreshRates[i][1];
        uint64_t tol = tm__refreshRates[i][1];
        if ((count > (ns - tol)) && (count < (ns + tol))) {
            return ns;
        }
    }
    return count;
}

