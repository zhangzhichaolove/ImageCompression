//
// Created by peakchao on 2019/3/22.
//

#include <jni.h>
#include <string>
#include "turbojpeg.h"
#include "jpeglib.h"
#include <android/bitmap.h>
#include <android/log.h>
#include <csetjmp>
#include <setjmp.h>
#include "peak_chao_picturecompression_CompressUtil.h"

#define LOG_TAG  "C_TAG"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
typedef u_int8_t BYTE;
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr *my_error_ptr;


int generateJPEG(BYTE *data, int w, int h, jint quality, const char *location, jint quality1) {
    int nComponent = 3;
    struct jpeg_compress_struct jcs;
    //自定义的error
    struct my_error_mgr jem;

    jcs.err = jpeg_std_error(&jem.pub);

    if (setjmp(jem.setjmp_buffer)) {
        return 0;
    }
    //为JPEG对象分配空间并初始化
    jpeg_create_compress(&jcs);
    //获取文件信息
    FILE *f = fopen(location, "wb");
    if (f == NULL) {
        return 0;
    }

    //指定压缩数据源
    jpeg_stdio_dest(&jcs, f);
    jcs.image_width = w;
    jcs.image_height = h;

    jcs.arith_code = false;
    jcs.input_components = nComponent;
    jcs.in_color_space = JCS_RGB;

    jpeg_set_defaults(&jcs);
    jcs.optimize_coding = quality;

    //为压缩设定参数，包括图像大小，颜色空间
    jpeg_set_quality(&jcs, quality, true);
    //开始压缩
    jpeg_start_compress(&jcs, true);
    JSAMPROW row_point[1];
    int row_stride;
    row_stride = jcs.image_width * nComponent;
    while (jcs.next_scanline < jcs.image_height) {
        row_point[0] = &data[jcs.next_scanline * row_stride];
        jpeg_write_scanlines(&jcs, row_point, 1);
    }

    if (jcs.optimize_coding) {
        LOGD("使用了哈夫曼算法完成压缩");
    } else {
        LOGD("未使用哈夫曼算法");
    }
    //压缩完毕
    jpeg_finish_compress(&jcs);
    //释放资源
    jpeg_destroy_compress(&jcs);
    fclose(f);
    return 1;
}

const char *jstringToString(JNIEnv *env, jstring jstr) {
    char *ret;
    const char *tempStr = env->GetStringUTFChars(jstr, NULL);
    jsize len = env->GetStringUTFLength(jstr);
    if (len > 0) {
        ret = (char *) malloc(len + 1);
        memcpy(ret, tempStr, len);
        ret[len] = 0;
    }
    env->ReleaseStringUTFChars(jstr, tempStr);
    return ret;
}

extern "C"
JNIEXPORT jint JNICALL
Java_peak_chao_picturecompression_CompressUtil_compressBitmap(JNIEnv *env, jclass,
                                                              jobject bitmap, jint optimize,
                                                              jstring destFile_) {
    AndroidBitmapInfo androidBitmapInfo;
    BYTE *pixelsColor;
    int ret;
    BYTE *data;
    BYTE *tmpData;
    const char *dstFileName = jstringToString(env, destFile_);
    //解码Android Bitmap信息
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &androidBitmapInfo)) < 0) {
        LOGD("AndroidBitmap_getInfo() failed error=%d", ret);
        return ret;
    }
    if ((ret = AndroidBitmap_lockPixels(env, bitmap, reinterpret_cast<void **>(&pixelsColor))) <
        0) {
        LOGD("AndroidBitmap_lockPixels() failed error=%d", ret);
        return ret;
    }

    LOGD("bitmap: width=%d,height=%d,size=%d , format=%d ",
         androidBitmapInfo.width, androidBitmapInfo.height,
         androidBitmapInfo.height * androidBitmapInfo.width,
         androidBitmapInfo.format);

    BYTE r, g, b;
    int color;

    int w, h, format;
    w = androidBitmapInfo.width;
    h = androidBitmapInfo.height;
    format = androidBitmapInfo.format;

    data = (BYTE *) malloc(androidBitmapInfo.width * androidBitmapInfo.height * 3);
    tmpData = data;
    // 将bitmap转换为rgb数据
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            //只处理 RGBA_8888
            if (format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
                color = (*(int *) (pixelsColor));
                // 这里取到的颜色对应的 A B G R  各占8位
                b = (color >> 16) & 0xFF;
                g = (color >> 8) & 0xFF;
                r = (color >> 0) & 0xFF;
                *data = r;
                *(data + 1) = g;
                *(data + 2) = b;

                data += 3;
                pixelsColor += 4;

            } else {
                return -2;
            }
        }
    }
    AndroidBitmap_unlockPixels(env, bitmap);
    //进行压缩
    ret = generateJPEG(tmpData, w, h, optimize, dstFileName, optimize);
    free((void *) dstFileName);
    free((void *) tmpData);
    return ret;
}
