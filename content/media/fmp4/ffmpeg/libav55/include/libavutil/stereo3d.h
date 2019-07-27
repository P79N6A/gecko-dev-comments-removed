



















#include <stdint.h>

#include "frame.h"




enum AVStereo3DType {
    


    AV_STEREO3D_2D,

    







    AV_STEREO3D_SIDEBYSIDE,

    







    AV_STEREO3D_TOPBOTTOM,

    








    AV_STEREO3D_FRAMESEQUENCE,

    







    AV_STEREO3D_CHECKERBOARD,

    








    AV_STEREO3D_SIDEBYSIDE_QUINCUNX,

    







    AV_STEREO3D_LINES,

    







    AV_STEREO3D_COLUMNS,
};





#define AV_STEREO3D_FLAG_INVERT     (1 << 0)








typedef struct AVStereo3D {
    


    enum AVStereo3DType type;

    


    int flags;
} AVStereo3D;







AVStereo3D *av_stereo3d_alloc(void);








AVStereo3D *av_stereo3d_create_side_data(AVFrame *frame);
