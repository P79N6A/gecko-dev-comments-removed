




#include "mozilla/ArrayUtils.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/TextEvents.h"

#include "NativeKeyBindings.h"
#include "nsString.h"
#include "nsMemory.h"
#include "nsGtkKeyUtils.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>

namespace mozilla {
namespace widget {

static nsIWidget::DoCommandCallback gCurrentCallback;
static void *gCurrentCallbackData;
static bool gHandled;


static void
copy_clipboard_cb(GtkWidget *w, gpointer user_data)
{
  gCurrentCallback(CommandCopy, gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "copy_clipboard");
  gHandled = true;
}

static void
cut_clipboard_cb(GtkWidget *w, gpointer user_data)
{
  gCurrentCallback(CommandCut, gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "cut_clipboard");
  gHandled = true;
}






static const Command sDeleteCommands[][2] = {
  
  { CommandDeleteCharBackward, CommandDeleteCharForward },    
  { CommandDeleteWordBackward, CommandDeleteWordForward },    
  { CommandDeleteWordBackward, CommandDeleteWordForward },    
  { CommandDeleteToBeginningOfLine, CommandDeleteToEndOfLine }, 
  { CommandDeleteToBeginningOfLine, CommandDeleteToEndOfLine }, 
  { CommandDeleteToBeginningOfLine, CommandDeleteToEndOfLine }, 
  { CommandDeleteToBeginningOfLine, CommandDeleteToEndOfLine }, 
  
  
  
  { CommandDoNothing, CommandDoNothing } 
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
      gCurrentCallback(CommandWordNext, gCurrentCallbackData);
      gCurrentCallback(CommandWordPrevious, gCurrentCallbackData);
    } else {
      gCurrentCallback(CommandWordPrevious, gCurrentCallbackData);
      gCurrentCallback(CommandWordNext, gCurrentCallbackData);
    }
  } else if (del_type == GTK_DELETE_DISPLAY_LINES ||
             del_type == GTK_DELETE_PARAGRAPHS) {

    
    
    if (forward) {
      gCurrentCallback(CommandBeginLine, gCurrentCallbackData);
    } else {
      gCurrentCallback(CommandEndLine, gCurrentCallbackData);
    }
  }

  Command command = sDeleteCommands[del_type][forward];
  if (!command) {
    return; 
  }

  unsigned int absCount = Abs(count);
  for (unsigned int i = 0; i < absCount; ++i) {
    gCurrentCallback(command, gCurrentCallbackData);
  }
}

static const Command sMoveCommands[][2][2] = {
  
  
  
  
  { 
    { CommandCharPrevious, CommandCharNext },
    { CommandSelectCharPrevious, CommandSelectCharNext }
  },
  { 
    { CommandCharPrevious, CommandCharNext },
    { CommandSelectCharPrevious, CommandSelectCharNext }
  },
  { 
    { CommandWordPrevious, CommandWordNext },
    { CommandSelectWordPrevious, CommandSelectWordNext }
  },
  { 
    { CommandLinePrevious, CommandLineNext },
    { CommandSelectLinePrevious, CommandSelectLineNext }
  },
  { 
    { CommandBeginLine, CommandEndLine },
    { CommandSelectBeginLine, CommandSelectEndLine }
  },
  { 
    { CommandLinePrevious, CommandLineNext },
    { CommandSelectLinePrevious, CommandSelectLineNext }
  },
  { 
    { CommandBeginLine, CommandEndLine },
    { CommandSelectBeginLine, CommandSelectEndLine }
  },
  { 
    { CommandMovePageUp, CommandMovePageDown },
    { CommandSelectPageUp, CommandSelectPageDown }
  },
  { 
    { CommandMoveTop, CommandMoveBottom },
    { CommandSelectTop, CommandSelectBottom }
  },
  { 
    { CommandDoNothing, CommandDoNothing },
    { CommandDoNothing, CommandDoNothing }
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

  Command command = sMoveCommands[step][extend_selection][forward];
  if (!command) {
    return; 
  }

  unsigned int absCount = Abs(count);
  for (unsigned int i = 0; i < absCount; ++i) {
    gCurrentCallback(command, gCurrentCallbackData);
  }
}

static void
paste_clipboard_cb(GtkWidget *w, gpointer user_data)
{
  gCurrentCallback(CommandPaste, gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "paste_clipboard");
  gHandled = true;
}


static void
select_all_cb(GtkWidget *w, gboolean select, gpointer user_data)
{
  gCurrentCallback(CommandSelectAll, gCurrentCallbackData);
  g_signal_stop_emission_by_name(w, "select_all");
  gHandled = true;
}

NativeKeyBindings* NativeKeyBindings::sInstanceForSingleLineEditor = nullptr;
NativeKeyBindings* NativeKeyBindings::sInstanceForMultiLineEditor = nullptr;


NativeKeyBindings*
NativeKeyBindings::GetInstance(NativeKeyBindingsType aType)
{
  switch (aType) {
    case nsIWidget::NativeKeyBindingsForSingleLineEditor:
      if (!sInstanceForSingleLineEditor) {
        sInstanceForSingleLineEditor = new NativeKeyBindings();
        sInstanceForSingleLineEditor->Init(aType);
      }
      return sInstanceForSingleLineEditor;

    default:
      
      MOZ_ASSERT(false, "aType is invalid or not yet implemented");
    case nsIWidget::NativeKeyBindingsForMultiLineEditor:
    case nsIWidget::NativeKeyBindingsForRichTextEditor:
      if (!sInstanceForMultiLineEditor) {
        sInstanceForMultiLineEditor = new NativeKeyBindings();
        sInstanceForMultiLineEditor->Init(aType);
      }
      return sInstanceForMultiLineEditor;
  }
}


void
NativeKeyBindings::Shutdown()
{
  delete sInstanceForSingleLineEditor;
  sInstanceForSingleLineEditor = nullptr;
  delete sInstanceForMultiLineEditor;
  sInstanceForMultiLineEditor = nullptr;
}

void
NativeKeyBindings::Init(NativeKeyBindingsType  aType)
{
  switch (aType) {
  case nsIWidget::NativeKeyBindingsForSingleLineEditor:
    mNativeTarget = gtk_entry_new();
    break;
  default:
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

NativeKeyBindings::~NativeKeyBindings()
{
  gtk_widget_destroy(mNativeTarget);
  g_object_unref(mNativeTarget);
}

bool
NativeKeyBindings::Execute(const WidgetKeyboardEvent& aEvent,
                           DoCommandCallback aCallback,
                           void* aCallbackData)
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

  if (ExecuteInternal(aEvent, aCallback, aCallbackData, keyval)) {
    return true;
  }

  for (uint32_t i = 0; i < aEvent.alternativeCharCodes.Length(); ++i) {
    uint32_t ch = aEvent.IsShift() ?
      aEvent.alternativeCharCodes[i].mShiftedCharCode :
      aEvent.alternativeCharCodes[i].mUnshiftedCharCode;
    if (ch && ch != aEvent.charCode) {
      keyval = gdk_unicode_to_keyval(ch);
      if (ExecuteInternal(aEvent, aCallback, aCallbackData, keyval)) {
        return true;
      }
    }
  }















  return false;
}

bool
NativeKeyBindings::ExecuteInternal(const WidgetKeyboardEvent& aEvent,
                                   DoCommandCallback aCallback,
                                   void* aCallbackData,
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

} 
} 
