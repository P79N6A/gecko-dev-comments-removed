












































#define THEME_COLOR 204
#define THEME_FONT  210


#define TS_NORMAL    1
#define TS_HOVER     2
#define TS_ACTIVE    3
#define TS_DISABLED  4
#define TS_FOCUSED   5


#define TKP_FOCUSED   4
#define TKP_DISABLED  5


#define TP_BUTTON 1
#define TP_SEPARATOR 5


#define TB_CHECKED       5
#define TB_HOVER_CHECKED 6


#define BP_BUTTON    1
#define BP_RADIO     2
#define BP_CHECKBOX  3



#define TFP_TEXTFIELD 1
#define TFP_EDITBORDER_NOSCROLL 6
#define TFS_READONLY  6


#define TFS_EDITBORDER_NORMAL 1
#define TFS_EDITBORDER_HOVER 2
#define TFS_EDITBORDER_FOCUSED 3
#define TFS_EDITBORDER_DISABLED 4


#define TREEVIEW_BODY 1


#define SP_BUTTON          1
#define SP_THUMBHOR        2
#define SP_THUMBVERT       3
#define SP_TRACKSTARTHOR   4
#define SP_TRACKENDHOR     5
#define SP_TRACKSTARTVERT  6
#define SP_TRACKENDVERT    7
#define SP_GRIPPERHOR      8
#define SP_GRIPPERVERT     9



#define SP_BUTTON_IMPLICIT_HOVER_BASE   17


#define TKP_TRACK          1
#define TKP_TRACKVERT      2
#define TKP_THUMB          3
#define TKP_THUMBVERT      6


#define SPNP_UP            1
#define SPNP_DOWN          2


#define PP_BAR             1
#define PP_BARVERT         2
#define PP_CHUNK           3
#define PP_CHUNKVERT       4


#define TABP_TAB             4
#define TABP_TAB_SELECTED    5
#define TABP_PANELS          9
#define TABP_PANEL           10


#define TTP_STANDARD         1


#define CBP_DROPMARKER       1
#define CBP_DROPBORDER       4

#define CBP_DROPFRAME        5
#define CBP_DROPMARKER_VISTA 6


#define MENU_BARBACKGROUND 7
#define MENU_BARITEM 8
#define MENU_POPUPBACKGROUND 9
#define MENU_POPUPBORDERS 10
#define MENU_POPUPCHECK 11
#define MENU_POPUPCHECKBACKGROUND 12
#define MENU_POPUPGUTTER 13
#define MENU_POPUPITEM 14
#define MENU_POPUPSEPARATOR 15
#define MENU_POPUPSUBMENU 16
#define MENU_SYSTEMCLOSE 17
#define MENU_SYSTEMMAXIMIZE 18
#define MENU_SYSTEMMINIMIZE 19
#define MENU_SYSTEMRESTORE 20

#define MB_ACTIVE 1
#define MB_INACTIVE 2

#define MS_NORMAL    1
#define MS_SELECTED  2
#define MS_DEMOTED   3

#define MBI_NORMAL 1
#define MBI_HOT 2
#define MBI_PUSHED 3
#define MBI_DISABLED 4
#define MBI_DISABLEDHOT 5
#define MBI_DISABLEDPUSHED 6

#define MC_CHECKMARKNORMAL 1
#define MC_CHECKMARKDISABLED 2
#define MC_BULLETNORMAL 3
#define MC_BULLETDISABLED 4

#define MCB_DISABLED 1
#define MCB_NORMAL 2
#define MCB_BITMAP 3

#define MPI_NORMAL 1
#define MPI_HOT 2
#define MPI_DISABLED 3
#define MPI_DISABLEDHOT 4

#define MSM_NORMAL 1
#define MSM_DISABLED 2



#define TS_MIN 0

#define TS_TRUE 1

#define TS_DRAW 2


#define TMT_TEXTCOLOR 3803
#define TMT_SIZINGMARGINS 3601
#define TMT_CONTENTMARGINS 3602
#define TMT_CAPTIONMARGINS 3603


#define RP_BAND              3
#define RP_BACKGROUND        6


#ifdef DFCS_HOT
#undef DFCS_HOT
#endif
#define DFCS_HOT             0x00001000

#ifdef COLOR_MENUHILIGHT
#undef COLOR_MENUHILIGHT
#endif
#define COLOR_MENUHILIGHT    29

#ifdef SPI_GETFLATMENU
#undef SPI_GETFLATMENU
#endif
#define SPI_GETFLATMENU      0x1022
#ifndef SPI_GETMENUSHOWDELAY
#define SPI_GETMENUSHOWDELAY      106
#endif 
#ifndef WS_EX_LAYOUTRTL 
#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
#endif



#define DFCS_RTL             0x00010000


#define TB_SEPARATOR_HEIGHT  2

