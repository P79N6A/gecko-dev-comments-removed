

















#ifndef __SVPFR_H__
#define __SVPFR_H__

#include FT_PFR_H
#include FT_INTERNAL_SERVICE_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_PFR_METRICS  "pfr-metrics"


  typedef FT_Error
  (*FT_PFR_GetMetricsFunc)( FT_Face    face,
                            FT_UInt   *aoutline,
                            FT_UInt   *ametrics,
                            FT_Fixed  *ax_scale,
                            FT_Fixed  *ay_scale );

  typedef FT_Error
  (*FT_PFR_GetKerningFunc)( FT_Face     face,
                            FT_UInt     left,
                            FT_UInt     right,
                            FT_Vector  *avector );

  typedef FT_Error
  (*FT_PFR_GetAdvanceFunc)( FT_Face   face,
                            FT_UInt   gindex,
                            FT_Pos   *aadvance );


  FT_DEFINE_SERVICE( PfrMetrics )
  {
    FT_PFR_GetMetricsFunc  get_metrics;
    FT_PFR_GetKerningFunc  get_kerning;
    FT_PFR_GetAdvanceFunc  get_advance;

  };

 

FT_END_HEADER

#endif 



