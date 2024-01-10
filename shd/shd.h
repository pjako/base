#ifndef SHD_H
#define SHD_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum shd_backend {
    shd_backend_rx = 0, // default
//    shd_backend_sokol,
} shd_backend;

typedef enum shd_variations {
    shd_backend_legacy = 0, // default
    shd_backend_bindless = 0,
//    shd_backend_sokol,
} shd_variations;


API RESULT shd_generateShader(Arena* storeMem, Str8 shdFileName, Str8 shdFileContent, shd_backend backend, shd_variations variation);


#ifdef __cplusplus
extern "C" {
#endif
#endif // SHD_H