






































#ifndef _states_h_
#define _states_h_

#include <prtypes.h>

namespace states {

  


  const PRUint64 UNAVAILABLE = ((PRUint64) 0x1) << 0;

  


  const PRUint64 SELECTED = ((PRUint64) 0x1) << 1;

  


  const PRUint64 FOCUSED = ((PRUint64) 0x1) << 2;

  


  const PRUint64 PRESSED = ((PRUint64) 0x1) << 3;

  



  const PRUint64 CHECKED = ((PRUint64) 0x1) << 4;

  




  const PRUint64 MIXED = ((PRUint64) 0x1) << 5;

  


  const PRUint64 READONLY = ((PRUint64) 0x1) << 6;

  





  const PRUint64 HOTTRACKED = ((PRUint64) 0x1) << 7;

  


  const PRUint64 DEFAULT = ((PRUint64) 0x1) << 8;

  




  const PRUint64 EXPANDED = ((PRUint64) 0x1) << 9;

  




  const PRUint64 COLLAPSED = ((PRUint64) 0x1) << 10;

  


  const PRUint64 BUSY = ((PRUint64) 0x1) << 11;

  



  const PRUint64 FLOATING = ((PRUint64) 0x1) << 12;

  


  const PRUint64 CHECKABLE = ((PRUint64) 0x1) << 13;

  


  const PRUint64 ANIMATED = ((PRUint64) 0x1) << 14;

  



  const PRUint64 INVISIBLE = ((PRUint64) 0x1) << 15;

  




  const PRUint64 OFFSCREEN = ((PRUint64) 0x1) << 16;

  


  const PRUint64 SIZEABLE = ((PRUint64) 0x1) << 17;

  


  const PRUint64 MOVEABLE = ((PRUint64) 0x1) << 18;

  




  const PRUint64 SELFVOICING = ((PRUint64) 0x1) << 19;

  


  const PRUint64 FOCUSABLE = ((PRUint64) 0x1) << 20;

  


  const PRUint64 SELECTABLE = ((PRUint64) 0x1) << 21;

  


  const PRUint64 LINKED = ((PRUint64) 0x1) << 22;

  



  const PRUint64 TRAVERSED = ((PRUint64) 0x1) << 23;

  


  const PRUint64 MULTISELECTABLE = ((PRUint64) 0x1) << 24;

  




  const PRUint64 EXTSELECTABLE = ((PRUint64) 0x1) << 25;

  


  const PRUint64 REQUIRED = ((PRUint64) 0x1) << 26;

  


  const PRUint64 ALERT = ((PRUint64) 0x1) << 27;

  


  const PRUint64 INVALID = ((PRUint64) 0x1) << 28;

  


  const PRUint64 PROTECTED = ((PRUint64) 0x1) << 29;

  


 const PRUint64 HASPOPUP = ((PRUint64) 0x1) << 30;

 


  const PRUint64 SUPPORTS_AUTOCOMPLETION = ((PRUint64) 0x1) << 31;

  


  const PRUint64 DEFUNCT = ((PRUint64) 0x1) << 32;

  


  const PRUint64 SELECTABLE_TEXT = ((PRUint64) 0x1) << 33;

  


  const PRUint64 EDITABLE = ((PRUint64) 0x1) << 34;

  


  const PRUint64 ACTIVE = ((PRUint64) 0x1) << 35;

  




  const PRUint64 MODAL = ((PRUint64) 0x1) << 36;

  


  const PRUint64 MULTI_LINE = ((PRUint64) 0x1) << 37;

  


  const PRUint64 HORIZONTAL = ((PRUint64) 0x1) << 38;

  


  const PRUint64 OPAQUE1 = ((PRUint64) 0x1) << 39;

  


  const PRUint64 SINGLE_LINE = ((PRUint64) 0x1) << 40;

  




  const PRUint64 TRANSIENT = ((PRUint64) 0x1) << 41;

  



  const PRUint64 VERTICAL = ((PRUint64) 0x1) << 42;

  


  const PRUint64 STALE = ((PRUint64) 0x1) << 43;

  


  const PRUint64 ENABLED = ((PRUint64) 0x1) << 44;

  


  const PRUint64 SENSITIVE = ((PRUint64) 0x1) << 45;

  



  const PRUint64 EXPANDABLE = ((PRUint64) 0x1) << 46;
}

#endif
	
