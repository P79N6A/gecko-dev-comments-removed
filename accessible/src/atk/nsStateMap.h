






































#include <atk/atk.h>
#include "nsAccessibleWrap.h"




























enum EStateMapEntryType {
  kMapDirectly,
  kMapOpposite,   
  kNoStateChange, 
  kNoSuchState
};

const AtkStateType kNone = ATK_STATE_INVALID;

struct AtkStateMap {
  AtkStateType atkState;
  EStateMapEntryType stateMapEntryType;

  static PRInt32 GetStateIndexFor(PRUint64 aState)
  {
    PRInt32 stateIndex = -1;
    while (aState > 0) {
      ++ stateIndex;
      aState >>= 1;
    }
    return stateIndex;  
  }
};



static const AtkStateMap gAtkStateMap[] = {                     
  { kNone,                                    kMapOpposite },   
  { ATK_STATE_SELECTED,                       kMapDirectly },   
  { ATK_STATE_FOCUSED,                        kMapDirectly },   
  { ATK_STATE_PRESSED,                        kMapDirectly },   
  { ATK_STATE_CHECKED,                        kMapDirectly },   
  { ATK_STATE_INDETERMINATE,                  kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { ATK_STATE_DEFAULT,                        kMapDirectly },   
  { ATK_STATE_EXPANDED,                       kMapDirectly },   
  { kNone,                                    kNoStateChange }, 
  { ATK_STATE_BUSY,                           kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { ATK_STATE_ANIMATED,                       kMapDirectly },   
  { ATK_STATE_VISIBLE,                        kMapOpposite },   
  { ATK_STATE_SHOWING,                        kMapOpposite },   
  { ATK_STATE_RESIZABLE,                      kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { ATK_STATE_FOCUSABLE,                      kMapDirectly },   
  { ATK_STATE_SELECTABLE,                     kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { ATK_STATE_VISITED,                        kMapDirectly },   
  { ATK_STATE_MULTISELECTABLE,                kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { ATK_STATE_REQUIRED,                       kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { ATK_STATE_INVALID_ENTRY,                  kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { kNone,                                    kMapDirectly },   
  { ATK_STATE_SUPPORTS_AUTOCOMPLETION,        kMapDirectly },   
  { ATK_STATE_DEFUNCT,                        kMapDirectly },   
  { ATK_STATE_SELECTABLE_TEXT,                kMapDirectly },   
  { ATK_STATE_EDITABLE,                       kMapDirectly },   
  { ATK_STATE_ACTIVE,                         kMapDirectly },   
  { ATK_STATE_MODAL,                          kMapDirectly },   
  { ATK_STATE_MULTI_LINE,                     kMapDirectly },   
  { ATK_STATE_HORIZONTAL,                     kMapDirectly },   
  { ATK_STATE_OPAQUE,                         kMapDirectly },   
  { ATK_STATE_SINGLE_LINE,                    kMapDirectly },   
  { ATK_STATE_TRANSIENT,                      kMapDirectly },   
  { ATK_STATE_VERTICAL,                       kMapDirectly },   
  { ATK_STATE_STALE,                          kMapDirectly },   
  { ATK_STATE_ENABLED,                        kMapDirectly },   
  { ATK_STATE_SENSITIVE,                      kMapDirectly },   
  { ATK_STATE_EXPANDABLE,                     kMapDirectly },   
  { kNone,                                    kNoSuchState },   
};
