





















#ifndef AVCODEC_DXVA_H
#define AVCODEC_DXVA_H

#include <stdint.h>

#include <d3d9.h>
#include <dxva2api.h>

#define FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG 1 ///< Work around for DXVA2 and old UVD/UVD+ ATI video cards







struct dxva_context {
    


    IDirectXVideoDecoder *decoder;

    


    const DXVA2_ConfigPictureDecode *cfg;

    


    unsigned surface_count;

    


    LPDIRECT3DSURFACE9 *surface;

    


    uint64_t workaround;

    


    unsigned report_id;
};

#endif 
