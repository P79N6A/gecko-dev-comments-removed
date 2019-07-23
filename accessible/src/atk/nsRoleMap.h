







































#include <atk/atk.h>
#include "nsAccessibleWrap.h"

const PRUint32 kROLE_ATK_LAST_ENTRY = 0xffffffff;


static const PRUint32 atkRoleMap[] = {
                                  
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_MENU_BAR,            
    ATK_ROLE_SCROLL_BAR,          
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_ALERT,               
    ATK_ROLE_WINDOW,              
    ATK_ROLE_INTERNAL_FRAME,      
    ATK_ROLE_MENU,                
    ATK_ROLE_MENU_ITEM,           
    ATK_ROLE_TOOL_TIP,            
    ATK_ROLE_EMBEDDED,            
    ATK_ROLE_DOCUMENT_FRAME,      
    ATK_ROLE_PANEL,               
    ATK_ROLE_CHART,               
    ATK_ROLE_DIALOG,              
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_PANEL,               
    ATK_ROLE_SEPARATOR,           
    ATK_ROLE_TOOL_BAR,            
    ATK_ROLE_STATUSBAR,           
    ATK_ROLE_TABLE,               
    ATK_ROLE_COLUMN_HEADER,       
    ATK_ROLE_ROW_HEADER,          
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_LIST_ITEM,           
    ATK_ROLE_TABLE_CELL,          
    ATK_ROLE_LINK,                
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_IMAGE,               
    ATK_ROLE_LIST,                
    ATK_ROLE_LIST_ITEM,           
    ATK_ROLE_TREE,                
    ATK_ROLE_LIST_ITEM,           
    ATK_ROLE_PAGE_TAB,            
    ATK_ROLE_SCROLL_PANE,         
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_IMAGE,               
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_PUSH_BUTTON,         
    ATK_ROLE_CHECK_BOX,           
    ATK_ROLE_RADIO_BUTTON,        
    ATK_ROLE_COMBO_BOX,           
    ATK_ROLE_COMBO_BOX,           
    ATK_ROLE_PROGRESS_BAR,        
    ATK_ROLE_DIAL,                
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_SLIDER,              
    ATK_ROLE_SPIN_BUTTON,         
    ATK_ROLE_IMAGE,               
    ATK_ROLE_ANIMATION,           
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_PUSH_BUTTON,         
    ATK_ROLE_PUSH_BUTTON,         
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_PAGE_TAB_LIST,       
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_PUSH_BUTTON,         
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_ACCEL_LABEL,         
    ATK_ROLE_ARROW,               
    ATK_ROLE_CANVAS,              
    ATK_ROLE_CHECK_MENU_ITEM,     
    ATK_ROLE_COLOR_CHOOSER,       
    ATK_ROLE_DATE_EDITOR,         
    ATK_ROLE_DESKTOP_ICON,        
    ATK_ROLE_DESKTOP_FRAME,       
    ATK_ROLE_DIRECTORY_PANE,      
    ATK_ROLE_FILE_CHOOSER,        
    ATK_ROLE_FONT_CHOOSER,        
    ATK_ROLE_FRAME,               
    ATK_ROLE_GLASS_PANE,          
    ATK_ROLE_HTML_CONTAINER,      
    ATK_ROLE_ICON,                
    ATK_ROLE_LABEL,               
    ATK_ROLE_LAYERED_PANE,        
    ATK_ROLE_OPTION_PANE,         
    ATK_ROLE_PASSWORD_TEXT,       
    ATK_ROLE_POPUP_MENU,          
    ATK_ROLE_RADIO_MENU_ITEM,     
    ATK_ROLE_ROOT_PANE,           
    ATK_ROLE_SCROLL_PANE,         
    ATK_ROLE_SPLIT_PANE,          
    ATK_ROLE_TABLE_COLUMN_HEADER, 
    ATK_ROLE_TABLE_ROW_HEADER,    
    ATK_ROLE_TEAR_OFF_MENU_ITEM,  
    ATK_ROLE_TERMINAL,            
    ATK_ROLE_TEXT,                
    ATK_ROLE_TOGGLE_BUTTON,       
    ATK_ROLE_TREE_TABLE,          
    ATK_ROLE_VIEWPORT,            
    ATK_ROLE_HEADER,              
    ATK_ROLE_FOOTER,              
    ATK_ROLE_PARAGRAPH,           
    ATK_ROLE_RULER,               
    ATK_ROLE_AUTOCOMPLETE,        
    ATK_ROLE_EDITBAR,             
    ATK_ROLE_ENTRY,               
    ATK_ROLE_CAPTION,             
    ATK_ROLE_DOCUMENT_FRAME,      
    ATK_ROLE_HEADING,             
    ATK_ROLE_PAGE,                
    ATK_ROLE_SECTION,             
    ATK_ROLE_REDUNDANT_OBJECT,    
    ATK_ROLE_FORM,                
    ATK_ROLE_INPUT_METHOD_WINDOW, 
    ATK_ROLE_APPLICATION,         
    ATK_ROLE_MENU,                
    ATK_ROLE_CALENDAR,            
    ATK_ROLE_MENU,                
    ATK_ROLE_MENU_ITEM,           
    ATK_ROLE_IMAGE,               
    ATK_ROLE_LIST_ITEM,           
    ATK_ROLE_LIST_ITEM,           
    ATK_ROLE_LIST,                
    ATK_ROLE_UNKNOWN,             
    ATK_ROLE_TABLE_CELL,          
    ATK_ROLE_PANEL,               
    kROLE_ATK_LAST_ENTRY          
};

