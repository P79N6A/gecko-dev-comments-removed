


"use strict";

this.EXPORTED_SYMBOLS = ["ContentUtil"];

const XHTML_NS = "http://www.w3.org/1999/xhtml";

this.ContentUtil = {
  populateFragmentFromString: function populateFragmentFromString(fragment, str) {
    let re = /^([^#]*)#(\d+)\b([^#]*)/,
        document = fragment.ownerDocument,
        
        replacements = Array.slice(arguments, 2),
        match;

    
    
    while ((match = re.exec(str))) {
      let [mstring,pre,num,post] = match,
          replaceText = "",
          replaceClass,
          idx = num-1; 

      str = str.substring(re.lastIndex+mstring.length);

      if (pre)
          fragment.appendChild(document.createTextNode(pre));

      if (replacements[idx]) {
        replaceText = replacements[idx].text;
        let spanNode = document.createElementNS(XHTML_NS, "span");
        spanNode.appendChild(document.createTextNode(replaceText));
        
        if(replacements[idx].className)
          spanNode.classList.add(replacements[idx].className);

        fragment.appendChild(spanNode);
      } else {
        
        fragment.appendChild(document.createTextNode("#"+num));
      }

      if(post)
        fragment.appendChild(document.createTextNode(post));
    }
    if(str)
      fragment.appendChild(document.createTextNode(str));

    return fragment;
  },

  
  
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
