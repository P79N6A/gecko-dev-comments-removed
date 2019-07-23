



































#include "PtRawDrawContainer.h"
#include <stdio.h>


PtWidgetClass_t *CreateRawDrawContainerClass( void );


PtWidgetClassRef_t WRawDrawContainer = { NULL, CreateRawDrawContainerClass };
PtWidgetClassRef_t *PtRawDrawContainer = &WRawDrawContainer; 




static void raw_draw_container_dflts( PtWidget_t *widget )
{
  widget->flags |= ( Pt_OPAQUE );
}




static void raw_draw_container_draw( PtWidget_t *widget, PhTile_t *damage )
{
  RawDrawContainerWidget	*rdc = ( RawDrawContainerWidget * ) widget;
  rdc->draw_f( widget, damage );
}




PtWidgetClass_t *CreateRawDrawContainerClass( void )
{
	
	static PtResourceRec_t resources[] = {
		{ RDC_DRAW_FUNC, Pt_CHANGE_INVISIBLE, 0, 
			Pt_ARG_IS_POINTER( RawDrawContainerWidget, draw_f ), 0 }
		};

	
	static PtArg_t args[] = {
		{ Pt_SET_VERSION, 110},
		{ Pt_SET_STATE_LEN, sizeof( RawDrawContainerWidget ) },
		{ Pt_SET_DFLTS_F, (long)raw_draw_container_dflts },
		{ Pt_SET_DRAW_F, (long)raw_draw_container_draw },
		{ Pt_SET_FLAGS, Pt_RECTANGULAR | Pt_OPAQUE, Pt_RECTANGULAR | Pt_OPAQUE },
		{ Pt_SET_NUM_RESOURCES, sizeof( resources ) / sizeof( resources[0] ) },
		{ Pt_SET_RESOURCES, (long)resources, sizeof( resources ) / sizeof( resources[0] ) },
		};

	
	return( PtRawDrawContainer->wclass = PtCreateWidgetClass(
		PtContainer, 0, sizeof( args )/sizeof( args[0] ), args ) );
}
