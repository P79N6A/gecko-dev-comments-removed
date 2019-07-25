





#ifndef _states_h_
#define _states_h_

#include <prtypes.h>

namespace mozilla {
namespace a11y {
namespace states {

  


  const uint64_t UNAVAILABLE = ((uint64_t) 0x1) << 0;

  


  const uint64_t SELECTED = ((uint64_t) 0x1) << 1;

  


  const uint64_t FOCUSED = ((uint64_t) 0x1) << 2;

  


  const uint64_t PRESSED = ((uint64_t) 0x1) << 3;

  



  const uint64_t CHECKED = ((uint64_t) 0x1) << 4;

  




  const uint64_t MIXED = ((uint64_t) 0x1) << 5;

  


  const uint64_t READONLY = ((uint64_t) 0x1) << 6;

  





  const uint64_t HOTTRACKED = ((uint64_t) 0x1) << 7;

  


  const uint64_t DEFAULT = ((uint64_t) 0x1) << 8;

  




  const uint64_t EXPANDED = ((uint64_t) 0x1) << 9;

  




  const uint64_t COLLAPSED = ((uint64_t) 0x1) << 10;

  


  const uint64_t BUSY = ((uint64_t) 0x1) << 11;

  



  const uint64_t FLOATING = ((uint64_t) 0x1) << 12;

  


  const uint64_t CHECKABLE = ((uint64_t) 0x1) << 13;

  


  const uint64_t ANIMATED = ((uint64_t) 0x1) << 14;

  



  const uint64_t INVISIBLE = ((uint64_t) 0x1) << 15;

  




  const uint64_t OFFSCREEN = ((uint64_t) 0x1) << 16;

  


  const uint64_t SIZEABLE = ((uint64_t) 0x1) << 17;

  


  const uint64_t MOVEABLE = ((uint64_t) 0x1) << 18;

  




  const uint64_t SELFVOICING = ((uint64_t) 0x1) << 19;

  


  const uint64_t FOCUSABLE = ((uint64_t) 0x1) << 20;

  


  const uint64_t SELECTABLE = ((uint64_t) 0x1) << 21;

  


  const uint64_t LINKED = ((uint64_t) 0x1) << 22;

  



  const uint64_t TRAVERSED = ((uint64_t) 0x1) << 23;

  


  const uint64_t MULTISELECTABLE = ((uint64_t) 0x1) << 24;

  




  const uint64_t EXTSELECTABLE = ((uint64_t) 0x1) << 25;

  


  const uint64_t REQUIRED = ((uint64_t) 0x1) << 26;

  


  const uint64_t ALERT = ((uint64_t) 0x1) << 27;

  


  const uint64_t INVALID = ((uint64_t) 0x1) << 28;

  


  const uint64_t PROTECTED = ((uint64_t) 0x1) << 29;

  


 const uint64_t HASPOPUP = ((uint64_t) 0x1) << 30;

 


  const uint64_t SUPPORTS_AUTOCOMPLETION = ((uint64_t) 0x1) << 31;

  


  const uint64_t DEFUNCT = ((uint64_t) 0x1) << 32;

  


  const uint64_t SELECTABLE_TEXT = ((uint64_t) 0x1) << 33;

  


  const uint64_t EDITABLE = ((uint64_t) 0x1) << 34;

  


  const uint64_t ACTIVE = ((uint64_t) 0x1) << 35;

  




  const uint64_t MODAL = ((uint64_t) 0x1) << 36;

  


  const uint64_t MULTI_LINE = ((uint64_t) 0x1) << 37;

  


  const uint64_t HORIZONTAL = ((uint64_t) 0x1) << 38;

  


  const uint64_t OPAQUE1 = ((uint64_t) 0x1) << 39;

  


  const uint64_t SINGLE_LINE = ((uint64_t) 0x1) << 40;

  




  const uint64_t TRANSIENT = ((uint64_t) 0x1) << 41;

  



  const uint64_t VERTICAL = ((uint64_t) 0x1) << 42;

  


  const uint64_t STALE = ((uint64_t) 0x1) << 43;

  


  const uint64_t ENABLED = ((uint64_t) 0x1) << 44;

  


  const uint64_t SENSITIVE = ((uint64_t) 0x1) << 45;

  



  const uint64_t EXPANDABLE = ((uint64_t) 0x1) << 46;
} 
} 
} 

#endif
	
