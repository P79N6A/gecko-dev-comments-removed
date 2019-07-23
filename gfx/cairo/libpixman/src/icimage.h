
























#ifndef _ICIMAGE_H_
#define _ICIMAGE_H_





























#define PICT_GRADIENT_STOPTABLE_SIZE 1024

#define SourcePictTypeSolidFill 0
#define SourcePictTypeLinear    1
#define SourcePictTypeRadial    2
#define SourcePictTypeConical   3

#define SourcePictClassUnknown    0
#define SourcePictClassHorizontal 1
#define SourcePictClassVertical   2

typedef struct _pixman_solid_fill_image {
    unsigned int type;
    unsigned int class;
    uint32_t	 color;
} pixman_solid_fill_image_t;

typedef struct _pixman_gradient_image {
    unsigned int	   type;
    unsigned int	   class;
    pixman_gradient_stop_t *stops;
    int			   nstops;
    int			   stopRange;
    uint32_t		   *colorTable;
    int			   colorTableSize;
} pixman_gradient_image_t;

typedef struct _pixman_linear_gradient_image {
    unsigned int	   type;
    unsigned int	   class;
    pixman_gradient_stop_t *stops;
    int			   nstops;
    int			   stopRange;
    uint32_t		   *colorTable;
    int			   colorTableSize;
    pixman_point_fixed_t   p1;
    pixman_point_fixed_t   p2;
} pixman_linear_gradient_image_t;

typedef struct _pixman_radial_gradient_image {
    unsigned int	   type;
    unsigned int	   class;
    pixman_gradient_stop_t *stops;
    int			   nstops;
    int			   stopRange;
    uint32_t		   *colorTable;
    int			   colorTableSize;
    double		   fx;
    double		   fy;
    double		   dx;
    double		   dy;
    double		   a;
    double		   m;
    double		   b;
} pixman_radial_gradient_image_t;

typedef struct _pixman_conical_gradient_image {
    unsigned int	   type;
    unsigned int	   class;
    pixman_gradient_stop_t *stops;
    int			   nstops;
    int			   stopRange;
    uint32_t		   *colorTable;
    int			   colorTableSize;
    pixman_point_fixed_t   center;
    pixman_fixed16_16_t	   angle;
} pixman_conical_gradient_image_t;

typedef union _pixman_source_image {
    unsigned int		    type;
    pixman_solid_fill_image_t	    solidFill;
    pixman_gradient_image_t	    gradient;
    pixman_linear_gradient_image_t  linear;
    pixman_radial_gradient_image_t  radial;
    pixman_conical_gradient_image_t conical;
} pixman_source_image_t;

typedef pixman_source_image_t *SourcePictPtr;

struct pixman_image {
    FbPixels	    *pixels;
    pixman_format_t	    image_format;
    int		    format_code;
    int		    refcnt;

    unsigned int    repeat : 2;
    unsigned int    graphicsExposures : 1;
    unsigned int    subWindowMode : 1;
    unsigned int    polyEdge : 1;
    unsigned int    polyMode : 1;
    unsigned int    freeCompClip : 1;
    unsigned int    freeSourceClip : 1;
    unsigned int    clientClipType : 2;
    unsigned int    componentAlpha : 1;
    unsigned int    compositeClipSource : 1;
    unsigned int    unused : 20;

    struct pixman_image *alphaMap;
    FbPoint	    alphaOrigin;

    FbPoint 	    clipOrigin;
    void	   *clientClip;

    unsigned long   dither;

    unsigned long   stateChanges;
    unsigned long   serialNumber;

    pixman_region16_t	    *pCompositeClip;
    pixman_region16_t	    *pSourceClip;

    pixman_transform_t     *transform;

    pixman_filter_t	    filter;
    pixman_fixed16_16_t    *filter_params;
    int		    filter_nparams;

    int		    owns_pixels;

    pixman_source_image_t *pSourcePict;
};

#endif 

#ifndef _IC_MIPICT_H_
#define _IC_MIPICT_H_

#define IC_MAX_INDEXED	256 /* XXX depth must be <= 8 */

#if IC_MAX_INDEXED <= 256
typedef uint8_t FbIndexType;
#endif


typedef struct _FbIndexed {
    int	color;
    uint32_t	rgba[IC_MAX_INDEXED];
    FbIndexType	ent[32768];
} FbIndexedRec, *FbIndexedPtr;

#define FbCvtR8G8B8to15(s) ((((s) >> 3) & 0x001f) | \
			     (((s) >> 6) & 0x03e0) | \
			     (((s) >> 9) & 0x7c00))
#define FbIndexToEnt15(icf,rgb15) ((icf)->ent[rgb15])
#define FbIndexToEnt24(icf,rgb24) FbIndexToEnt15(icf,FbCvtR8G8B8to15(rgb24))

#define FbIndexToEntY24(icf,rgb24) ((icf)->ent[CvtR8G8B8toY15(rgb24)])






pixman_private void
pixman_image_init (pixman_image_t *image);

pixman_private void
pixman_image_destroyClip (pixman_image_t *image);

















pixman_private int
FbComputeCompositeRegion (pixman_region16_t	*region,
			  pixman_image_t	*iSrc,
			  pixman_image_t	*iMask,
			  pixman_image_t	*iDst,
			  int16_t		xSrc,
			  int16_t		ySrc,
			  int16_t		xMask,
			  int16_t		yMask,
			  int16_t		xDst,
			  int16_t		yDst,
			  uint16_t	width,
			  uint16_t	height);

pixman_private int
miIsSolidAlpha (pixman_image_t *src);




























pixman_private pixman_image_t *
FbCreateAlphaPicture (pixman_image_t	*dst,
		      pixman_format_t	*format,
		      uint16_t	width,
		      uint16_t	height);

typedef void	(*CompositeFunc) (pixman_operator_t   op,
				  pixman_image_t    *iSrc,
				  pixman_image_t    *iMask,
				  pixman_image_t    *iDst,
				  int16_t      xSrc,
				  int16_t      ySrc,
				  int16_t      xMask,
				  int16_t      yMask,
				  int16_t      xDst,
				  int16_t      yDst,
				  uint16_t     width,
				  uint16_t     height);

typedef struct _FbCompositeOperand FbCompositeOperand;

typedef uint32_t (*pixman_compositeFetch)(FbCompositeOperand *op);
typedef void (*pixman_compositeStore) (FbCompositeOperand *op, uint32_t value);

typedef void (*pixman_compositeStep) (FbCompositeOperand *op);
typedef void (*pixman_compositeSet) (FbCompositeOperand *op, int x, int y);

struct _FbCompositeOperand {
    union {
	struct {
	    pixman_bits_t		*top_line;
	    int			left_offset;

	    int			start_offset;
	    pixman_bits_t		*line;
	    uint32_t		offset;
	    FbStride		stride;
	    int			bpp;
	} drawable;
	struct {
	    int			alpha_dx;
	    int			alpha_dy;
	} external;
	struct {
	    int			top_y;
	    int			left_x;
	    int			start_x;
	    int			x;
	    int			y;
	    pixman_transform_t		*transform;
	    pixman_filter_t		filter;
	    int                         repeat;
	    int                         width;
	    int                         height;
	} transform;
    } u;
    pixman_compositeFetch	fetch;
    pixman_compositeFetch	fetcha;
    pixman_compositeStore	store;
    pixman_compositeStep	over;
    pixman_compositeStep	down;
    pixman_compositeSet	set;



    pixman_region16_t		*dst_clip;
    pixman_region16_t		*src_clip;
};

typedef void (*FbCombineFunc) (FbCompositeOperand	*src,
			       FbCompositeOperand	*msk,
			       FbCompositeOperand	*dst);

typedef struct _FbAccessMap {
    uint32_t		format_code;
    pixman_compositeFetch	fetch;
    pixman_compositeFetch	fetcha;
    pixman_compositeStore	store;
} FbAccessMap;



typedef struct _FbCompSrc {
    uint32_t	value;
    uint32_t	alpha;
} FbCompSrc;

pixman_private int
fbBuildCompositeOperand (pixman_image_t	    *image,
			 FbCompositeOperand op[4],
			 int16_t		    x,
			 int16_t		    y,
			 int		    transform,
			 int		    alpha);

pixman_private void
pixman_compositeGeneral (pixman_operator_t	op,
			 pixman_image_t	*iSrc,
			 pixman_image_t	*iMask,
			 pixman_image_t	*iDst,
			 int16_t	xSrc,
			 int16_t	ySrc,
			 int16_t	xMask,
			 int16_t	yMask,
			 int16_t	xDst,
			 int16_t	yDst,
			 uint16_t	width,
			 uint16_t	height);

#endif 
