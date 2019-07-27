@
@
@
@
@
@
@
@
@
@
@
@

@
@
@
@
@
@
@
@
@

@
	.equ	OMX_Sts_NoErr, 0    @
	.equ	OMX_Sts_Err, -2    @
	.equ	OMX_Sts_InvalidBitstreamValErr, -182  @
	.equ	OMX_Sts_MemAllocErr, -9    @
	.equ	OMX_StsACAAC_GainCtrErr, -159  @
	.equ	OMX_StsACAAC_PrgNumErr, -167  @
	.equ	OMX_StsACAAC_CoefValErr, -163  @
	.equ	OMX_StsACAAC_MaxSfbErr, -162  @
	.equ	OMX_StsACAAC_PlsDataErr, -160  @

@
	.equ	OMX_Sts_BadArgErr, -5    @

	.equ	OMX_StsACAAC_TnsNumFiltErr, -157  @
	.equ	OMX_StsACAAC_TnsLenErr, -156  @
	.equ	OMX_StsACAAC_TnsOrderErr, -155  @
	.equ	OMX_StsACAAC_TnsCoefResErr, -154  @
	.equ	OMX_StsACAAC_TnsCoefErr, -153  @
	.equ	OMX_StsACAAC_TnsDirectErr, -152  @
	.equ	OMX_StsICJP_JPEGMarkerErr, -183  @
                                            @
	.equ	OMX_StsICJP_JPEGMarker, -181  @
                                            @
	.equ	OMX_StsIPPP_ContextMatchErr, -17   @

	.equ	OMX_StsSP_EvenMedianMaskSizeErr, -180  @

	.equ	OMX_Sts_MaximumEnumeration, 0x7FFFFFFF



	.equ	OMX_MIN_S8, (-128)
	.equ	OMX_MIN_U8, 0
	.equ	OMX_MIN_S16, (-32768)
	.equ	OMX_MIN_U16, 0


	.equ	OMX_MIN_S32, (-2147483647-1)
	.equ	OMX_MIN_U32, 0

	.equ	OMX_MAX_S8, (127)
	.equ	OMX_MAX_U8, (255)
	.equ	OMX_MAX_S16, (32767)
	.equ	OMX_MAX_U16, (0xFFFF)
	.equ	OMX_MAX_S32, (2147483647)
	.equ	OMX_MAX_U32, (0xFFFFFFFF)

	.equ	OMX_VC_UPPER, 0x1                 @
	.equ	OMX_VC_LEFT, 0x2                 @
	.equ	OMX_VC_UPPER_RIGHT, 0x40          @

	.equ	NULL, 0
