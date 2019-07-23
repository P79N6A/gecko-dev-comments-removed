









































#include "nsMai.h"
#include "nsAutoPtr.h"
#include "nsRootAccessible.h"
#include "nsDocAccessibleWrap.h"
#include "nsAccessibleEventData.h"
#include "nsIAccessibleValue.h"

#include <atk/atk.h>
#include <glib.h>
#include <glib-object.h>

#include "nsStateMap.h"



nsDocAccessibleWrap::nsDocAccessibleWrap(nsIDOMNode *aDOMNode,
                                         nsIWeakReference *aShell): 
  nsDocAccessible(aDOMNode, aShell), mActivated(PR_FALSE)
{
}

nsDocAccessibleWrap::~nsDocAccessibleWrap()
{
}

NS_IMETHODIMP nsDocAccessibleWrap::FireToolkitEvent(PRUint32 aEvent,
                                                    nsIAccessible* aAccessible,
                                                    void* aEventData)
{
    NS_ENSURE_ARG_POINTER(aAccessible);

    
    nsDocAccessible::FireToolkitEvent(aEvent, aAccessible, aEventData);

    nsresult rv = NS_ERROR_FAILURE;

    nsAccessibleWrap *accWrap =
        NS_STATIC_CAST(nsAccessibleWrap *, aAccessible);
    MAI_LOG_DEBUG(("\n\nReceived event: aEvent=%u, obj=0x%x, data=0x%x \n",
                   aEvent, aAccessible, aEventData));
    
    
    AtkObject *atkObj = accWrap->GetAtkObject();
    if (!atkObj) {
      return NS_OK;
    }

    AtkTableChange * pAtkTableChange = nsnull;

    switch (aEvent) {
    case nsIAccessibleEvent::EVENT_FOCUS:
      {
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_FOCUS\n"));
        nsRefPtr<nsRootAccessible> rootAccWrap = accWrap->GetRootAccessible();
        if (rootAccWrap && rootAccWrap->mActivated) {
          atk_focus_tracker_notify(atkObj);
        }
        rv = NS_OK;
      } break;

    case nsIAccessibleEvent::EVENT_VALUE_CHANGE :
      {
        nsCOMPtr<nsIAccessibleValue> value(do_QueryInterface(aAccessible));
        if (value) {    
          
          
          g_object_notify( (GObject*)atkObj, "accessible-value" );
        }
      }
      break;

    case nsIAccessibleEvent::EVENT_SELECTION_CHANGED:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_SELECTION_CHANGED\n"));
        g_signal_emit_by_name(atkObj, "selection_changed");
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_TEXT_SELECTION_CHANGED:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_TEXT_SELECTION_CHANGED\n"));
        g_signal_emit_by_name(atkObj, "text_selection_changed");
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_TEXT_CARET_MOVED\n"));
        NS_ASSERTION(aEventData, "Event needs event data");
        if (!aEventData)
            break;

        MAI_LOG_DEBUG(("\n\nCaret postion: %d", *(gint *)aEventData ));
        g_signal_emit_by_name(atkObj,
                              "text_caret_moved",
                              
                              *(gint *)aEventData);
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_TABLE_MODEL_CHANGED:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_TABLE_MODEL_CHANGED\n"));
        g_signal_emit_by_name(atkObj, "model_changed");
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_TABLE_ROW_INSERT:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_TABLE_ROW_INSERT\n"));
        NS_ASSERTION(aEventData, "Event needs event data");
        if (!aEventData)
            break;

        pAtkTableChange = NS_REINTERPRET_CAST(AtkTableChange *, aEventData);

        g_signal_emit_by_name(atkObj,
                              "row_inserted",
                              
                              pAtkTableChange->index,
                              
                              pAtkTableChange->count);
        rv = NS_OK;
        break;
        
    case nsIAccessibleEvent::EVENT_TABLE_ROW_DELETE:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_TABLE_ROW_DELETE\n"));
        NS_ASSERTION(aEventData, "Event needs event data");
        if (!aEventData)
            break;

        pAtkTableChange = NS_REINTERPRET_CAST(AtkTableChange *, aEventData);

        g_signal_emit_by_name(atkObj,
                              "row_deleted",
                              
                              pAtkTableChange->index,
                              
                              pAtkTableChange->count);
        rv = NS_OK;
        break;
        
    case nsIAccessibleEvent::EVENT_TABLE_ROW_REORDER:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_TABLE_ROW_REORDER\n"));
        g_signal_emit_by_name(atkObj, "row_reordered");
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_TABLE_COLUMN_INSERT:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_TABLE_COLUMN_INSERT\n"));
        NS_ASSERTION(aEventData, "Event needs event data");
        if (!aEventData)
            break;

        pAtkTableChange = NS_REINTERPRET_CAST(AtkTableChange *, aEventData);

        g_signal_emit_by_name(atkObj,
                              "column_inserted",
                              
                              pAtkTableChange->index,
                              
                              pAtkTableChange->count);
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_TABLE_COLUMN_DELETE:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_TABLE_COLUMN_DELETE\n"));
        NS_ASSERTION(aEventData, "Event needs event data");
        if (!aEventData)
            break;

        pAtkTableChange = NS_REINTERPRET_CAST(AtkTableChange *, aEventData);

        g_signal_emit_by_name(atkObj,
                              "column_deleted",
                              
                              pAtkTableChange->index,
                              
                              pAtkTableChange->count);
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_TABLE_COLUMN_REORDER:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_TABLE_COLUMN_REORDER\n"));
        g_signal_emit_by_name(atkObj, "column_reordered");
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_SECTION_CHANGED:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_SECTION_CHANGED\n"));
        g_signal_emit_by_name(atkObj, "visible_data_changed");
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_HYPERTEXT_LINK_SELECTED:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_HYPERTEXT_LINK_SELECTED\n"));
        atk_focus_tracker_notify(atkObj);
        g_signal_emit_by_name(atkObj,
                              "link_selected",
                              
                              *(gint *)aEventData);
        rv = NS_OK;
        break;

        
    case nsIAccessibleEvent::EVENT_REORDER:
        AtkChildrenChange *pAtkChildrenChange;

        MAI_LOG_DEBUG(("\n\nReceived: EVENT_REORDER(children_change)\n"));

        pAtkChildrenChange = NS_REINTERPRET_CAST(AtkChildrenChange *,
                                                 aEventData);
        nsAccessibleWrap *childAccWrap;
        if (pAtkChildrenChange && pAtkChildrenChange->child) {
            childAccWrap = NS_STATIC_CAST(nsAccessibleWrap *,
                                          pAtkChildrenChange->child);
            g_signal_emit_by_name (atkObj,
                                   pAtkChildrenChange->add ? \
                                   "children_changed::add" : \
                                   "children_changed::remove",
                                   pAtkChildrenChange->index,
                                   childAccWrap->GetAtkObject(),
                                   NULL);
        }
        else {
            
            
            
            
            
            
            
            g_signal_emit_by_name (atkObj,
                                   "children_changed::add",
                                   -1, NULL, NULL);
        }

        rv = NS_OK;
        break;

        





    case nsIAccessibleEvent::EVENT_MENU_START:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_MENU_START\n"));
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_MENU_END:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_MENU_END\n"));
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_WINDOW_ACTIVATE:
      {
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_WINDOW_ACTIVATED\n"));
        nsDocAccessibleWrap *accDocWrap =
          NS_STATIC_CAST(nsDocAccessibleWrap *, aAccessible);
        accDocWrap->mActivated = PR_TRUE;
        guint id = g_signal_lookup ("activate", MAI_TYPE_ATK_OBJECT);
        g_signal_emit(atkObj, id, 0);
        rv = NS_OK;
      } break;

    case nsIAccessibleEvent::EVENT_WINDOW_DEACTIVATE:
      {
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_WINDOW_DEACTIVATED\n"));
        nsDocAccessibleWrap *accDocWrap =
          NS_STATIC_CAST(nsDocAccessibleWrap *, aAccessible);
        accDocWrap->mActivated = PR_FALSE;
        guint id = g_signal_lookup ("deactivate", MAI_TYPE_ATK_OBJECT);
        g_signal_emit(atkObj, id, 0);
        rv = NS_OK;
      } break;

    case nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE:
      {
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_DOCUMENT_LOAD_COMPLETE\n"));
        g_signal_emit_by_name (atkObj, "load_complete");
        rv = NS_OK;
      } break;

    case nsIAccessibleEvent::EVENT_DOCUMENT_RELOAD:
      {
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_DOCUMENT_RELOAD\n"));
        g_signal_emit_by_name (atkObj, "reload");
        rv = NS_OK;
      } break;

    case nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_STOPPED:
      {
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_DOCUMENT_LOAD_STOPPED\n"));
        g_signal_emit_by_name (atkObj, "load_stopped");
        rv = NS_OK;
      } break;

    case nsIAccessibleEvent::EVENT_DOCUMENT_ATTRIBUTES_CHANGED:
      {
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_DOCUMENT_ATTRIBUTES_CHANGED\n"));
        g_signal_emit_by_name (atkObj, "attributes_changed");
        rv = NS_OK;
      } break;

    case nsIAccessibleEvent::EVENT_MENUPOPUP_START:
        
        atk_focus_tracker_notify(atkObj);

    case nsIAccessibleEvent::EVENT_SHOW:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_SHOW\n"));
        atk_object_notify_state_change(atkObj, ATK_STATE_VISIBLE, PR_TRUE);
        atk_object_notify_state_change(atkObj, ATK_STATE_SHOWING, PR_TRUE);
        rv = NS_OK;
        break;

    case nsIAccessibleEvent::EVENT_HIDE:
    case nsIAccessibleEvent::EVENT_MENUPOPUP_END:
        MAI_LOG_DEBUG(("\n\nReceived: EVENT_HIDE\n"));
        atk_object_notify_state_change(atkObj, ATK_STATE_VISIBLE, PR_FALSE);
        atk_object_notify_state_change(atkObj, ATK_STATE_SHOWING, PR_FALSE);
        rv = NS_OK;
        break;

    default:
        
        MAI_LOG_DEBUG(("\n\nReceived an unknown event=0x%u\n", aEvent));
        break;
    }

    return rv;
}

