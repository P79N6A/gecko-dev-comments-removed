


"use strict";

this.EXPORTED_SYMBOLS = ["ContentUtil"];

this.ContentUtil = {
  
  
  extend: function extend() {
    
    let target = arguments[0] || {};
    let length = arguments.length;

    if (length === 1) {
      return target;
    }

    
    if (typeof target != "object" && typeof target != "function") {
      target = {};
    }

    for (let i = 1; i < length; i++) {
      
      let options = arguments[i];
      if (options != null) {
        
        for (let name in options) {
          let copy = options[name];

          
          if (target === copy)
            continue;

          if (copy !== undefined)
            target[name] = copy;
        }
      }
    }

    
    return target;
  }

};
