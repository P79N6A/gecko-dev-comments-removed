



"use strict";

this.EXPORTED_SYMBOLS = ["RevivableWindows"];



let closedWindows = [];











this.RevivableWindows = Object.freeze({
  
  get isEmpty() {
    return closedWindows.length == 0;
  },

  
  add(winState) {
#ifndef XP_MACOSX
    closedWindows.push(winState);
#endif
  },

  
  get() {
    return [...closedWindows];
  },

  
  clear() {
    closedWindows.length = 0;
  }
});
