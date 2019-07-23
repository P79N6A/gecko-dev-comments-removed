







































#include <winuser.h>
#ifndef WINABLEAPI
#include <winable.h>
#endif
#include "AccessibleEventId.h"

const PRUint32 kEVENT_WIN_UNKNOWN = 0x00000000;
const PRUint32 kEVENT_LAST_ENTRY  = 0xffffffff;

static const PRUint32 gWinEventMap[] = {
  kEVENT_WIN_UNKNOWN,                                
  EVENT_OBJECT_SHOW,                                 
  EVENT_OBJECT_HIDE,                                 
  EVENT_OBJECT_REORDER,                              
  IA2_EVENT_ACTIVE_DECENDENT_CHANGED,                
  EVENT_OBJECT_FOCUS,                                
  EVENT_OBJECT_STATECHANGE,                          
  EVENT_OBJECT_LOCATIONCHANGE,                       
  EVENT_OBJECT_NAMECHANGE,                           
  EVENT_OBJECT_DESCRIPTIONCHANGE,                    
  EVENT_OBJECT_VALUECHANGE,                          
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  IA2_EVENT_ACTION_CHANGED,                          
  kEVENT_WIN_UNKNOWN,                                
  EVENT_OBJECT_SELECTION,                            
  EVENT_OBJECT_SELECTIONADD,                         
  EVENT_OBJECT_SELECTIONREMOVE,                      
  EVENT_OBJECT_SELECTIONWITHIN,                      
  EVENT_SYSTEM_ALERT,                                
  kEVENT_WIN_UNKNOWN,                                
  EVENT_SYSTEM_MENUSTART,                            
  EVENT_SYSTEM_MENUEND,                              
  EVENT_SYSTEM_MENUPOPUPSTART,                       
  EVENT_SYSTEM_MENUPOPUPEND,                         
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  EVENT_SYSTEM_DRAGDROPSTART,                        
  EVENT_SYSTEM_DRAGDROPEND,                          
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  EVENT_SYSTEM_SCROLLINGSTART,                       
  EVENT_SYSTEM_SCROLLINGEND,                         
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  IA2_EVENT_DOCUMENT_LOAD_COMPLETE,                  
  IA2_EVENT_DOCUMENT_RELOAD,                         
  IA2_EVENT_DOCUMENT_LOAD_STOPPED,                   
  IA2_EVENT_DOCUMENT_ATTRIBUTE_CHANGED,              
  IA2_EVENT_DOCUMENT_CONTENT_CHANGED,                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  IA2_EVENT_TEXT_ATTRIBUTE_CHANGED,                  
  IA2_EVENT_TEXT_CARET_MOVED,                        
  IA2_EVENT_TEXT_CHANGED,                            
  IA2_EVENT_TEXT_INSERTED,                           
  IA2_EVENT_TEXT_REMOVED,                            
  IA2_EVENT_TEXT_UPDATED,                            
  IA2_EVENT_TEXT_SELECTION_CHANGED,                  
  IA2_EVENT_VISIBLE_DATA_CHANGED,                    
  IA2_EVENT_TEXT_COLUMN_CHANGED,                     
  IA2_EVENT_SECTION_CHANGED,                         
  IA2_EVENT_TABLE_CAPTION_CHANGED,                   
  IA2_EVENT_TABLE_MODEL_CHANGED,                     
  IA2_EVENT_TABLE_SUMMARY_CHANGED,                   
  IA2_EVENT_TABLE_ROW_DESCRIPTION_CHANGED,           
  IA2_EVENT_TABLE_ROW_HEADER_CHANGED,                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  IA2_EVENT_TABLE_COLUMN_DESCRIPTION_CHANGED,        
  IA2_EVENT_TABLE_COLUMN_HEADER_CHANGED,             
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_WIN_UNKNOWN,                                
  IA2_EVENT_HYPERLINK_END_INDEX_CHANGED,             
  IA2_EVENT_HYPERLINK_NUMBER_OF_ANCHORS_CHANGED,     
  IA2_EVENT_HYPERLINK_SELECTED_LINK_CHANGED,         
  IA2_EVENT_HYPERTEXT_LINK_ACTIVATED,                
  IA2_EVENT_HYPERTEXT_LINK_SELECTED,                 
  IA2_EVENT_HYPERLINK_START_INDEX_CHANGED,           
  IA2_EVENT_HYPERTEXT_CHANGED,                       
  IA2_EVENT_HYPERTEXT_NLINKS_CHANGED,                
  IA2_EVENT_OBJECT_ATTRIBUTE_CHANGED,                
  IA2_EVENT_PAGE_CHANGED,                            
  kEVENT_WIN_UNKNOWN,                                
  kEVENT_LAST_ENTRY                                  
};

