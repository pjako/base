#ifndef _BASE_TIME_
#define _BASE_TIME_
#ifdef __cplusplus
extern "C" {
#endif

#pragma mark - Date time

API DenseTime tm_toDenseTime(DateTime in);
API DateTime tm_fromDenseTime(DenseTime in);
API DateTime tm_nowGm(void);
API DateTime tm_nowLocal(void);

#pragma mark - High resolution time measurement

API tm_FrequencyInfo tm_getPerformanceFrequency(void);
API u64 tm_currentCount(void);
API u64 tm_countToNanoseconds(tm_FrequencyInfo info, i64 count);
API u64 tm_roundToCommonRefreshRate(u64 nanoSeconds);

#define tm_countToSeconds(COUNT) (f64_cast(COUNT) / 1000000000.0)
#define tm_countToMiliseconds(COUNT) (f64_cast(COUNT) / 1000000.0)
#define tm_countToMicroSeconds(COUNT) (f64_cast(COUNT) / 1000.0)
#define tm_countToNanoSeconds(COUNT) (f64_cast(COUNT))

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // _BASE_TIME_