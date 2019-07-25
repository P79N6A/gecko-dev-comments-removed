










#include "vpx_scale/yv12config.h"
#include "vpx_mem/vpx_mem.h"








int
vp8_yv12_de_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf)
{
    if (ybf)
    {
        vpx_free(ybf->buffer_alloc);

        


        vpx_memset (ybf, 0, sizeof (YV12_BUFFER_CONFIG));
    }
    else
    {
        return -1;
    }

    return 0;
}




int
vp8_yv12_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf, int width, int height, int border)
{


    if (ybf)
    {
        int y_stride = ((width + 2 * border) + 31) & ~31;
        int yplane_size = (height + 2 * border) * y_stride;
        int uv_width = width >> 1;
        int uv_height = height >> 1;
        

        int uv_stride = y_stride >> 1;
        int uvplane_size = (uv_height + border) * uv_stride;

        vp8_yv12_de_alloc_frame_buffer(ybf);

        





        if ((width & 0xf) | (height & 0xf) | (border & 0x1f))
            return -3;

        ybf->y_width  = width;
        ybf->y_height = height;
        ybf->y_stride = y_stride;

        ybf->uv_width = uv_width;
        ybf->uv_height = uv_height;
        ybf->uv_stride = uv_stride;

        ybf->border = border;
        ybf->frame_size = yplane_size + 2 * uvplane_size;

        ybf->buffer_alloc = (unsigned char *) vpx_memalign(32, ybf->frame_size);

        if (ybf->buffer_alloc == NULL)
            return -1;

        ybf->y_buffer = ybf->buffer_alloc + (border * y_stride) + border;
        ybf->u_buffer = ybf->buffer_alloc + yplane_size + (border / 2  * uv_stride) + border / 2;
        ybf->v_buffer = ybf->buffer_alloc + yplane_size + uvplane_size + (border / 2  * uv_stride) + border / 2;

        ybf->corrupted = 0; 
    }
    else
    {
        return -2;
    }

    return 0;
}
