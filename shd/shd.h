#ifndef SHD_H
#define SHD_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum shd_variation {
    shd_variation_ogl40 = 1,
    shd_variation_oglES3 = 2,
    shd_variation_mtl = 4,
    shd_variation_vk = 8,
    shd_variation_vkBindless = 16,
    shd_variation_dx12 = 32,
} shd_variation;
typedef flags32 shd_variationFlags;

API void shd_generateShader(Arena* storeMem, S8 shdFilePath, S8 shdFileContent, shd_variationFlags variationFlags);


#ifdef __cplusplus
extern "C" {
#endif
#endif // SHD_H