
#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"

#include "os/os.h"

#define RX_INTERNAL
#include "rx/rx.h"

#define RX_OGL


#ifdef RX_OGL
#undef RX_OGL
#define RX_OGL 1
#else
#define RX_OGL 0
#endif


//#if RX_METAL
//#import <QuartzCore/CAMetalLayer.h>
//#endif
#if RX_OGL
#if OS_APPLE
#include <TargetConditionals.h>
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif

#if OS_IOS
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#import <UIKit/UIKit.h>
#else
#include <OpenGL/gl3.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#define RX_GLCORE33
#endif
#elif OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment (lib, "kernel32")   // GetProcAddress()
#define RX_USE_WIN32_GL_LOADER

#if defined(RX_USE_WIN32_GL_LOADER)
typedef unsigned int  GLuint;
#endif

#else
#error "Undefined platform"
#endif
#endif



#if OS_APPLE
#if __has_feature(objc_arc)
#define RX_OBJC_RELESE(obj) { obj = nil; }
#else
#define RX_OBJC_RELESE(obj) { [obj release]; obj = nil; }
#endif
#endif

typedef struct rx_Shader {
    union {
#if RX_OGL
        GLuint handle;
#endif
    };
    u32 gen : 16;
} rx_Shader;

typedef struct rx_Buffer {
    union {
#if RX_OGL
        struct {
            GLuint handle;
        } gl;
#endif
    };
    u32                 gen : 16;
    u64                 alignment;
    u64                 allocatedSize;
    u64                 size;
    rx_bufferUsageFlags usage;
} rx_Buffer;

typedef struct rx_Texture {
    union {
#if RX_OGL
        struct {
            GLuint handle;
        } gl;
#endif
    };
    u32 gen : 16;
} rx_Texture;

typedef struct rx_TextureView {
    union {
#if RX_OGL
        struct {
            GLuint handle;
        } gl;
#endif
    };
    u32 gen : 7;
    rx_texture textureRef;
} rx_TextureView;

typedef struct rx_Sampler {
    union {
#if RX_OGL
        GLuint handle;
#endif
    };
    u32 gen : 16;
} rx_Sampler;

typedef enum rx_api {
    rx_api_default = 0,
    rx_api_ogl,
    rx_api_vk,
    rx_api_mtl,
    rx_api_dx12,
} rx_api;

typedef struct rx_IndexQueue {
    u64 out;
    u64 in;
    u32* indicies;
    u32 size;
} rx_IndexQueue;

#define rx__poolDef(TYPE) struct {TYPE* elements; u32 capacity; rx_IndexQueue freeIndicies;}
#define rx__poolInit(ARENA, POOL, COUNT) (POOL)->elements = mem_arenaPush((ARENA), sizeOf((POOL)->elements[0]) * (COUNT) ); (POOL)->capacity = (COUNT); rx__queueInit((ARENA), &(POOL)->freeIndicies, (COUNT), (COUNT))
// rx_poolInit(ctx->.arena, &ctx->buffers, descWithDefaults.maxBuffers);

typedef struct rx_Ctx {
    rx_api api;
    Arena* arena;
    rx__poolDef(rx_Buffer) buffers;
    //rx__poolDef(rx_Texture) textures;
} rx_Ctx;


#define rx_valueOrDefault(VAR, DEFAULT) (VAR) != 0 ? (VAR) : (DEFAULT)

// Shared code

enum {
    RX__INVALID_INDEX = RX_U32_MAX
};

#define rx__queueRingEntry(queue, index) (queue->indicies + (index & (queue->size - 1)))

void rx__queueInit(Arena* arena, rx_IndexQueue* queue, u32 size, u32 endIndex) {
    u32_nextPowerOfTwo(size);
    queue->in = 0;
    queue->out = 0;
    queue->size = size;
    queue->indicies = mem_arenaPushArray(arena, u32, size);
    u32 idx = 0;
    for (; idx < endIndex; idx++) {
        queue->indicies[idx] = idx + 1;
    }
    queue->in = idx;
    u32 zeroSize = (queue->size - idx);
    if (zeroSize > 0) {
        mem_setZero(&queue->indicies[idx], zeroSize * sizeOf(u32));
    }
}


bool rx__queuePush(rx_IndexQueue* queue, u32 index) {
    if ((queue->in + 1) == queue->out) {
        // queue full
        return false;
    }

    u32* idxPtr = rx__queueRingEntry(queue, queue->in);
    *idxPtr = (index + 1);
    queue->in += 1;

    return false;
}

u32 rx__queuePull(rx_IndexQueue* queue) {
    if (queue->in <= (queue->out + 1)) {
        // queue empty
        return RX__INVALID_INDEX;
    }
    u32 idx = queue->out;
    queue->out += 1;

    idx = (idx & (queue->size - 1));
    u32 out = queue->indicies[idx] - 1;
    ASSERT(out != RX__INVALID_INDEX);
    queue->indicies[idx] = 0;
    return out;
}



#if RX_VULKAN

typedef struct rx_VulkanCtx {
    rx_Ctx base;
    VkInstance instance;
    struct {
        VkResult (VKAPI_PTR* vkCreateInstance)(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
        void     (VKAPI_PTR* vkDestroyInstance)(VkInstance instance, const VkAllocationCallbacks* pAllocator);
        VkResult (VKAPI_PTR* vkEnumerateInstanceLayerProperties)(u32* pPropertyCount, VkLayerProperties* pProperties);
        VkResult (VKAPI_PTR* vkEnumerateInstanceExtensionProperties)(const char* pLayerName, u32* pPropertyCount, VkExtensionProperties* pProperties);

        VkResult (VKAPI_PTR* vkCreateDevice)(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
        void     (VKAPI_PTR* vkDestroyDevice)(VkDevice device, const VkAllocationCallbacks* pAllocator);
    } instanceApi;
} rx_VulkanCtx;

#endif

#if RX_OGL
#define rx__oglCheckErrors() { ASSERT(glGetError() == GL_NO_ERROR); }

// optional GL loader for win32
#if defined(RX_USE_WIN32_GL_LOADER)

#define __gl_h_ 1
#define __gl32_h_ 1
#define __gl31_h_ 1
#define __GL_H__ 1
#define __glext_h_ 1
#define __GLEXT_H_ 1
#define __gltypes_h_ 1
#define __glcorearb_h_ 1
#define __gl_glcorearb_h_ 1
#define GL_APIENTRY APIENTRY

typedef unsigned int  GLenum;
typedef unsigned int  GLuint;
typedef int  GLsizei;
typedef char  GLchar;
typedef ptrdiff_t  GLintptr;
typedef ptrdiff_t  GLsizeiptr;
typedef double  GLclampd;
typedef unsigned short  GLushort;
typedef unsigned char  GLubyte;
typedef unsigned char  GLboolean;
typedef uint64_t  GLuint64;
typedef double  GLdouble;
typedef unsigned short  GLhalf;
typedef float  GLclampf;
typedef unsigned int  GLbitfield;
typedef signed char  GLbyte;
typedef short  GLshort;
typedef void  GLvoid;
typedef int64_t  GLint64;
typedef float  GLfloat;
typedef int  GLint;
#define GL_INT_2_10_10_10_REV 0x8D9F
#define GL_R32F 0x822E
#define GL_PROGRAM_POINT_SIZE 0x8642
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_R16F 0x822D
#define GL_COLOR_ATTACHMENT22 0x8CF6
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_NUM_EXTENSIONS 0x821D
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_VERTEX_SHADER 0x8B31
#define GL_INCR 0x1E02
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_STATIC_DRAW 0x88E4
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_FUNC_SUBTRACT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_CONSTANT_COLOR 0x8001
#define GL_DECR_WRAP 0x8508
#define GL_R8 0x8229
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_SHORT 0x1402
#define GL_DEPTH_TEST 0x0B71
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_LINK_STATUS 0x8B82
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_RGBA16F 0x881A
#define GL_CONSTANT_ALPHA 0x8003
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_MIN_LOD 0x813A
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_STREAM_DRAW 0x88E0
#define GL_ONE 1
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_RGB10_A2 0x8059
#define GL_RGBA8 0x8058
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_RGBA4 0x8056
#define GL_RGB8 0x8051
#define GL_ARRAY_BUFFER 0x8892
#define GL_STENCIL 0x1802
#define GL_TEXTURE_2D 0x0DE1
#define GL_DEPTH 0x1801
#define GL_FRONT 0x0404
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_REPEAT 0x2901
#define GL_RGBA 0x1908
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_DECR 0x1E03
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_FLOAT 0x1406
#define GL_TEXTURE_MAX_LOD 0x813B
#define GL_DEPTH_COMPONENT 0x1902
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_COLOR 0x1800
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_TRIANGLES 0x0004
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_NONE 0
#define GL_SRC_COLOR 0x0300
#define GL_BYTE 0x1400
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_LINE_STRIP 0x0003
#define GL_TEXTURE_3D 0x806F
#define GL_CW 0x0900
#define GL_LINEAR 0x2601
#define GL_RENDERBUFFER 0x8D41
#define GL_GEQUAL 0x0206
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_RGBA32F 0x8814
#define GL_BLEND 0x0BE2
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_EXTENSIONS 0x1F03
#define GL_NO_ERROR 0
#define GL_REPLACE 0x1E01
#define GL_KEEP 0x1E00
#define GL_CCW 0x0901
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_RGB 0x1907
#define GL_TRIANGLE_STRIP 0x0005
#define GL_FALSE 0
#define GL_ZERO 0
#define GL_CULL_FACE 0x0B44
#define GL_INVERT 0x150A
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_UNSIGNED_SHORT 0x1403
#define GL_NEAREST 0x2600
#define GL_SCISSOR_TEST 0x0C11
#define GL_LEQUAL 0x0203
#define GL_STENCIL_TEST 0x0B90
#define GL_DITHER 0x0BD0
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_EQUAL 0x0202
#define GL_FRAMEBUFFER 0x8D40
#define GL_RGB5 0x8050
#define GL_LINES 0x0001
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_SRC_ALPHA 0x0302
#define GL_INCR_WRAP 0x8507
#define GL_LESS 0x0201
#define GL_MULTISAMPLE 0x809D
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_BACK 0x0405
#define GL_ALWAYS 0x0207
#define GL_FUNC_ADD 0x8006
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_NOTEQUAL 0x0205
#define GL_DST_COLOR 0x0306
#define GL_COMPILE_STATUS 0x8B81
#define GL_RED 0x1903
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_DST_ALPHA 0x0304
#define GL_RGB5_A1 0x8057
#define GL_GREATER 0x0204
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_TRUE 1
#define GL_NEVER 0x0200
#define GL_POINTS 0x0000
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_MIRRORED_REPEAT 0x8370
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_R11F_G11F_B10F 0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define GL_RGB9_E5 0x8C3D
#define GL_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#define GL_RGBA32UI 0x8D70
#define GL_RGB32UI 0x8D71
#define GL_RGBA16UI 0x8D76
#define GL_RGB16UI 0x8D77
#define GL_RGBA8UI 0x8D7C
#define GL_RGB8UI 0x8D7D
#define GL_RGBA32I 0x8D82
#define GL_RGB32I 0x8D83
#define GL_RGBA16I 0x8D88
#define GL_RGB16I 0x8D89
#define GL_RGBA8I 0x8D8E
#define GL_RGB8I 0x8D8F
#define GL_RED_INTEGER 0x8D94
#define GL_RG 0x8227
#define GL_RG_INTEGER 0x8228
#define GL_R8 0x8229
#define GL_R16 0x822A
#define GL_RG8 0x822B
#define GL_RG16 0x822C
#define GL_R16F 0x822D
#define GL_R32F 0x822E
#define GL_RG16F 0x822F
#define GL_RG32F 0x8230
#define GL_R8I 0x8231
#define GL_R8UI 0x8232
#define GL_R16I 0x8233
#define GL_R16UI 0x8234
#define GL_R32I 0x8235
#define GL_R32UI 0x8236
#define GL_RG8I 0x8237
#define GL_RG8UI 0x8238
#define GL_RG16I 0x8239
#define GL_RG16UI 0x823A
#define GL_RG32I 0x823B
#define GL_RG32UI 0x823C
#define GL_RGBA_INTEGER 0x8D99
#define GL_R8_SNORM 0x8F94
#define GL_RG8_SNORM 0x8F95
#define GL_RGB8_SNORM 0x8F96
#define GL_RGBA8_SNORM 0x8F97
#define GL_R16_SNORM 0x8F98
#define GL_RG16_SNORM 0x8F99
#define GL_RGB16_SNORM 0x8F9A
#define GL_RGBA16_SNORM 0x8F9B
#define GL_RGBA16 0x805B
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
#define GL_MAX_3D_TEXTURE_SIZE 0x8073
#define GL_MAX_ARRAY_TEXTURE_LAYERS 0x88FF
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_TEXTURE_BORDER_COLOR 0x1004
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_TEXTURE_COMPARE_FUNC 0x884D
#define GL_COMPARE_REF_TO_TEXTURE 0x884E
#define GL_TEXTURE_CUBE_MAP_SEAMLESS 0x884F
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_FRAMEBUFFER_UNDEFINED 0x8219
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56


// X Macro list of GL function names and signatures
#define RX__GL_FUNCS \
    RX__XMACRO(glBindVertexArray,                 void, (GLuint array)) \
    RX__XMACRO(glFramebufferTextureLayer,         void, (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)) \
    RX__XMACRO(glGenFramebuffers,                 void, (GLsizei n, GLuint * framebuffers)) \
    RX__XMACRO(glBindFramebuffer,                 void, (GLenum target, GLuint framebuffer)) \
    RX__XMACRO(glBindRenderbuffer,                void, (GLenum target, GLuint renderbuffer)) \
    RX__XMACRO(glGetStringi,                      const GLubyte *, (GLenum name, GLuint index)) \
    RX__XMACRO(glClearBufferfi,                   void, (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)) \
    RX__XMACRO(glClearBufferfv,                   void, (GLenum buffer, GLint drawbuffer, const GLfloat * value)) \
    RX__XMACRO(glClearBufferuiv,                  void, (GLenum buffer, GLint drawbuffer, const GLuint * value)) \
    RX__XMACRO(glClearBufferiv,                   void, (GLenum buffer, GLint drawbuffer, const GLint * value)) \
    RX__XMACRO(glDeleteRenderbuffers,             void, (GLsizei n, const GLuint * renderbuffers)) \
    RX__XMACRO(glUniform1fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    RX__XMACRO(glUniform2fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    RX__XMACRO(glUniform3fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    RX__XMACRO(glUniform4fv,                      void, (GLint location, GLsizei count, const GLfloat * value)) \
    RX__XMACRO(glUniform1iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    RX__XMACRO(glUniform2iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    RX__XMACRO(glUniform3iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    RX__XMACRO(glUniform4iv,                      void, (GLint location, GLsizei count, const GLint * value)) \
    RX__XMACRO(glUniformMatrix4fv,                void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)) \
    RX__XMACRO(glUseProgram,                      void, (GLuint program)) \
    RX__XMACRO(glShaderSource,                    void, (GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length)) \
    RX__XMACRO(glLinkProgram,                     void, (GLuint program)) \
    RX__XMACRO(glGetUniformLocation,              GLint, (GLuint program, const GLchar * name)) \
    RX__XMACRO(glGetShaderiv,                     void, (GLuint shader, GLenum pname, GLint * params)) \
    RX__XMACRO(glGetProgramInfoLog,               void, (GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog)) \
    RX__XMACRO(glGetAttribLocation,               GLint, (GLuint program, const GLchar * name)) \
    RX__XMACRO(glDisableVertexAttribArray,        void, (GLuint index)) \
    RX__XMACRO(glDeleteShader,                    void, (GLuint shader)) \
    RX__XMACRO(glDeleteProgram,                   void, (GLuint program)) \
    RX__XMACRO(glCompileShader,                   void, (GLuint shader)) \
    RX__XMACRO(glStencilFuncSeparate,             void, (GLenum face, GLenum func, GLint ref, GLuint mask)) \
    RX__XMACRO(glStencilOpSeparate,               void, (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)) \
    RX__XMACRO(glRenderbufferStorageMultisample,  void, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)) \
    RX__XMACRO(glDrawBuffers,                     void, (GLsizei n, const GLenum * bufs)) \
    RX__XMACRO(glVertexAttribDivisor,             void, (GLuint index, GLuint divisor)) \
    RX__XMACRO(glBufferSubData,                   void, (GLenum target, GLintptr offset, GLsizeiptr size, const void * data)) \
    RX__XMACRO(glGenBuffers,                      void, (GLsizei n, GLuint * buffers)) \
    RX__XMACRO(glCheckFramebufferStatus,          GLenum, (GLenum target)) \
    RX__XMACRO(glFramebufferRenderbuffer,         void, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)) \
    RX__XMACRO(glCompressedTexImage2D,            void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data)) \
    RX__XMACRO(glCompressedTexImage3D,            void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data)) \
    RX__XMACRO(glActiveTexture,                   void, (GLenum texture)) \
    RX__XMACRO(glTexSubImage3D,                   void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels)) \
    RX__XMACRO(glRenderbufferStorage,             void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)) \
    RX__XMACRO(glGenTextures,                     void, (GLsizei n, GLuint * textures)) \
    RX__XMACRO(glPolygonOffset,                   void, (GLfloat factor, GLfloat units)) \
    RX__XMACRO(glDrawElements,                    void, (GLenum mode, GLsizei count, GLenum type, const void * indices)) \
    RX__XMACRO(glDeleteFramebuffers,              void, (GLsizei n, const GLuint * framebuffers)) \
    RX__XMACRO(glBlendEquationSeparate,           void, (GLenum modeRGB, GLenum modeAlpha)) \
    RX__XMACRO(glDeleteTextures,                  void, (GLsizei n, const GLuint * textures)) \
    RX__XMACRO(glGetProgramiv,                    void, (GLuint program, GLenum pname, GLint * params)) \
    RX__XMACRO(glBindTexture,                     void, (GLenum target, GLuint texture)) \
    RX__XMACRO(glTexImage3D,                      void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels)) \
    RX__XMACRO(glCreateShader,                    GLuint, (GLenum type)) \
    RX__XMACRO(glTexSubImage2D,                   void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels)) \
    RX__XMACRO(glFramebufferTexture2D,            void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
    RX__XMACRO(glCreateProgram,                   GLuint, (void)) \
    RX__XMACRO(glViewport,                        void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    RX__XMACRO(glDeleteBuffers,                   void, (GLsizei n, const GLuint * buffers)) \
    RX__XMACRO(glDrawArrays,                      void, (GLenum mode, GLint first, GLsizei count)) \
    RX__XMACRO(glDrawElementsInstanced,           void, (GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount)) \
    RX__XMACRO(glVertexAttribPointer,             void, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer)) \
    RX__XMACRO(glUniform1i,                       void, (GLint location, GLint v0)) \
    RX__XMACRO(glDisable,                         void, (GLenum cap)) \
    RX__XMACRO(glColorMask,                       void, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    RX__XMACRO(glColorMaski,                      void, (GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    RX__XMACRO(glBindBuffer,                      void, (GLenum target, GLuint buffer)) \
    RX__XMACRO(glDeleteVertexArrays,              void, (GLsizei n, const GLuint * arrays)) \
    RX__XMACRO(glDepthMask,                       void, (GLboolean flag)) \
    RX__XMACRO(glDrawArraysInstanced,             void, (GLenum mode, GLint first, GLsizei count, GLsizei instancecount)) \
    RX__XMACRO(glScissor,                         void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
    RX__XMACRO(glGenRenderbuffers,                void, (GLsizei n, GLuint * renderbuffers)) \
    RX__XMACRO(glBufferData,                      void, (GLenum target, GLsizeiptr size, const void * data, GLenum usage)) \
    RX__XMACRO(glBlendFuncSeparate,               void, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)) \
    RX__XMACRO(glTexParameteri,                   void, (GLenum target, GLenum pname, GLint param)) \
    RX__XMACRO(glGetIntegerv,                     void, (GLenum pname, GLint * data)) \
    RX__XMACRO(glEnable,                          void, (GLenum cap)) \
    RX__XMACRO(glBlitFramebuffer,                 void, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)) \
    RX__XMACRO(glStencilMask,                     void, (GLuint mask)) \
    RX__XMACRO(glAttachShader,                    void, (GLuint program, GLuint shader)) \
    RX__XMACRO(glGetError,                        GLenum, (void)) \
    RX__XMACRO(glBlendColor,                      void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
    RX__XMACRO(glTexParameterf,                   void, (GLenum target, GLenum pname, GLfloat param)) \
    RX__XMACRO(glTexParameterfv,                  void, (GLenum target, GLenum pname, const GLfloat* params)) \
    RX__XMACRO(glGetShaderInfoLog,                void, (GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog)) \
    RX__XMACRO(glDepthFunc,                       void, (GLenum func)) \
    RX__XMACRO(glStencilOp ,                      void, (GLenum fail, GLenum zfail, GLenum zpass)) \
    RX__XMACRO(glStencilFunc,                     void, (GLenum func, GLint ref, GLuint mask)) \
    RX__XMACRO(glEnableVertexAttribArray,         void, (GLuint index)) \
    RX__XMACRO(glBlendFunc,                       void, (GLenum sfactor, GLenum dfactor)) \
    RX__XMACRO(glReadBuffer,                      void, (GLenum src)) \
    RX__XMACRO(glTexImage2D,                      void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels)) \
    RX__XMACRO(glGenVertexArrays,                 void, (GLsizei n, GLuint * arrays)) \
    RX__XMACRO(glFrontFace,                       void, (GLenum mode)) \
    RX__XMACRO(glCullFace,                        void, (GLenum mode)) \
    RX__XMACRO(glPixelStorei,                     void, (GLenum pname, GLint param)) \
    RX__XMACRO(glBindSampler,                     void, (GLuint unit, GLuint sampler)) \
    RX__XMACRO(glGenSamplers,                     void, (GLsizei n, GLuint* samplers)) \
    RX__XMACRO(glSamplerParameteri,               void, (GLuint sampler, GLenum pname, GLint param)) \
    RX__XMACRO(glSamplerParameterf,               void, (GLuint sampler, GLenum pname, GLfloat param)) \
    RX__XMACRO(glSamplerParameterfv,              void, (GLuint sampler, GLenum pname, const GLfloat* params)) \
    RX__XMACRO(glDeleteSamplers,                  void, (GLsizei n, const GLuint* samplers))

// generate GL function pointer typedefs
#define RX__XMACRO(name, ret, args) typedef ret (GL_APIENTRY* PFN_ ## name) args;
RX__GL_FUNCS
#undef RX__XMACRO

// generate GL function pointers
#define RX__XMACRO(name, ret, args) static PFN_ ## name name;
RX__GL_FUNCS
#undef RX__XMACRO

#endif

#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8 0x84FA
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_COMPRESSED_RED_RGTC1
#define GL_COMPRESSED_RED_RGTC1 0x8DBB
#endif
#ifndef GL_COMPRESSED_SIGNED_RED_RGTC1
#define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#endif
#ifndef GL_COMPRESSED_RED_GREEN_RGTC2
#define GL_COMPRESSED_RED_GREEN_RGTC2 0x8DBD
#endif
#ifndef GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2
#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2 0x8DBE
#endif
#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM_ARB
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB 0x8E8C
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB 0x8E8D
#endif
#ifndef GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB 0x8E8E
#endif
#ifndef GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB 0x8E8F
#endif
#ifndef GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#endif
#ifndef GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif
#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif
#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif
#ifndef GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#endif
#ifndef GL_COMPRESSED_RG11_EAC
#define GL_COMPRESSED_RG11_EAC 0x9272
#endif
#ifndef GL_COMPRESSED_SIGNED_RG11_EAC
#define GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT 0x140B
#endif
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL 0x84F9
#endif
#ifndef GL_LUMINANCE
#define GL_LUMINANCE 0x1909
#endif

#if 0
_SOKOL_PRIVATE void _sg_gl_unload_opengl(void) {
    SOKOL_ASSERT(_sg.gl.opengl32_dll);
    FreeLibrary(_sg.gl.opengl32_dll);
    _sg.gl.opengl32_dll = 0;
}
#endif

#endif // _SOKOL_USE_WIN32_GL_LOADER


#if OS_OSX
#define RXGLLayer NSOpenGLLayer
#else
#define RXGLLayer CAEAGLLayer
#endif
typedef struct rx_OpenGlCtx {
    rx_Ctx base;
#if OS_WIN
    os_Dl* openGl32Dll;
#endif
#if OS_APPLE
    RXGLLayer* nsGlLayer;
#endif
#if OS_OSX
    NSOpenGLContext* nsOpenGlContext;
#endif
    GLuint vao;
    GLuint defaultFramebuffer;
    struct {
        GLuint vertexBuffer;
        GLuint indexBuffer;
        GLuint storedVertexBuffer;
        GLuint storedIndexBuffer;
    } cache;
} rx_OpenGlCtx;

#if defined(RX_USE_WIN32_GL_LOADER)
// helper function to lookup GL functions in GL DLL
typedef PROC (WINAPI * rx__wglGetProcAddress)(LPCSTR);
LOCAL void* rx__glGetProcAddr(os_Dl* openGl32Dll, const char* name, rx__wglGetProcAddress wgl_getprocaddress) {
    void* proc_addr = (void*) wgl_getprocaddress(name);
    if (0 == proc_addr) {
        proc_addr = (void*) GetProcAddress(openGl32Dll, name);
    }
    SOKOL_ASSERT(proc_addr);
    return proc_addr;
}

// populate GL function pointers
LOCAL void rx__loadOpenGl(rx_OpenGlCtx* ctx) {
    ASSERT(ctx->openGl32Dll == 0);
    ctx->openGl32Dll = LoadLibraryA("opengl32.dll");
    ASSERT(ctx->openGl32Dll != NULL);
    rx__wglGetProcAddress wgl_getprocaddress = (rx__wglGetProcAddress) GetProcAddress(ctx->openGl32Dll, "wglGetProcAddress");
    ASSERT(wgl_getprocaddress);
    #define RX__XMACRO(name, ret, args) name = (PFN_ ## name) rx__glGetProcAddr(ctx->openGl32Dll, #name, wgl_getprocaddress);
    RX__GL_FUNCS
    #undef RX__XMACRO
}
#endif // RX_USE_WIN32_GL_LOADER

#if 0
LOCAL rx_renderPipeline rx__makeRenderPipeline(rx_RenderPipelineDesc* desc) {

}
#endif


#if defined(RX_GLCORE33)
LOCAL void rx__initCapsGlCore33(rx_OpenGlCtx* ctx) {
    #if 0
    _sg.backend = SG_BACKEND_GLCORE33;

    _sg.features.origin_top_left = false;
    _sg.features.image_clamp_to_border = true;
    _sg.features.mrt_independent_blend_state = false;
    _sg.features.mrt_independent_write_mask = true;

    // scan extensions
    bool has_s3tc = false;  // BC1..BC3
    bool has_rgtc = false;  // BC4 and BC5
    bool has_bptc = false;  // BC6H and BC7
    bool has_pvrtc = false;
    bool has_etc2 = false;
    GLint num_ext = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
    for (int i = 0; i < num_ext; i++) {
        const char* ext = (const char*) glGetStringi(GL_EXTENSIONS, (GLuint)i);
        if (ext) {
            if (strstr(ext, "_texture_compression_s3tc")) {
                has_s3tc = true;
            } else if (strstr(ext, "_texture_compression_rgtc")) {
                has_rgtc = true;
            } else if (strstr(ext, "_texture_compression_bptc")) {
                has_bptc = true;
            } else if (strstr(ext, "_texture_compression_pvrtc")) {
                has_pvrtc = true;
            } else if (strstr(ext, "_ES3_compatibility")) {
                has_etc2 = true;
            } else if (strstr(ext, "_texture_filter_anisotropic")) {
                _sg.gl.ext_anisotropic = true;
            }
        }
    }

    // limits
    _sg_gl_init_limits();

    // pixel formats
    const bool has_bgra = false;    // not a bug
    const bool has_colorbuffer_float = true;
    const bool has_colorbuffer_half_float = true;
    const bool has_texture_float_linear = true; // FIXME???
    const bool has_float_blend = true;
    _sg_gl_init_pixelformats(has_bgra);
    _sg_gl_init_pixelformats_float(has_colorbuffer_float, has_texture_float_linear, has_float_blend);
    _sg_gl_init_pixelformats_half_float(has_colorbuffer_half_float);
    if (has_s3tc) {
        _sg_gl_init_pixelformats_s3tc();
    }
    if (has_rgtc) {
        _sg_gl_init_pixelformats_rgtc();
    }
    if (has_bptc) {
        _sg_gl_init_pixelformats_bptc();
    }
    if (has_pvrtc) {
        _sg_gl_init_pixelformats_pvrtc();
    }
    if (has_etc2) {
        _sg_gl_init_pixelformats_etc2();
    }
    #endif
}
#endif


LOCAL void rx__setupBackend(rx_Ctx* baseCtx, const rx_SetupDesc* desc) {
    ASSERT(baseCtx);
    ASSERT(desc);
    //ASSERT(desc->context.gl.default_framebuffer_cb == 0 || desc->context.gl.default_framebuffer_userdata_cb == 0);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;

    // assumes that _sg.gl is already zero-initialized
    //_sg.gl.valid = true;

    #if defined(_SOKOL_USE_WIN32_GL_LOADER)
    _sg_gl_load_opengl();
    #endif


    // clear initial GL error state
    #if defined(SOKOL_DEBUG)
        while (glGetError() != GL_NO_ERROR);
    #endif
    #if defined(RX_GLCORE33)
        rx__initCapsGlCore33(ctx);
    #elif defined(SOKOL_GLES3)
        _sg_gl_init_caps_gles3(ctx);
    #endif
}

LOCAL rx_Ctx* rx__create(Arena* arena, rx_SetupDesc* desc) {
    ASSERT(arena);
    ASSERT(desc);
    rx_OpenGlCtx* ctx = mem_arenaPushStructZero(arena, rx_OpenGlCtx);
    ctx->base.api = rx_api_ogl;
#if OS_WINDOWS
    os_Dl* openGlLib = os_dlOpen(s8(""))
#endif


#if OS_APPLE
    ctx->nsGlLayer = (RXGLLayer*) desc->context.gl.appleCaOpenGlLayer;
    ASSERT(ctx->nsGlLayer != 0);
#endif

#if OS_OSX
    {
        NSOpenGLPixelFormatAttribute attrs[32];
        int i = 0;
        attrs[i++] = NSOpenGLPFAAccelerated;
        attrs[i++] = NSOpenGLPFADoubleBuffer;
        attrs[i++] = NSOpenGLPFAOpenGLProfile;
        const int glVersion = 41; //_sapp.desc.gl_major_version * 10 + _sapp.desc.gl_minor_version;
        switch(glVersion) {
            case 10: attrs[i++] = NSOpenGLProfileVersionLegacy;  break;
            case 32: attrs[i++] = NSOpenGLProfileVersion3_2Core; break;
            case 41: attrs[i++] = NSOpenGLProfileVersion4_1Core; break;
            default: ASSERT(!"MACOS_INVALID_NSOPENGL_PROFILE");//_SAPP_PANIC(MACOS_INVALID_NSOPENGL_PROFILE);
        }
        attrs[i++] = NSOpenGLPFAColorSize; attrs[i++] = 24;
        attrs[i++] = NSOpenGLPFAAlphaSize; attrs[i++] = 8;
        attrs[i++] = NSOpenGLPFADepthSize; attrs[i++] = 24;
        attrs[i++] = NSOpenGLPFAStencilSize; attrs[i++] = 8;
        if (desc->sampleCount > 1) {
            attrs[i++] = NSOpenGLPFAMultisample;
            attrs[i++] = NSOpenGLPFASampleBuffers;
            attrs[i++] = 1;
            attrs[i++] = NSOpenGLPFASamples;
            attrs[i++] = (NSOpenGLPixelFormatAttribute) desc->sampleCount;
        }
        else {
            attrs[i++] = NSOpenGLPFASampleBuffers; attrs[i++] = 0;
        }
        attrs[i++] = 0;
        NSOpenGLPixelFormat* glpixelformatObj = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        
        ctx->nsOpenGlContext = [[NSOpenGLContext alloc] initWithFormat:glpixelformatObj shareContext: nil];
        ctx->nsGlLayer.openGLContext = ctx->nsOpenGlContext;
        [ctx->nsOpenGlContext makeCurrentContext];
        NSView* view = ctx->nsGlLayer.view;
        [ctx->nsOpenGlContext setView:view];
        [ctx->nsOpenGlContext flushBuffer];

        RX_OBJC_RELESE(glpixelformatObj);
    }
#endif

    // init context
#if 1
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&ctx->defaultFramebuffer);
    rx__oglCheckErrors()
    //ASSERT(ctx->defaultFramebuffer);
    glGenVertexArrays(1, &ctx->vao);
    rx__oglCheckErrors()
    ASSERT(ctx->vao);
    glBindVertexArray(ctx->vao);
#endif
    rx__oglCheckErrors()
    // incoming texture data is generally expected to be packed tightly
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    rx__oglCheckErrors()

    #if defined(RX_GLCORE33)
        // enable seamless cubemap sampling (only desktop GL)
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    #endif
    return (rx_Ctx*) ctx;
}

LOCAL void rx__glDiscardContext(rx_Ctx* baseCtx) {
    ASSERT(baseCtx);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    if (ctx->vao) {
        glDeleteVertexArrays(1, &ctx->vao);
    }
    rx__oglCheckErrors();
}

LOCAL void rx__activateContext(rx_Ctx* baseCtx) {
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;

    rx__oglCheckErrors();
    // NOTE: ctx can be 0 to unset the current context
    //_sg.gl.cur_context = ctx;
    //_sg_gl_reset_state_cache();
}

LOCAL void rx__glCacheBindBuffer(rx_OpenGlCtx* ctx, GLenum target, GLuint buffer) {
    ASSERT((target == GL_ARRAY_BUFFER) || (target == GL_ELEMENT_ARRAY_BUFFER));
    if (target == GL_ARRAY_BUFFER) {
        if (ctx->cache.vertexBuffer != buffer) {
            ctx->cache.vertexBuffer = buffer;
            glBindBuffer(target, buffer);
            //_sg_stats_add(gl.num_bind_buffer, 1);
        }
    } else {
        if (ctx->cache.indexBuffer != buffer) {
            ctx->cache.indexBuffer = buffer;
            glBindBuffer(target, buffer);
            //_sg_stats_add(gl.num_bind_buffer, 1);
        }
    }
}

LOCAL void rx__glCacheStoreBufferBinding(rx_OpenGlCtx* ctx, GLenum target) {
    if (target == GL_ARRAY_BUFFER) {
        ctx->cache.storedVertexBuffer = ctx->cache.vertexBuffer;
    } else {
        ctx->cache.storedIndexBuffer = ctx->cache.indexBuffer;
    }
}

LOCAL void rx__glCacheRestoreBufferBinding(rx_OpenGlCtx* ctx, GLenum target) {
    if (target == GL_ARRAY_BUFFER) {
        if (ctx->cache.storedVertexBuffer != 0) {
            // we only care about restoring valid ids
            rx__glCacheBindBuffer(ctx, target, ctx->cache.storedVertexBuffer);
            ctx->cache.storedVertexBuffer = 0;
        }
    } else {
        if (ctx->cache.storedIndexBuffer != 0) {
            // we only care about restoring valid ids
            rx__glCacheBindBuffer(ctx, target, ctx->cache.storedIndexBuffer);
            ctx->cache.storedIndexBuffer = 0;
        }
    }
}

LOCAL GLenum rx__glBufferTarget(rx_bufferUsage usage) {
    if ((usage & rx_bufferUsage_vertex) != 0) {
        return GL_ARRAY_BUFFER;
    }
    if ((usage & rx_bufferUsage_index) != 0) {
        return GL_ELEMENT_ARRAY_BUFFER;
    }
    ASSERT(!"Unsupported buffer type");
    return 0;
}

LOCAL GLenum rx__glBufferUsage(rx_bufferUsage usage) {
    if ((usage & rx_bufferUsage_immutable) != 0) {
        return GL_STATIC_DRAW;
    }
    if ((usage & GL_STREAM_DRAW) != 0) {
        return GL_STREAM_DRAW;
    }
    return GL_DYNAMIC_DRAW;
}

LOCAL bx rx__glMakeBuffer(rx_Ctx* baseCtx, rx_Buffer* buffer, rx_BufferDesc* desc) {
    ASSERT(baseCtx && buffer && desc);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    rx__oglCheckErrors();

    const GLenum glTarget = rx__glBufferTarget(desc->usage);
    const GLenum glUsage  = rx__glBufferUsage(desc->usage);

    GLuint glBuffer = 0;
    glGenBuffers(1, &glBuffer);
    ASSERT(glBuffer != 0); 
    buffer->gl.handle = glBuffer;
    rx__oglCheckErrors();

    rx__glCacheStoreBufferBinding(ctx, glTarget);
    rx__glCacheBindBuffer(ctx, glTarget, glBuffer);
    glBufferData(glTarget, desc->size, 0, glUsage);
    rx__oglCheckErrors();
    //_sg_gl_cache_restore_buffer_binding(gl_target);
    //rx__oglCheckErrors();

    return true;
}

LOCAL void rx__glUpdateBuffer(rx_Ctx* baseCtx, rx_Buffer* buffer, mms offset, rx_Range range) {
    ASSERT(baseCtx && buffer);
    rx_OpenGlCtx* ctx = (rx_OpenGlCtx*) baseCtx;
    GLenum glTarget = rx__glBufferTarget(buffer->usage);
    rx__glCacheStoreBufferBinding(ctx, glTarget);
    rx__glCacheBindBuffer(ctx, glTarget, buffer->gl.handle);
    glBufferSubData(glTarget, offset, (GLsizeiptr)range.size, range.ptr);
    rx__oglCheckErrors();
    rx__glCacheRestoreBufferBinding(ctx, glTarget);
    rx__oglCheckErrors();
}

#endif

#if RX_METAL
typedef struct rx_MtlCtx {
    rx_Ctx base;
} rx_MtlCtx;

#endif

#define rx_callBknFn(FNNAME,...) rx__gl##FNNAME(__VA_ARGS__)

static rx_Ctx* rx__ctx;

void rx_setup(rx_SetupDesc* desc) {
    ASSERT(!rx__ctx);
    ASSERT(desc);

    // set defaults
    rx_SetupDesc descWithDefaults = *desc;

    descWithDefaults.maxBuffers = rx_valueOrDefault(descWithDefaults.maxBuffers, RX_DEFAULT_MAX_BUFFERS);
    descWithDefaults.maxTextures = rx_valueOrDefault(descWithDefaults.maxTextures, RX_DEFAULT_MAX_TEXTURES);
    descWithDefaults.maxTextureViews = rx_valueOrDefault(descWithDefaults.maxTextureViews, RX_DEFAULT_MAX_TEXTURE_VIEWS);
    descWithDefaults.maxPasses = rx_valueOrDefault(descWithDefaults.maxPasses, RX_DEFAULT_MAX_PASSES);
    descWithDefaults.maxPerFramePasses = rx_valueOrDefault(descWithDefaults.maxPerFramePasses, RX_DEFAULT_MAX_PER_FRAME_PASSES);

    BaseMemory baseMem = os_getBaseMemory();
    Arena* arena = mem_makeArena(&baseMem, MEGABYTE(6));

    rx_Ctx* ctx = rx__ctx = rx__create(arena, &descWithDefaults);
    rx__setupBackend(ctx, &descWithDefaults);
    ctx->arena = arena;

    rx__poolInit(ctx->arena, &ctx->buffers, descWithDefaults.maxBuffers);

#if 0
    arrMake(ctx->arena, &ctx->buffers, descWithDefaults.maxBuffers);
    arrMake(ctx->arena, &ctx->textures, descWithDefaults.maxTextures);
    arrMake(ctx->arena, &ctx->textureViews, descWithDefaults.maxTextureViews);
    arrMake(ctx->arena, &ctx->passDefs, descWithDefaults.maxPasses);
    arrMake(ctx->arena, &ctx->currentFramePasses, descWithDefaults.maxPerFramePasses);
#endif

}

void rx_shutdown(void) {
    ASSERT(rx__ctx);
    rx_Ctx* ctx = rx__ctx;
    rx_callBknFn(DiscardContext, ctx);
    // Destroy arena and free up all alocated memory with it
    mem_destroyArena(ctx->arena);
    rx__ctx = NULL;
}

API rx_buffer rx_makeBuffer(const rx_BufferDesc* desc) {
    ASSERT(rx__ctx);
    rx_Ctx* ctx = rx__ctx;

    // fill defaults for bufferDesc
    rx_BufferDesc descWithDefaults = *desc;

    // get free buffer slot
    u32 idx = rx__queuePull(&ctx->buffers.freeIndicies);
    ASSERT(idx != RX__INVALID_INDEX && "No free buffer available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_buffer) {0};
    }

    rx_Buffer* buffer = &ctx->buffers.elements[idx];
    buffer->usage = desc->usage;
    buffer->size = desc->size;
    buffer->gen = maxVal(1, buffer->gen + 1);

    // call backend specific function
    bx success = rx_callBknFn(MakeBuffer, ctx, buffer, &descWithDefaults);
    ASSERT(success);

    rx_buffer bufferHandle = {
        .idx = idx,
        .gen = buffer->gen
    };

    return bufferHandle;
    #if 0

    RX_ASSERT(rx__ctx);
    RX_ASSERT(desc);
    // get free index
    u32 idx   = rx__queuePull(&rx__ctx->textures.freeIndicies);
    RX_ASSERT(idx != RX__INVALID_INDEX && "No free buffer available");
    if (idx == RX__INVALID_INDEX) {
        return (rx_buffer) {0};
    }
    rx_buffer   handle = {rx_makeHandle(idx, 0)};
    rx__Buffer* buffer = rx__getBuffer(rx__ctx, handle);
    rx__pushGeneration(&buffer->info);
    // create hal buffer
    rx__ctx->halApi.makeBuffer(&buffer->halBuffer, &(rx_HalBufferDesc) {
        .label      = desc->label,
        .device     = &rx__ctx->halDevice,
        .usage      = desc->usage | rx_bufferUsage_copyDst,
        .size       = desc->size,
    });
    // we do alloc in a specific upload phase
    // for legacy APIs if there is data we would upload it right away
    buffer->allocHandle.id = 0;

    // should we really do it like this? maybe we should have more specific buffers
    // for specific usecases like the legacy APIs
    rx_allocForBuffer(&buffer->halBuffer, &rx__ctx->bufferMem);
    #endif
}

API void rx_updateBuffer(rx_buffer buffer, mms offset, rx_Range range) {
    ASSERT(rx__ctx);
    ASSERT(range.ptr && "upload ptr is NULL");
    ASSERT(range.size > 0 && "upload size is zero");
    rx_Ctx* ctx = rx__ctx;
    rx_Buffer* bufferContent = &ctx->buffers.elements[buffer.idx];
    ASSERT(bufferContent->gen == buffer.gen);
    ASSERT(bufferContent->size > (offset) && "upload offset is bigger then the buffer");
    ASSERT(bufferContent->size >= (offset + range.size) && "upload size does not fit in the buffer");
    rx_callBknFn(UpdateBuffer, ctx, bufferContent, offset, range);
}


#if 0
LOCAL rx_renderPipeline rx_makeRenderPipeline(rx_RenderPipelineDesc* desc) {
    ASSERT(rx__ctx);
    ASSERT(desc);

}
#endif