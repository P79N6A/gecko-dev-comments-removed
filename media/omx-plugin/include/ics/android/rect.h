
















#ifndef ANDROID_RECT_H
#define ANDROID_RECT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ARect {
#ifdef __cplusplus
    typedef int32_t value_type;
#endif
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} ARect;

#ifdef __cplusplus
};
#endif

#endif
