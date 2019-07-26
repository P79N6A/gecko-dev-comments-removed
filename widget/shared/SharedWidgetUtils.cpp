





#include "WidgetUtils.h"

#include "mozilla/TextEvents.h"

#include "nsIBaseWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShell.h"
#include "nsIInterfaceRequestorUtils.h"

namespace mozilla {
namespace widget {


already_AddRefed<nsIWidget>
WidgetUtils::DOMWindowToWidget(nsIDOMWindow *aDOMWindow)
{
  nsCOMPtr<nsIWidget> widget;

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aDOMWindow);
  if (window) {
    nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(window->GetDocShell()));

    while (!widget && baseWin) {
      baseWin->GetParentWidget(getter_AddRefs(widget));
      if (!widget) {
        nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(baseWin));
        if (!docShellAsItem)
          return nullptr;

        nsCOMPtr<nsIDocShellTreeItem> parent;
        docShellAsItem->GetParent(getter_AddRefs(parent));

        window = do_GetInterface(parent);
        if (!window)
          return nullptr;

        baseWin = do_QueryInterface(window->GetDocShell());
      }
    }
  }

  return widget.forget();
}


uint32_t
WidgetUtils::ComputeKeyCodeFromChar(uint32_t aCharCode)
{
  if (aCharCode >= 'A' && aCharCode <= 'Z') {
    return aCharCode - 'A' + NS_VK_A;
  }
  if (aCharCode >= 'a' && aCharCode <= 'z') {
    return aCharCode - 'a' + NS_VK_A;
  }
  if (aCharCode >= '0' && aCharCode <= '9') {
    return aCharCode - '0' + NS_VK_0;
  }
  switch (aCharCode) {
    case ' ': return NS_VK_SPACE;
    case '\t': return NS_VK_TAB;
    case ':': return NS_VK_COLON;
    case ';': return NS_VK_SEMICOLON;
    case '<': return NS_VK_LESS_THAN;
    case '=': return NS_VK_EQUALS;
    case '>': return NS_VK_GREATER_THAN;
    case '?': return NS_VK_QUESTION_MARK;
    case '@': return NS_VK_AT;
    case '^': return NS_VK_CIRCUMFLEX;
    case '!': return NS_VK_EXCLAMATION;
    case '"': return NS_VK_DOUBLE_QUOTE;
    case '#': return NS_VK_HASH;
    case '$': return NS_VK_DOLLAR;
    case '%': return NS_VK_PERCENT;
    case '&': return NS_VK_AMPERSAND;
    case '_': return NS_VK_UNDERSCORE;
    case '(': return NS_VK_OPEN_PAREN;
    case ')': return NS_VK_CLOSE_PAREN;
    case '*': return NS_VK_ASTERISK;
    case '+': return NS_VK_PLUS;
    case '|': return NS_VK_PIPE;
    case '-': return NS_VK_HYPHEN_MINUS;
    case '{': return NS_VK_OPEN_CURLY_BRACKET;
    case '}': return NS_VK_CLOSE_CURLY_BRACKET;
    case '~': return NS_VK_TILDE;
    case ',': return NS_VK_COMMA;
    case '.': return NS_VK_PERIOD;
    case '/': return NS_VK_SLASH;
    case '`': return NS_VK_BACK_QUOTE;
    case '[': return NS_VK_OPEN_BRACKET;
    case '\\': return NS_VK_BACK_SLASH;
    case ']': return NS_VK_CLOSE_BRACKET;
    case '\'': return NS_VK_QUOTE;
  }
  return 0;
}


void
WidgetUtils::GetLatinCharCodeForKeyCode(uint32_t aKeyCode,
                                        bool aIsCapsLock,
                                        uint32_t* aUnshiftedCharCode,
                                        uint32_t* aShiftedCharCode)
{
  MOZ_ASSERT(aUnshiftedCharCode && aShiftedCharCode,
             "aUnshiftedCharCode and aShiftedCharCode must not be NULL");

  if (aKeyCode >= NS_VK_A && aKeyCode <= NS_VK_Z) {
    *aUnshiftedCharCode = *aShiftedCharCode = aKeyCode;
    if (aIsCapsLock) {
      *aShiftedCharCode += 0x20;
    } else {
      *aUnshiftedCharCode += 0x20;
    }
    return;
  }

  
  *aShiftedCharCode = 0;

  if (aKeyCode >= NS_VK_0 && aKeyCode <= NS_VK_9) {
    *aUnshiftedCharCode = aKeyCode;
    return;
  }

  switch (aKeyCode) {
    case NS_VK_SPACE:               *aUnshiftedCharCode = ' '; break;
    case NS_VK_COLON:               *aUnshiftedCharCode = ':'; break;
    case NS_VK_SEMICOLON:           *aUnshiftedCharCode = ';'; break;
    case NS_VK_LESS_THAN:           *aUnshiftedCharCode = '<'; break;
    case NS_VK_EQUALS:              *aUnshiftedCharCode = '='; break;
    case NS_VK_GREATER_THAN:        *aUnshiftedCharCode = '>'; break;
    case NS_VK_QUESTION_MARK:       *aUnshiftedCharCode = '?'; break;
    case NS_VK_AT:                  *aUnshiftedCharCode = '@'; break;
    case NS_VK_CIRCUMFLEX:          *aUnshiftedCharCode = '^'; break;
    case NS_VK_EXCLAMATION:         *aUnshiftedCharCode = '!'; break;
    case NS_VK_DOUBLE_QUOTE:        *aUnshiftedCharCode = '"'; break;
    case NS_VK_HASH:                *aUnshiftedCharCode = '#'; break;
    case NS_VK_DOLLAR:              *aUnshiftedCharCode = '$'; break;
    case NS_VK_PERCENT:             *aUnshiftedCharCode = '%'; break;
    case NS_VK_AMPERSAND:           *aUnshiftedCharCode = '&'; break;
    case NS_VK_UNDERSCORE:          *aUnshiftedCharCode = '_'; break;
    case NS_VK_OPEN_PAREN:          *aUnshiftedCharCode = '('; break;
    case NS_VK_CLOSE_PAREN:         *aUnshiftedCharCode = ')'; break;
    case NS_VK_ASTERISK:            *aUnshiftedCharCode = '*'; break;
    case NS_VK_PLUS:                *aUnshiftedCharCode = '+'; break;
    case NS_VK_PIPE:                *aUnshiftedCharCode = '|'; break;
    case NS_VK_HYPHEN_MINUS:        *aUnshiftedCharCode = '-'; break;
    case NS_VK_OPEN_CURLY_BRACKET:  *aUnshiftedCharCode = '{'; break;
    case NS_VK_CLOSE_CURLY_BRACKET: *aUnshiftedCharCode = '}'; break;
    case NS_VK_TILDE:               *aUnshiftedCharCode = '~'; break;
    case NS_VK_COMMA:               *aUnshiftedCharCode = ','; break;
    case NS_VK_PERIOD:              *aUnshiftedCharCode = '.'; break;
    case NS_VK_SLASH:               *aUnshiftedCharCode = '/'; break;
    case NS_VK_BACK_QUOTE:          *aUnshiftedCharCode = '`'; break;
    case NS_VK_OPEN_BRACKET:        *aUnshiftedCharCode = '['; break;
    case NS_VK_BACK_SLASH:          *aUnshiftedCharCode = '\\'; break;
    case NS_VK_CLOSE_BRACKET:       *aUnshiftedCharCode = ']'; break;
    case NS_VK_QUOTE:               *aUnshiftedCharCode = '\''; break;
    default:                        *aUnshiftedCharCode = 0; break;
  }
}


KeyNameIndex
WidgetUtils::GetDeadKeyNameIndex(PRUnichar aChar)
{
  switch (aChar) {
    case '`':
    case 0x02CB: 
    case 0x0300: 
      return KEY_NAME_INDEX_DeadGrave;
    case '\'':
    case 0x00B4: 
    case 0x02B9: 
    case 0x02CA: 
    case 0x0301: 
    case 0x0384: 
      return KEY_NAME_INDEX_DeadAcute;
    case '^':
    case 0x02C6: 
    case 0x0302: 
      return KEY_NAME_INDEX_DeadCircumflex;
    case '~':
    case 0x02DC: 
    case 0x0303: 
      return KEY_NAME_INDEX_DeadTilde;
    case 0x00AF: 
    case 0x02C9: 
    case 0x0304: 
      return KEY_NAME_INDEX_DeadMacron;
    case 0x02D8: 
    case 0xA67C: 
    case 0x0306: 
      return KEY_NAME_INDEX_DeadBreve;
    case 0x02D9: 
    case 0x0307: 
      return KEY_NAME_INDEX_DeadAboveDot;
    case 0x00A8: 
    case 0x0308: 
      return KEY_NAME_INDEX_DeadUmlaut;
    case 0x00B0: 
    case 0x02DA: 
    case 0x030A: 
      return KEY_NAME_INDEX_DeadAboveRing;
    case '"':
    case 0x02BA: 
    case 0x02DD: 
    case 0x030B: 
      return KEY_NAME_INDEX_DeadDoubleacute;
    case 0x02C7: 
    case 0x030C: 
      return KEY_NAME_INDEX_DeadCaron;
    case 0x00B8: 
    case 0x0327: 
      return KEY_NAME_INDEX_DeadCedilla;
    case 0x02DB: 
    case 0x0328: 
      return KEY_NAME_INDEX_DeadOgonek;
    case 0x0345: 
    case 0x037A: 
    case 0x0399: 
      return KEY_NAME_INDEX_DeadIota;
    case 0x3099: 
    case 0x309B: 
      return KEY_NAME_INDEX_DeadVoicedSound;
    case 0x309A: 
    case 0x309C: 
      return KEY_NAME_INDEX_DeadSemivoicedSound;
    default:
      return KEY_NAME_INDEX_Unidentified;
  }
}

} 
} 
