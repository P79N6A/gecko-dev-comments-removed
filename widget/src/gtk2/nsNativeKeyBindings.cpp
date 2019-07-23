





































#include "nsNativeKeyBindings.h"
#include "nsString.h"
#include "nsMemory.h"
#include "nsGtkKeyUtils.h"

#include <gtk/gtkentry.h>
#include <gtk/gtktextview.h>
#include <gtk/gtkbindings.h>
#include <gtk/gtkmain.h>
#include <gdk/gdkkeysyms.h>

static nsINativeKeyBindings::DoCommandCallback gCurrentCallback;
static void *gCurrentCallbackData;
static PRBool gHandled;


static void
copy_clipboard_cb(GtkWidget *w, gpointer user_data)
{
  gCurrentCallback("cmd_copy", gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "copy_clipboard");
  gHandled = PR_TRUE;
}

static void
cut_clipboard_cb(GtkWidget *w, gpointer user_data)
{
  gCurrentCallback("cmd_cut", gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "cut_clipboard");
  gHandled = PR_TRUE;
}






static const char *const sDeleteCommands[][2] = {
  
  { "cmd_deleteCharBackward", "cmd_deleteCharForward" },    
  { "cmd_deleteWordBackward", "cmd_deleteWordForward" },    
  { "cmd_deleteWordBackward", "cmd_deleteWordForward" },    
  { "cmd_deleteToBeginningOfLine", "cmd_deleteToEndOfLine" }, 
  { "cmd_deleteToBeginningOfLine", "cmd_deleteToEndOfLine" }, 
  { "cmd_deleteToBeginningOfLine", "cmd_deleteToEndOfLine" }, 
  { "cmd_deleteToBeginningOfLine", "cmd_deleteToEndOfLine" }, 
  
  
  
  { nsnull, nsnull } 
};

static void
delete_from_cursor_cb(GtkWidget *w, GtkDeleteType del_type,
                      gint count, gpointer user_data)
{
  g_signal_stop_emission_by_name(w, "delete_from_cursor");
  gHandled = PR_TRUE;

  PRBool forward = count > 0;
  if (PRUint32(del_type) >= NS_ARRAY_LENGTH(sDeleteCommands)) {
    
    return;
  }

  if (del_type == GTK_DELETE_WORDS) {
    
    
    if (forward) {
      gCurrentCallback("cmd_wordNext", gCurrentCallbackData);
      gCurrentCallback("cmd_wordPrevious", gCurrentCallbackData);
    } else {
      gCurrentCallback("cmd_wordPrevious", gCurrentCallbackData);
      gCurrentCallback("cmd_wordNext", gCurrentCallbackData);
    }
  } else if (del_type == GTK_DELETE_DISPLAY_LINES ||
             del_type == GTK_DELETE_PARAGRAPHS) {

    
    
    if (forward) {
      gCurrentCallback("cmd_beginLine", gCurrentCallbackData);
    } else {
      gCurrentCallback("cmd_endLine", gCurrentCallbackData);
    }
  }

  const char *cmd = sDeleteCommands[del_type][forward];
  if (!cmd)
    return; 

  count = PR_ABS(count);
  for (int i = 0; i < count; ++i) {
    gCurrentCallback(cmd, gCurrentCallbackData);
  }
}

static const char *const sMoveCommands[][2][2] = {
  
  
  
  
  { 
    { "cmd_charPrevious", "cmd_charNext" },
    { "cmd_selectCharPrevious", "cmd_selectCharNext" }
  },
  { 
    { "cmd_charPrevious", "cmd_charNext" },
    { "cmd_selectCharPrevious", "cmd_selectCharNext" }
  },
  { 
    { "cmd_wordPrevious", "cmd_wordNext" },
    { "cmd_selectWordPrevious", "cmd_selectWordNext" }
  },
  { 
    { "cmd_linePrevious", "cmd_lineNext" },
    { "cmd_selectLinePrevious", "cmd_selectLineNext" }
  },
  { 
    { "cmd_beginLine", "cmd_endLine" },
    { "cmd_selectBeginLine", "cmd_selectEndLine" }
  },
  { 
    { "cmd_linePrevious", "cmd_lineNext" },
    { "cmd_selectLinePrevious", "cmd_selectLineNext" }
  },
  { 
    { "cmd_beginLine", "cmd_endLine" },
    { "cmd_selectBeginLine", "cmd_selectEndLine" }
  },
  { 
    { "cmd_movePageUp", "cmd_movePageDown" },
    { "cmd_selectPageUp", "cmd_selectPageDown" }
  },
  { 
    { "cmd_moveTop", "cmd_moveBottom" },
    { "cmd_selectTop", "cmd_selectBottom" }
  },
  { 
    { nsnull, nsnull },
    { nsnull, nsnull }
  }
};

static void
move_cursor_cb(GtkWidget *w, GtkMovementStep step, gint count,
               gboolean extend_selection, gpointer user_data)
{
  g_signal_stop_emission_by_name(w, "move_cursor");
  gHandled = PR_TRUE;
  PRBool forward = count > 0;
  if (PRUint32(step) >= NS_ARRAY_LENGTH(sMoveCommands)) {
    
    return;
  }

  const char *cmd = sMoveCommands[step][extend_selection][forward];
  if (!cmd)
    return; 

  
  count = PR_ABS(count);
  for (int i = 0; i < count; ++i) {
    gCurrentCallback(cmd, gCurrentCallbackData);
  }
}

static void
paste_clipboard_cb(GtkWidget *w, gpointer user_data)
{
  gCurrentCallback("cmd_paste", gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "paste_clipboard");
  gHandled = PR_TRUE;
}


static void
select_all_cb(GtkWidget *w, gboolean select, gpointer user_data)
{
  gCurrentCallback("cmd_selectAll", gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "select_all");
  gHandled = PR_TRUE;
}

void
nsNativeKeyBindings::Init(NativeKeyBindingsType  aType)
{
  switch (aType) {
  case eKeyBindings_Input:
    mNativeTarget = gtk_entry_new();
    break;
  case eKeyBindings_TextArea:
    mNativeTarget = gtk_text_view_new();
    if (gtk_major_version > 2 ||
        (gtk_major_version == 2 && (gtk_minor_version > 2 ||
                                    (gtk_minor_version == 2 &&
                                     gtk_micro_version >= 2)))) {
      
      
      g_signal_connect(G_OBJECT(mNativeTarget), "select_all",
                       G_CALLBACK(select_all_cb), this);
    }
    break;
  }

  g_signal_connect(G_OBJECT(mNativeTarget), "copy_clipboard",
                   G_CALLBACK(copy_clipboard_cb), this);
  g_signal_connect(G_OBJECT(mNativeTarget), "cut_clipboard",
                   G_CALLBACK(cut_clipboard_cb), this);
  g_signal_connect(G_OBJECT(mNativeTarget), "delete_from_cursor",
                   G_CALLBACK(delete_from_cursor_cb), this);
  g_signal_connect(G_OBJECT(mNativeTarget), "move_cursor",
                   G_CALLBACK(move_cursor_cb), this);
  g_signal_connect(G_OBJECT(mNativeTarget), "paste_clipboard",
                   G_CALLBACK(paste_clipboard_cb), this);
}

nsNativeKeyBindings::~nsNativeKeyBindings()
{
  gtk_widget_destroy(mNativeTarget);
}

NS_IMPL_ISUPPORTS1(nsNativeKeyBindings, nsINativeKeyBindings)

PRBool
nsNativeKeyBindings::KeyDown(const nsNativeKeyEvent& aEvent,
                             DoCommandCallback aCallback, void *aCallbackData)
{
  return PR_FALSE;
}

PRBool
nsNativeKeyBindings::KeyPress(const nsNativeKeyEvent& aEvent,
                              DoCommandCallback aCallback, void *aCallbackData)
{
  PRUint32 keyCode;

  if (aEvent.charCode != 0)
    keyCode = gdk_unicode_to_keyval(aEvent.charCode);
  else
    keyCode = DOMKeyCodeToGdkKeyCode(aEvent.keyCode);

  int modifiers = 0;
  if (aEvent.altKey)
    modifiers |= GDK_MOD1_MASK;
  if (aEvent.ctrlKey)
    modifiers |= GDK_CONTROL_MASK;
  if (aEvent.shiftKey)
    modifiers |= GDK_SHIFT_MASK;
  

  gCurrentCallback = aCallback;
  gCurrentCallbackData = aCallbackData;

  gHandled = PR_FALSE;

  gtk_bindings_activate(GTK_OBJECT(mNativeTarget),
                        keyCode, GdkModifierType(modifiers));

  gCurrentCallback = nsnull;
  gCurrentCallbackData = nsnull;

  return gHandled;
}

PRBool
nsNativeKeyBindings::KeyUp(const nsNativeKeyEvent& aEvent,
                           DoCommandCallback aCallback, void *aCallbackData)
{
  return PR_FALSE;
}
