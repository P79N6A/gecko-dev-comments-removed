




#include "mozilla/MathAlgorithms.h"
#include "mozilla/TextEvents.h"
#include "mozilla/Util.h"

#include "nsNativeKeyBindings.h"
#include "nsString.h"
#include "nsMemory.h"
#include "nsGtkKeyUtils.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>


#ifdef KeyPress
#undef KeyPress
#endif

using namespace mozilla;
using namespace mozilla::widget;

static nsINativeKeyBindings::DoCommandCallback gCurrentCallback;
static void *gCurrentCallbackData;
static bool gHandled;


static void
copy_clipboard_cb(GtkWidget *w, gpointer user_data)
{
  gCurrentCallback("cmd_copy", gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "copy_clipboard");
  gHandled = true;
}

static void
cut_clipboard_cb(GtkWidget *w, gpointer user_data)
{
  gCurrentCallback("cmd_cut", gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "cut_clipboard");
  gHandled = true;
}






static const char *const sDeleteCommands[][2] = {
  
  { "cmd_deleteCharBackward", "cmd_deleteCharForward" },    
  { "cmd_deleteWordBackward", "cmd_deleteWordForward" },    
  { "cmd_deleteWordBackward", "cmd_deleteWordForward" },    
  { "cmd_deleteToBeginningOfLine", "cmd_deleteToEndOfLine" }, 
  { "cmd_deleteToBeginningOfLine", "cmd_deleteToEndOfLine" }, 
  { "cmd_deleteToBeginningOfLine", "cmd_deleteToEndOfLine" }, 
  { "cmd_deleteToBeginningOfLine", "cmd_deleteToEndOfLine" }, 
  
  
  
  { nullptr, nullptr } 
};

static void
delete_from_cursor_cb(GtkWidget *w, GtkDeleteType del_type,
                      gint count, gpointer user_data)
{
  g_signal_stop_emission_by_name(w, "delete_from_cursor");
  gHandled = true;

  bool forward = count > 0;
  if (uint32_t(del_type) >= ArrayLength(sDeleteCommands)) {
    
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

  unsigned int absCount = Abs(count);
  for (unsigned int i = 0; i < absCount; ++i) {
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
    { nullptr, nullptr },
    { nullptr, nullptr }
  }
};

static void
move_cursor_cb(GtkWidget *w, GtkMovementStep step, gint count,
               gboolean extend_selection, gpointer user_data)
{
  g_signal_stop_emission_by_name(w, "move_cursor");
  gHandled = true;
  bool forward = count > 0;
  if (uint32_t(step) >= ArrayLength(sMoveCommands)) {
    
    return;
  }

  const char *cmd = sMoveCommands[step][extend_selection][forward];
  if (!cmd)
    return; 

  
  unsigned int absCount = Abs(count);
  for (unsigned int i = 0; i < absCount; ++i) {
    gCurrentCallback(cmd, gCurrentCallbackData);
  }
}

static void
paste_clipboard_cb(GtkWidget *w, gpointer user_data)
{
  gCurrentCallback("cmd_paste", gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "paste_clipboard");
  gHandled = true;
}


static void
select_all_cb(GtkWidget *w, gboolean select, gpointer user_data)
{
  gCurrentCallback("cmd_selectAll", gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "select_all");
  gHandled = true;
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
      
      
      g_signal_connect(mNativeTarget, "select_all",
                       G_CALLBACK(select_all_cb), this);
    }
    break;
  }

  g_object_ref_sink(mNativeTarget);

  g_signal_connect(mNativeTarget, "copy_clipboard",
                   G_CALLBACK(copy_clipboard_cb), this);
  g_signal_connect(mNativeTarget, "cut_clipboard",
                   G_CALLBACK(cut_clipboard_cb), this);
  g_signal_connect(mNativeTarget, "delete_from_cursor",
                   G_CALLBACK(delete_from_cursor_cb), this);
  g_signal_connect(mNativeTarget, "move_cursor",
                   G_CALLBACK(move_cursor_cb), this);
  g_signal_connect(mNativeTarget, "paste_clipboard",
                   G_CALLBACK(paste_clipboard_cb), this);
}

nsNativeKeyBindings::~nsNativeKeyBindings()
{
  gtk_widget_destroy(mNativeTarget);
  g_object_unref(mNativeTarget);
}

NS_IMPL_ISUPPORTS1(nsNativeKeyBindings, nsINativeKeyBindings)

bool
nsNativeKeyBindings::KeyDown(const WidgetKeyboardEvent& aEvent,
                             DoCommandCallback aCallback, void *aCallbackData)
{
  return false;
}

bool
nsNativeKeyBindings::KeyPress(const WidgetKeyboardEvent& aEvent,
                              DoCommandCallback aCallback, void *aCallbackData)
{
  
  
  
  if (!aEvent.mNativeKeyEvent) {
    
    return false;
  }

  guint keyval;

  if (aEvent.charCode) {
    keyval = gdk_unicode_to_keyval(aEvent.charCode);
  } else {
    keyval =
      static_cast<GdkEventKey*>(aEvent.mNativeKeyEvent)->keyval;
  }

  if (KeyPressInternal(aEvent, aCallback, aCallbackData, keyval)) {
    return true;
  }

  for (uint32_t i = 0; i < aEvent.alternativeCharCodes.Length(); ++i) {
    uint32_t ch = aEvent.IsShift() ?
      aEvent.alternativeCharCodes[i].mShiftedCharCode :
      aEvent.alternativeCharCodes[i].mUnshiftedCharCode;
    if (ch && ch != aEvent.charCode) {
      keyval = gdk_unicode_to_keyval(ch);
      if (KeyPressInternal(aEvent, aCallback, aCallbackData, keyval)) {
        return true;
      }
    }
  }















  return false;
}

bool
nsNativeKeyBindings::KeyPressInternal(const WidgetKeyboardEvent& aEvent,
                                      DoCommandCallback aCallback,
                                      void *aCallbackData,
                                      guint aKeyval)
{
  guint modifiers =
    static_cast<GdkEventKey*>(aEvent.mNativeKeyEvent)->state;

  gCurrentCallback = aCallback;
  gCurrentCallbackData = aCallbackData;

  gHandled = false;
#if (MOZ_WIDGET_GTK == 2)
  gtk_bindings_activate(GTK_OBJECT(mNativeTarget),
                        aKeyval, GdkModifierType(modifiers));
#else
  gtk_bindings_activate(G_OBJECT(mNativeTarget),
                        aKeyval, GdkModifierType(modifiers));
#endif

  gCurrentCallback = nullptr;
  gCurrentCallbackData = nullptr;

  return gHandled;
}

bool
nsNativeKeyBindings::KeyUp(const WidgetKeyboardEvent& aEvent,
                           DoCommandCallback aCallback, void *aCallbackData)
{
  return false;
}
