















#ifndef ANDROID_HARDWARE_CAMERA_PARAMETERS_H
#define ANDROID_HARDWARE_CAMERA_PARAMETERS_H

#include <utils/KeyedVector.h>
#include <utils/String8.h>

namespace android {

struct Size {
    int width;
    int height;

    Size() {
        width = 0;
        height = 0;
    }

    Size(int w, int h) {
        width = w;
        height = h;
    }
};

class CameraParameters
{
public:
    CameraParameters();
    CameraParameters(const String8 &params) { unflatten(params); }
    ~CameraParameters();

    String8 flatten() const;
    void unflatten(const String8 &params);

    void set(const char *key, const char *value);
    void set(const char *key, int value);
    void setFloat(const char *key, float value);
    const char *get(const char *key) const;
    int getInt(const char *key) const;
    float getFloat(const char *key) const;

    void remove(const char *key);

    void setPreviewSize(int width, int height);
    void getPreviewSize(int *width, int *height) const;
    void getSupportedPreviewSizes(Vector<Size> &sizes) const;
    void setPreviewFrameRate(int fps);
    int getPreviewFrameRate() const;
    void getPreviewFpsRange(int *min_fps, int *max_fps) const;
    void setPreviewFormat(const char *format);
    const char *getPreviewFormat() const;
    void setPictureSize(int width, int height);
    void getPictureSize(int *width, int *height) const;
    void getSupportedPictureSizes(Vector<Size> &sizes) const;
    void setPictureFormat(const char *format);
    const char *getPictureFormat() const;

    void dump() const;
    status_t dump(int fd, const Vector<String16>& args) const;

    
    
    

    
    
    static const char KEY_PREVIEW_SIZE[];
    
    
    static const char KEY_SUPPORTED_PREVIEW_SIZES[];
    
    
    
    
    
    static const char KEY_PREVIEW_FPS_RANGE[];
    
    
    
    
    
    
    
    static const char KEY_SUPPORTED_PREVIEW_FPS_RANGE[];
    
    
    
    static const char KEY_PREVIEW_FORMAT[];
    
    
    static const char KEY_SUPPORTED_PREVIEW_FORMATS[];
    
    
    
    static const char KEY_PREVIEW_FRAME_RATE[];
    
    
    static const char KEY_SUPPORTED_PREVIEW_FRAME_RATES[];
    
    
    static const char KEY_PICTURE_SIZE[];
    
    
    static const char KEY_SUPPORTED_PICTURE_SIZES[];
    
    
    
    static const char KEY_PICTURE_FORMAT[];
    
    
    static const char KEY_SUPPORTED_PICTURE_FORMATS[];
    
    
    static const char KEY_JPEG_THUMBNAIL_WIDTH[];
    
    
    static const char KEY_JPEG_THUMBNAIL_HEIGHT[];
    
    
    
    static const char KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES[];
    
    
    
    static const char KEY_JPEG_THUMBNAIL_QUALITY[];
    
    
    
    static const char KEY_JPEG_QUALITY[];
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static const char KEY_ROTATION[];
    
    
    
    static const char KEY_GPS_LATITUDE[];
    
    
    
    static const char KEY_GPS_LONGITUDE[];
    
    
    
    static const char KEY_GPS_ALTITUDE[];
    
    
    
    static const char KEY_GPS_TIMESTAMP[];
    
    
    static const char KEY_GPS_PROCESSING_METHOD[];
    
    
    static const char KEY_WHITE_BALANCE[];
    
    
    static const char KEY_SUPPORTED_WHITE_BALANCE[];
    
    
    static const char KEY_EFFECT[];
    
    
    static const char KEY_SUPPORTED_EFFECTS[];
    
    
    static const char KEY_ANTIBANDING[];
    
    
    static const char KEY_SUPPORTED_ANTIBANDING[];
    
    
    static const char KEY_SCENE_MODE[];
    
    
    static const char KEY_SUPPORTED_SCENE_MODES[];
    
    
    static const char KEY_FLASH_MODE[];
    
    
    static const char KEY_SUPPORTED_FLASH_MODES[];
    
    
    
    
    static const char KEY_FOCUS_MODE[];
    
    
    static const char KEY_SUPPORTED_FOCUS_MODES[];
    
    
    static const char KEY_FOCAL_LENGTH[];
    
    
    static const char KEY_HORIZONTAL_VIEW_ANGLE[];
    
    
    static const char KEY_VERTICAL_VIEW_ANGLE[];
    
    
    static const char KEY_EXPOSURE_COMPENSATION[];
    
    
    static const char KEY_MAX_EXPOSURE_COMPENSATION[];
    
    
    static const char KEY_MIN_EXPOSURE_COMPENSATION[];
    
    
    
    
    static const char KEY_EXPOSURE_COMPENSATION_STEP[];
    
    
    static const char KEY_ZOOM[];
    
    
    static const char KEY_MAX_ZOOM[];
    
    
    
    
    
    static const char KEY_ZOOM_RATIOS[];
    
    
    
    static const char KEY_ZOOM_SUPPORTED[];
    
    
    
    
    
    
    static const char KEY_SMOOTH_ZOOM_SUPPORTED[];

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static const char KEY_FOCUS_DISTANCES[];

    
    
    
    static const char KEY_VIDEO_FRAME_FORMAT[];

    
    static const char TRUE[];

    
    static const char FOCUS_DISTANCE_INFINITY[];

    
    static const char WHITE_BALANCE_AUTO[];
    static const char WHITE_BALANCE_INCANDESCENT[];
    static const char WHITE_BALANCE_FLUORESCENT[];
    static const char WHITE_BALANCE_WARM_FLUORESCENT[];
    static const char WHITE_BALANCE_DAYLIGHT[];
    static const char WHITE_BALANCE_CLOUDY_DAYLIGHT[];
    static const char WHITE_BALANCE_TWILIGHT[];
    static const char WHITE_BALANCE_SHADE[];

    
    static const char EFFECT_NONE[];
    static const char EFFECT_MONO[];
    static const char EFFECT_NEGATIVE[];
    static const char EFFECT_SOLARIZE[];
    static const char EFFECT_SEPIA[];
    static const char EFFECT_POSTERIZE[];
    static const char EFFECT_WHITEBOARD[];
    static const char EFFECT_BLACKBOARD[];
    static const char EFFECT_AQUA[];

    
    static const char ANTIBANDING_AUTO[];
    static const char ANTIBANDING_50HZ[];
    static const char ANTIBANDING_60HZ[];
    static const char ANTIBANDING_OFF[];

    
    
    static const char FLASH_MODE_OFF[];
    
    
    static const char FLASH_MODE_AUTO[];
    
    
    static const char FLASH_MODE_ON[];
    
    static const char FLASH_MODE_RED_EYE[];
    
    
    static const char FLASH_MODE_TORCH[];

    
    static const char SCENE_MODE_AUTO[];
    static const char SCENE_MODE_ACTION[];
    static const char SCENE_MODE_PORTRAIT[];
    static const char SCENE_MODE_LANDSCAPE[];
    static const char SCENE_MODE_NIGHT[];
    static const char SCENE_MODE_NIGHT_PORTRAIT[];
    static const char SCENE_MODE_THEATRE[];
    static const char SCENE_MODE_BEACH[];
    static const char SCENE_MODE_SNOW[];
    static const char SCENE_MODE_SUNSET[];
    static const char SCENE_MODE_STEADYPHOTO[];
    static const char SCENE_MODE_FIREWORKS[];
    static const char SCENE_MODE_SPORTS[];
    static const char SCENE_MODE_PARTY[];
    static const char SCENE_MODE_CANDLELIGHT[];
    
    
    static const char SCENE_MODE_BARCODE[];

    
    static const char PIXEL_FORMAT_YUV422SP[];
    static const char PIXEL_FORMAT_YUV420SP[]; 
    static const char PIXEL_FORMAT_YUV422I[]; 
    static const char PIXEL_FORMAT_RGB565[];
    static const char PIXEL_FORMAT_JPEG[];

    
    
    
    static const char FOCUS_MODE_AUTO[];
    
    
    static const char FOCUS_MODE_INFINITY[];
    
    
    static const char FOCUS_MODE_MACRO[];
    
    
    
    
    static const char FOCUS_MODE_FIXED[];
    
    
    
    static const char FOCUS_MODE_EDOF[];
    
    
    
    
    
    
    
    static const char FOCUS_MODE_CONTINUOUS_VIDEO[];

private:
    DefaultKeyedVector<String8,String8>    mMap;
};

}; 

#endif
