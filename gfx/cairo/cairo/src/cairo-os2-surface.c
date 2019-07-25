





































#include "cairoint.h"

#include "cairo-os2-private.h"
#include "cairo-error-private.h"

#if CAIRO_HAS_FC_FONT
#include <fontconfig/fontconfig.h>
#endif

#include <float.h>
#ifdef BUILD_CAIRO_DLL
# include "cairo-os2.h"
# ifndef __WATCOMC__
#  include <emx/startup.h>
# endif
#endif














static int cairo_os2_initialization_count = 0;

static inline void
DisableFPUException (void)
{
    unsigned short usCW;

    






    usCW = _control87 (0, 0);
    usCW = usCW | EM_INVALID | 0x80;
    _control87 (usCW, MCW_EM | 0x80);
}











cairo_public void
cairo_os2_init (void)
{
    

    cairo_os2_initialization_count++;
    if (cairo_os2_initialization_count > 1) return;

    DisableFPUException ();

#if CAIRO_HAS_FC_FONT
    
    FcInit ();
#endif

    CAIRO_MUTEX_INITIALIZE ();
}











cairo_public void
cairo_os2_fini (void)
{
    

    if (cairo_os2_initialization_count <= 0) return;
    cairo_os2_initialization_count--;
    if (cairo_os2_initialization_count > 0) return;

    DisableFPUException ();

    cairo_debug_reset_static_data ();

#if CAIRO_HAS_FC_FONT
# if HAVE_FCFINI
    
    FcFini ();
# endif
#endif

#ifdef __WATCOMC__
    







    _heapshrink ();
#else
    


    _heapmin ();
#endif
}









void *_buffer_alloc (size_t a, size_t b, const unsigned int size)
{
    size_t nbytes;
    void  *buffer = NULL;

    if (!a || !b || !size ||
        a >= INT32_MAX / b || a*b >= INT32_MAX / size) {
        return NULL;
    }
    nbytes = a * b * size;

#ifdef OS2_USE_PLATFORM_ALLOC
    



#ifdef OS2_HIGH_MEMORY
    if (!DosAllocMem (&buffer, nbytes,
                      OBJ_ANY | PAG_READ | PAG_WRITE | PAG_COMMIT))
        return buffer;
#endif
    if (DosAllocMem (&buffer, nbytes,
                     PAG_READ | PAG_WRITE | PAG_COMMIT))
        return NULL;
#else
    
    buffer = malloc (nbytes);
    if (buffer) {
        memset (buffer, 0, nbytes);
    }
#endif
    return buffer;
}





void _buffer_free (void *buffer)
{
#ifdef OS2_USE_PLATFORM_ALLOC
    DosFreeMem (buffer);
#else
    free (buffer);
#endif
}







#ifdef BUILD_CAIRO_DLL



#ifdef __WATCOMC__
unsigned _System
LibMain (unsigned hmod,
         unsigned termination)
#else
unsigned long _System
_DLL_InitTerm (unsigned long hModule,
               unsigned long termination)
#endif
{
    if (termination) {
        
        cairo_os2_fini ();

#ifndef __WATCOMC__
        
        __ctordtorTerm ();
        _CRT_term ();
#endif
        return 1;
    } else {
        
#ifndef __WATCOMC__
        
        if (_CRT_init () != 0)
            return 0;
        __ctordtorInit ();
#endif

        cairo_os2_init ();
        return 1;
    }
}

#endif 








static const cairo_surface_backend_t cairo_os2_surface_backend;







BOOL APIENTRY GpiEnableYInversion (HPS hps, LONG lHeight);
LONG APIENTRY GpiQueryYInversion (HPS hps);

#ifdef __WATCOMC__

LONG APIENTRY GpiDrawBits (HPS hps,
                           PVOID pBits,
                           PBITMAPINFO2 pbmiInfoTable,
                           LONG lCount,
                           PPOINTL aptlPoints,
                           LONG lRop,
                           ULONG flOptions);
#endif

static void
_cairo_os2_surface_blit_pixels (cairo_os2_surface_t *surface,
                                HPS                  hps_begin_paint,
                                PRECTL               prcl_begin_paint_rect)
{
    POINTL aptlPoints[4];
    LONG   lOldYInversion;
    LONG   rc = GPI_OK;

    
    if (prcl_begin_paint_rect->xLeft < 0)
        prcl_begin_paint_rect->xLeft = 0;
    if (prcl_begin_paint_rect->yBottom < 0)
        prcl_begin_paint_rect->yBottom = 0;
    if (prcl_begin_paint_rect->xRight > (LONG) surface->bitmap_info.cx)
        prcl_begin_paint_rect->xRight = (LONG) surface->bitmap_info.cx;
    if (prcl_begin_paint_rect->yTop > (LONG) surface->bitmap_info.cy)
        prcl_begin_paint_rect->yTop = (LONG) surface->bitmap_info.cy;

    
    if (prcl_begin_paint_rect->xLeft   >= prcl_begin_paint_rect->xRight ||
        prcl_begin_paint_rect->yBottom >= prcl_begin_paint_rect->yTop)
        return;

    
    *((PRECTL)&aptlPoints[0]) = *prcl_begin_paint_rect;
    *((PRECTL)&aptlPoints[2]) = *prcl_begin_paint_rect;

    
    aptlPoints[1].x -= 1;
    aptlPoints[1].y -= 1;

    


    lOldYInversion = GpiQueryYInversion (hps_begin_paint);
    GpiEnableYInversion (hps_begin_paint, surface->bitmap_info.cy-1);

    
#if 0
    {
        int x, y;
        unsigned char *pixels;

        pixels = surface->pixels;
        for (x = 0; x < surface->bitmap_info.cx; x++) {
            for (y = 0; y < surface->bitmap_info.cy; y++) {
                if ((x == 0) ||
                    (y == 0) ||
                    (x == y) ||
                    (x >= surface->bitmap_info.cx-1) ||
                    (y >= surface->bitmap_info.cy-1))
                {
                    pixels[y*surface->bitmap_info.cx*4+x*4] = 255;
                }
            }
        }
    }
#endif
    if (!surface->use_24bpp) {
        rc = GpiDrawBits (hps_begin_paint,
                          surface->pixels,
                          &(surface->bitmap_info),
                          4,
                          aptlPoints,
                          ROP_SRCCOPY,
                          BBO_IGNORE);
        if (rc != GPI_OK)
            surface->use_24bpp = TRUE;
    }

    if (surface->use_24bpp) {
        






        BITMAPINFO2       bmpinfo;
        unsigned char    *pchPixBuf;
        unsigned char    *pchTarget;
        ULONG            *pulSource;
        ULONG             ulX;
        ULONG             ulY;
        ULONG             ulPad;

        
        bmpinfo = surface->bitmap_info;
        bmpinfo.cBitCount = 24;

        



        ulX = (((bmpinfo.cx * bmpinfo.cBitCount) + 31) / 32) * 4;
        bmpinfo.cbImage = ulX * bmpinfo.cy;
        ulPad = ulX - bmpinfo.cx * 3;

        




        pchPixBuf = (unsigned char *)_buffer_alloc (1, 1,
                                        bmpinfo.cbImage + (ulPad ? 0 : 1));

        if (pchPixBuf) {
            



            pchTarget = pchPixBuf;
            pulSource = (ULONG*)surface->pixels;
            for (ulY = bmpinfo.cy; ulY; ulY--) {
                for (ulX = bmpinfo.cx; ulX; ulX--) {
                    *((ULONG*)pchTarget) = *pulSource++;
                    pchTarget += 3;
                }
                pchTarget += ulPad;
            }

            rc = GpiDrawBits (hps_begin_paint,
                              pchPixBuf,
                              &bmpinfo,
                              4,
                              aptlPoints,
                              ROP_SRCCOPY,
                              BBO_IGNORE);
            if (rc != GPI_OK)
                surface->use_24bpp = FALSE;

            _buffer_free (pchPixBuf);
        }
    }

    
    GpiEnableYInversion (hps_begin_paint, lOldYInversion);
}

static void
_cairo_os2_surface_get_pixels_from_screen (cairo_os2_surface_t *surface,
                                           HPS                  hps_begin_paint,
                                           PRECTL               prcl_begin_paint_rect)
{
    HPS hps;
    HDC hdc;
    SIZEL sizlTemp;
    HBITMAP hbmpTemp;
    BITMAPINFO2 bmi2Temp;
    POINTL aptlPoints[4];
    int y;
    unsigned char *pchTemp;

    











    
    hdc = DevOpenDC (0, OD_MEMORY,"*",0L, NULL, NULLHANDLE);
    if (!hdc) {
        return;
    }

    
    sizlTemp.cx = prcl_begin_paint_rect->xRight - prcl_begin_paint_rect->xLeft;
    sizlTemp.cy = prcl_begin_paint_rect->yTop - prcl_begin_paint_rect->yBottom;
    hps = GpiCreatePS (0,
                       hdc,
                       &sizlTemp,
                       PU_PELS | GPIT_NORMAL | GPIA_ASSOC);
    if (!hps) {
        DevCloseDC (hdc);
        return;
    }

    
    
    memset (&bmi2Temp, 0, sizeof (bmi2Temp));
    bmi2Temp.cbFix = sizeof (BITMAPINFOHEADER2);
    bmi2Temp.cx = sizlTemp.cx;
    bmi2Temp.cy = sizlTemp.cy;
    bmi2Temp.cPlanes = 1;
    bmi2Temp.cBitCount = 32;

    hbmpTemp = GpiCreateBitmap (hps,
                                (PBITMAPINFOHEADER2) &bmi2Temp,
                                0,
                                NULL,
                                NULL);

    if (!hbmpTemp) {
        GpiDestroyPS (hps);
        DevCloseDC (hdc);
        return;
    }

    
    GpiSetBitmap (hps, hbmpTemp);

    
    aptlPoints[0].x = 0;
    aptlPoints[0].y = 0;

    aptlPoints[1].x = sizlTemp.cx;
    aptlPoints[1].y = sizlTemp.cy;

    
    aptlPoints[2].x = prcl_begin_paint_rect->xLeft;
    aptlPoints[2].y = surface->bitmap_info.cy - prcl_begin_paint_rect->yBottom;

    aptlPoints[3].x = prcl_begin_paint_rect->xRight;
    aptlPoints[3].y = surface->bitmap_info.cy - prcl_begin_paint_rect->yTop;

    
    GpiBitBlt (hps,
               hps_begin_paint,
               4,
               aptlPoints,
               ROP_SRCCOPY,
               BBO_IGNORE);

    
    pchTemp =
        surface->pixels +
        (prcl_begin_paint_rect->yBottom)*surface->bitmap_info.cx*4 +
        prcl_begin_paint_rect->xLeft*4;
    for (y = 0; y < sizlTemp.cy; y++) {
        
        GpiQueryBitmapBits (hps,
                            sizlTemp.cy - y - 1, 
                            1,                   
                            (PBYTE)pchTemp,
                            &bmi2Temp);

        
        pchTemp += surface->bitmap_info.cx*4;
    }

    
    GpiSetBitmap (hps, (HBITMAP) NULL);
    GpiDeleteBitmap (hbmpTemp);
    GpiDestroyPS (hps);
    DevCloseDC (hdc);
}

static cairo_status_t
_cairo_os2_surface_acquire_source_image (void                   *abstract_surface,
                                         cairo_image_surface_t **image_out,
                                         void                  **image_extra)
{
    cairo_os2_surface_t *local_os2_surface;

    local_os2_surface = (cairo_os2_surface_t *) abstract_surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    }

    DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT);

    
    local_os2_surface->pixel_array_lend_count++;

    *image_out = local_os2_surface->image_surface;
    *image_extra = NULL;

    DosReleaseMutexSem (local_os2_surface->hmtx_use_private_fields);

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_os2_surface_release_source_image (void                  *abstract_surface,
                                         cairo_image_surface_t *image,
                                         void                  *image_extra)
{
    cairo_os2_surface_t *local_os2_surface;

    local_os2_surface = (cairo_os2_surface_t *) abstract_surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return;
    }

    
    DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT);

    if (local_os2_surface->pixel_array_lend_count > 0)
        local_os2_surface->pixel_array_lend_count--;
    DosPostEventSem (local_os2_surface->hev_pixel_array_came_back);

    DosReleaseMutexSem (local_os2_surface->hmtx_use_private_fields);
    return;
}

static cairo_status_t
_cairo_os2_surface_acquire_dest_image (void                     *abstract_surface,
                                       cairo_rectangle_int_t    *interest_rect,
                                       cairo_image_surface_t   **image_out,
                                       cairo_rectangle_int_t    *image_rect,
                                       void                    **image_extra)
{
    cairo_os2_surface_t *local_os2_surface;

    local_os2_surface = (cairo_os2_surface_t *) abstract_surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    }

    DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT);

    
    local_os2_surface->pixel_array_lend_count++;

    *image_out = local_os2_surface->image_surface;
    *image_extra = NULL;

    image_rect->x = 0;
    image_rect->y = 0;
    image_rect->width = local_os2_surface->bitmap_info.cx;
    image_rect->height = local_os2_surface->bitmap_info.cy;

    DosReleaseMutexSem (local_os2_surface->hmtx_use_private_fields);

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_os2_surface_release_dest_image (void                    *abstract_surface,
                                       cairo_rectangle_int_t   *interest_rect,
                                       cairo_image_surface_t   *image,
                                       cairo_rectangle_int_t   *image_rect,
                                       void                    *image_extra)
{
    cairo_os2_surface_t *local_os2_surface;
    RECTL rclToBlit;

    local_os2_surface = (cairo_os2_surface_t *) abstract_surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return;
    }

    




    if (local_os2_surface->blit_as_changes) {
        
        if (DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT)!=NO_ERROR) {
            
            return;
        }

        if (local_os2_surface->hwnd_client_window) {
            





            rclToBlit.xLeft = interest_rect->x;
            rclToBlit.xRight = interest_rect->x+interest_rect->width; 
            rclToBlit.yTop = local_os2_surface->bitmap_info.cy - (interest_rect->y);
            rclToBlit.yBottom = local_os2_surface->bitmap_info.cy - (interest_rect->y+interest_rect->height); 

            WinInvalidateRect (local_os2_surface->hwnd_client_window,
                               &rclToBlit,
                               FALSE);
        } else {
            







            rclToBlit.xLeft = interest_rect->x;
            rclToBlit.xRight = interest_rect->x+interest_rect->width; 
            rclToBlit.yBottom = interest_rect->y;
            rclToBlit.yTop = interest_rect->y+interest_rect->height; 
            
            _cairo_os2_surface_blit_pixels (local_os2_surface,
                                            local_os2_surface->hps_client_window,
                                            &rclToBlit);
        }

        DosReleaseMutexSem (local_os2_surface->hmtx_use_private_fields);
    }
    
    DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT);

    if (local_os2_surface->pixel_array_lend_count > 0)
        local_os2_surface->pixel_array_lend_count--;
    DosPostEventSem (local_os2_surface->hev_pixel_array_came_back);

    DosReleaseMutexSem (local_os2_surface->hmtx_use_private_fields);
}

static cairo_bool_t
_cairo_os2_surface_get_extents (void                    *abstract_surface,
                                cairo_rectangle_int_t   *rectangle)
{
    cairo_os2_surface_t *local_os2_surface;

    local_os2_surface = (cairo_os2_surface_t *) abstract_surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    }

    rectangle->x = 0;
    rectangle->y = 0;
    rectangle->width  = local_os2_surface->bitmap_info.cx;
    rectangle->height = local_os2_surface->bitmap_info.cy;

    return TRUE;
}






















cairo_surface_t *
cairo_os2_surface_create (HPS hps_client_window,
                          int width,
                          int height)
{
    cairo_os2_surface_t *local_os2_surface = 0;
    cairo_status_t status;
    int rc;

    
    if ((width <= 0) || (height <= 0)) {
        status = _cairo_error (CAIRO_STATUS_INVALID_SIZE);
        goto error_exit;
    }

    
    local_os2_surface = (cairo_os2_surface_t *) malloc (sizeof (cairo_os2_surface_t));
    if (!local_os2_surface) {
        status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
        goto error_exit;
    }

    memset(local_os2_surface, 0, sizeof(cairo_os2_surface_t));

    
    if (DosCreateMutexSem (NULL,
                           &(local_os2_surface->hmtx_use_private_fields),
                           0,
                           FALSE))
    {
        status = _cairo_error (CAIRO_STATUS_DEVICE_ERROR);
        goto error_exit;
    }

    if (DosCreateEventSem (NULL,
                           &(local_os2_surface->hev_pixel_array_came_back),
                           0,
                           FALSE))
    {
        status = _cairo_error (CAIRO_STATUS_DEVICE_ERROR);
        goto error_exit;
    }

    local_os2_surface->pixels = (unsigned char *) _buffer_alloc (height, width, 4);
    if (!local_os2_surface->pixels) {
        status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
        goto error_exit;
    }

    
    local_os2_surface->image_surface = (cairo_image_surface_t *)
        cairo_image_surface_create_for_data (local_os2_surface->pixels,
                                             CAIRO_FORMAT_ARGB32,
                                             width,      
                                             height,     
                                             width * 4); 
    status = local_os2_surface->image_surface->base.status;
    if (status)
        goto error_exit;

    



    local_os2_surface->hps_client_window = hps_client_window;
    local_os2_surface->blit_as_changes = TRUE;

    
    local_os2_surface->bitmap_info.cbFix = sizeof (BITMAPINFOHEADER2);
    local_os2_surface->bitmap_info.cx = width;
    local_os2_surface->bitmap_info.cy = height;
    local_os2_surface->bitmap_info.cPlanes = 1;
    local_os2_surface->bitmap_info.cBitCount = 32;

    
    _cairo_surface_init (&local_os2_surface->base,
                         &cairo_os2_surface_backend,
                         NULL, 
                         _cairo_content_from_format (CAIRO_FORMAT_ARGB32));

    
    return (cairo_surface_t *)local_os2_surface;

 error_exit:

    

    if (local_os2_surface) {
        if (local_os2_surface->pixels)
            _buffer_free (local_os2_surface->pixels);
        if (local_os2_surface->hev_pixel_array_came_back)
            DosCloseEventSem (local_os2_surface->hev_pixel_array_came_back);
        if (local_os2_surface->hmtx_use_private_fields)
            DosCloseMutexSem (local_os2_surface->hmtx_use_private_fields);
        free (local_os2_surface);
    }

    return _cairo_surface_create_in_error (status);
}




















cairo_surface_t *
cairo_os2_surface_create_for_window (HWND hwnd_client_window,
                                     int width,
                                     int height)
{
    cairo_os2_surface_t *local_os2_surface;

    
    if (!hwnd_client_window) {
        return _cairo_surface_create_in_error (
                                _cairo_error (CAIRO_STATUS_NO_MEMORY));
    }

    
    local_os2_surface = (cairo_os2_surface_t *)
        cairo_os2_surface_create (0, width, height);

    
    if (!local_os2_surface->image_surface->base.status) {
        local_os2_surface->hwnd_client_window = hwnd_client_window;
        local_os2_surface->blit_as_changes = FALSE;
    }

    return (cairo_surface_t *)local_os2_surface;
}



























int
cairo_os2_surface_set_size (cairo_surface_t *surface,
                            int              new_width,
                            int              new_height,
                            int              timeout)
{
    cairo_os2_surface_t *local_os2_surface;
    unsigned char *pchNewPixels;
    int rc;
    cairo_image_surface_t *pNewImageSurface;

    local_os2_surface = (cairo_os2_surface_t *) surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    }

    if ((new_width <= 0) ||
        (new_height <= 0))
    {
        
        return _cairo_error (CAIRO_STATUS_INVALID_SIZE);
    }

    
    pchNewPixels = (unsigned char *) _buffer_alloc (new_height, new_width, 4);
    if (!pchNewPixels) {
        


        return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    
    pNewImageSurface = (cairo_image_surface_t *)
        cairo_image_surface_create_for_data (pchNewPixels,
                                             CAIRO_FORMAT_ARGB32,
                                             new_width,      
                                             new_height,     
                                             new_width * 4); 

    if (pNewImageSurface->base.status) {
        


        _buffer_free (pchNewPixels);
        return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    


    if (DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT)!=NO_ERROR) {
        


        cairo_surface_destroy ((cairo_surface_t *) pNewImageSurface);
        _buffer_free (pchNewPixels);
        return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    


    while (local_os2_surface->pixel_array_lend_count > 0) {
        ULONG ulPostCount;
        DosResetEventSem (local_os2_surface->hev_pixel_array_came_back, &ulPostCount);
        DosReleaseMutexSem (local_os2_surface->hmtx_use_private_fields);
        
        rc = DosWaitEventSem (local_os2_surface->hev_pixel_array_came_back, timeout);
        if (rc != NO_ERROR) {
            
            cairo_surface_destroy ((cairo_surface_t *) pNewImageSurface);
            _buffer_free (pchNewPixels);
            return _cairo_error (CAIRO_STATUS_NO_MEMORY);
        }
        
        if (DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT)
            != NO_ERROR)
        {
            


            cairo_surface_destroy ((cairo_surface_t *) pNewImageSurface);
            _buffer_free (pchNewPixels);
            return _cairo_error (CAIRO_STATUS_NO_MEMORY);
        }
    }

    
    cairo_surface_destroy ((cairo_surface_t *) (local_os2_surface->image_surface));
    
    _buffer_free (local_os2_surface->pixels);
    
    local_os2_surface->image_surface = pNewImageSurface;
    
    local_os2_surface->pixels = pchNewPixels;
    
    local_os2_surface->bitmap_info.cx = new_width;
    local_os2_surface->bitmap_info.cy = new_height;

    DosReleaseMutexSem (local_os2_surface->hmtx_use_private_fields);
    return CAIRO_STATUS_SUCCESS;
}


























void
cairo_os2_surface_refresh_window (cairo_surface_t *surface,
                                  HPS              hps_begin_paint,
                                  PRECTL           prcl_begin_paint_rect)
{
    cairo_os2_surface_t *local_os2_surface;
    RECTL rclTemp;
    HPS hpsTemp = 0;

    local_os2_surface = (cairo_os2_surface_t *) surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return;
    }

    
    if (!hps_begin_paint) {
        hps_begin_paint = local_os2_surface->hps_client_window;
        if (!hps_begin_paint) {
            if (local_os2_surface->hwnd_client_window) {
                hpsTemp = WinGetPS(local_os2_surface->hwnd_client_window);
                hps_begin_paint = hpsTemp;
            }
            
            if (!hps_begin_paint)
                return;
        }
    }

    if (prcl_begin_paint_rect == NULL) {
        
        rclTemp.xLeft = 0;
        rclTemp.xRight = local_os2_surface->bitmap_info.cx;
        rclTemp.yTop = local_os2_surface->bitmap_info.cy;
        rclTemp.yBottom = 0;
    } else {
        
        rclTemp.xLeft = prcl_begin_paint_rect->xLeft;
        rclTemp.xRight = prcl_begin_paint_rect->xRight;
        rclTemp.yTop = local_os2_surface->bitmap_info.cy - prcl_begin_paint_rect->yBottom;
        rclTemp.yBottom = local_os2_surface->bitmap_info.cy - prcl_begin_paint_rect->yTop ;
    }

    
    if (DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT)
        != NO_ERROR)
    {
        
        if (hpsTemp)
            WinReleasePS(hpsTemp);
        return;
    }

    if ((local_os2_surface->dirty_area_present) &&
        (local_os2_surface->rcl_dirty_area.xLeft == rclTemp.xLeft) &&
        (local_os2_surface->rcl_dirty_area.xRight == rclTemp.xRight) &&
        (local_os2_surface->rcl_dirty_area.yTop == rclTemp.yTop) &&
        (local_os2_surface->rcl_dirty_area.yBottom == rclTemp.yBottom))
    {
        


        local_os2_surface->dirty_area_present = FALSE;
        _cairo_os2_surface_get_pixels_from_screen (local_os2_surface,
                                                   hps_begin_paint,
                                                   &rclTemp);
    } else {
        



        _cairo_os2_surface_blit_pixels (local_os2_surface,
                                        hps_begin_paint,
                                        &rclTemp);
    }

    DosReleaseMutexSem (local_os2_surface->hmtx_use_private_fields);

    if (hpsTemp)
        WinReleasePS(hpsTemp);
}

static cairo_status_t
_cairo_os2_surface_finish (void *abstract_surface)
{
    cairo_os2_surface_t *local_os2_surface;

    local_os2_surface = (cairo_os2_surface_t *) abstract_surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    }

    DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT);

    
    cairo_surface_destroy ((cairo_surface_t *) (local_os2_surface->image_surface));
    
    _buffer_free (local_os2_surface->pixels);
    DosCloseMutexSem (local_os2_surface->hmtx_use_private_fields);
    DosCloseEventSem (local_os2_surface->hev_pixel_array_came_back);

    



    return CAIRO_STATUS_SUCCESS;
}























void
cairo_os2_surface_set_hwnd (cairo_surface_t *surface,
                            HWND             hwnd_client_window)
{
    cairo_os2_surface_t *local_os2_surface;

    local_os2_surface = (cairo_os2_surface_t *) surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return;
    }

    if (DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT)
        != NO_ERROR)
    {
        
        return;
    }

    local_os2_surface->hwnd_client_window = hwnd_client_window;

    DosReleaseMutexSem (local_os2_surface->hmtx_use_private_fields);
}




















void
cairo_os2_surface_set_manual_window_refresh (cairo_surface_t *surface,
                                             cairo_bool_t     manual_refresh)
{
    cairo_os2_surface_t *local_os2_surface;

    local_os2_surface = (cairo_os2_surface_t *) surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return;
    }

    local_os2_surface->blit_as_changes = !manual_refresh;
}











cairo_bool_t
cairo_os2_surface_get_manual_window_refresh (cairo_surface_t *surface)
{
    cairo_os2_surface_t *local_os2_surface;

    local_os2_surface = (cairo_os2_surface_t *) surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return FALSE;
    }

    return !(local_os2_surface->blit_as_changes);
}














cairo_status_t
cairo_os2_surface_get_hps (cairo_surface_t *surface,
                           HPS             *hps)
{
    cairo_os2_surface_t *local_os2_surface;

    local_os2_surface = (cairo_os2_surface_t *) surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    }
    if (!hps)
    {
        return _cairo_error (CAIRO_STATUS_NULL_POINTER);
    }
    *hps = local_os2_surface->hps_client_window;

    return CAIRO_STATUS_SUCCESS;
}
















cairo_status_t
cairo_os2_surface_set_hps (cairo_surface_t *surface,
                           HPS              hps)
{
    cairo_os2_surface_t *local_os2_surface;

    local_os2_surface = (cairo_os2_surface_t *) surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    }
    local_os2_surface->hps_client_window = hps;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_os2_surface_mark_dirty_rectangle (void *surface,
                                         int   x,
                                         int   y,
                                         int   width,
                                         int   height)
{
    cairo_os2_surface_t *local_os2_surface;
    RECTL rclToBlit;

    local_os2_surface = (cairo_os2_surface_t *) surface;
    if ((!local_os2_surface) ||
        (local_os2_surface->base.backend != &cairo_os2_surface_backend))
    {
        
        return _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    }

    
    if (DosRequestMutexSem (local_os2_surface->hmtx_use_private_fields, SEM_INDEFINITE_WAIT)
        != NO_ERROR)
    {
        
        return CAIRO_STATUS_NO_MEMORY;
    }

    
    if (width < 0)
        width = local_os2_surface->bitmap_info.cx;
    if (height < 0)
        height = local_os2_surface->bitmap_info.cy;

    if (local_os2_surface->hwnd_client_window) {
        








        rclToBlit.xLeft = x;
        rclToBlit.xRight = x + width; 
        rclToBlit.yTop = local_os2_surface->bitmap_info.cy - (y);
        rclToBlit.yBottom = local_os2_surface->bitmap_info.cy - (y + height); 

#if 0
        if (local_os2_surface->dirty_area_present) {
            



        }
#endif

        
        memcpy (&(local_os2_surface->rcl_dirty_area), &rclToBlit, sizeof (RECTL));
        local_os2_surface->dirty_area_present = TRUE;

        
        WinInvalidateRect (local_os2_surface->hwnd_client_window,
                           &rclToBlit,
                           FALSE);
    } else {
        








        rclToBlit.xLeft = x;
        rclToBlit.xRight = x + width; 
        rclToBlit.yBottom = y;
        rclToBlit.yTop = y + height; 
        
        _cairo_os2_surface_get_pixels_from_screen (local_os2_surface,
                                                   local_os2_surface->hps_client_window,
                                                   &rclToBlit);
    }

    DosReleaseMutexSem (local_os2_surface->hmtx_use_private_fields);

    return CAIRO_STATUS_SUCCESS;
}

static const cairo_surface_backend_t cairo_os2_surface_backend = {
    CAIRO_SURFACE_TYPE_OS2,
    NULL, 
    _cairo_os2_surface_finish,
    _cairo_os2_surface_acquire_source_image,
    _cairo_os2_surface_release_source_image,
    _cairo_os2_surface_acquire_dest_image,
    _cairo_os2_surface_release_dest_image,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _cairo_os2_surface_get_extents,
    NULL, 
    NULL, 
    NULL, 
    _cairo_os2_surface_mark_dirty_rectangle,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL  
};
