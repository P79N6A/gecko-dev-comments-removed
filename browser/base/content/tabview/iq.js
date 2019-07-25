






















































function iQ(selector, context) {
  
  return new iQClass(selector, context);
};



let quickExpr = /^[^<]*(<[\w\W]+>)[^>]*$|^#([\w-]+)$/;


let rsingleTag = /^<(\w+)\s*\/?>(?:<\/\1>)?$/;









let iQClass = function(selector, context) {

  
  if (!selector) {
    return this;
  }

  
  if (selector.nodeType) {
    this.context = selector;
    this[0] = selector;
    this.length = 1;
    return this;
  }

  
  if (selector === "body" && !context) {
    this.context = document;
    this[0] = document.body;
    this.selector = "body";
    this.length = 1;
    return this;
  }

  
  if (typeof selector === "string") {
    

    let match = quickExpr.exec(selector);

    
    if (match && (match[1] || !context)) {

      
      if (match[1]) {
        let doc = (context ? context.ownerDocument || context : document);

        
        
        let ret = rsingleTag.exec(selector);

        if (ret) {
          if (Utils.isPlainObject(context)) {
            Utils.assert('does not support HTML creation with context', false);
          } else {
            selector = [doc.createElement(ret[1])];
          }

        } else {
            Utils.assert('does not support complex HTML creation', false);
        }

        return Utils.merge(this, selector);

      
      } else {
        let elem = document.getElementById(match[2]);

        if (elem) {
          this.length = 1;
          this[0] = elem;
        }

        this.context = document;
        this.selector = selector;
        return this;
      }

    
    } else if (!context && /^\w+$/.test(selector)) {
      this.selector = selector;
      this.context = document;
      selector = document.getElementsByTagName(selector);
      return Utils.merge(this, selector);

    
    } else if (!context || context.iq) {
      return (context || iQ(document)).find(selector);

    
    
    } else {
      return iQ(context).find(selector);
    }

  
  
  } else if (typeof selector == "function") {
    Utils.log('iQ does not support ready functions');
    return null;
  }

  if (selector.selector !== undefined) {
    this.selector = selector.selector;
    this.context = selector.context;
  }

  let ret = this || [];
  if (selector != null) {
    
    if (selector.length == null || typeof selector == "string" || selector.setInterval) {
      Array.push(ret, selector);
    } else {
      Utils.merge(ret, selector);
    }
  }
  return ret;
}
iQClass.prototype = {

  
  selector: "",

  
  iq: "1.4.2",

  
  length: 0,

  
  
  
  each: function(callback) {
    if (typeof callback != "function") {
      Utils.assert("each's argument must be a function", false);
      return null;
    }
    for (let i = 0; this[i] != null; i++) {
      callback(this[i]);
    }
    return this;
  },

  
  
  
  addClass: function(value) {
    if (typeof value != "string" || !value) {
      Utils.assert('requires a valid string argument', false);
      return null;
    }

    let length = this.length;
    for (let i = 0; i < length; i++) {
      let elem = this[i];
      if (elem.nodeType === 1) {
        value.split(/\s+/).forEach(function(className) {
          elem.classList.add(className);
        });
      }
    }

    return this;
  },

  
  
  
  removeClass: function(value) {
    if (typeof value != "string" || !value) {
      Utils.assert('does not support function argument', false);
      return null;
    }

    let length = this.length;
    for (let i = 0; i < length; i++) {
      let elem = this[i];
      if (elem.nodeType === 1 && elem.className) {
        value.split(/\s+/).forEach(function(className) {
          elem.classList.remove(className);
        });
      }
    }

    return this;
  },

  
  
  
  hasClass: function(singleClassName) {
    let length = this.length;
    for (let i = 0; i < length; i++) {
      if (this[i].classList.contains(singleClassName)) {
        return true;
      }
    }
    return false;
  },

  
  
  
  
  find: function(selector) {
    let ret = [];
    let length = 0;

    let l = this.length;
    for (let i = 0; i < l; i++) {
      length = ret.length;
      try {
        Utils.merge(ret, this[i].querySelectorAll(selector));
      } catch(e) {
        Utils.log('iQ.find error (bad selector)', e);
      }

      if (i > 0) {
        
        for (let n = length; n < ret.length; n++) {
          for (let r = 0; r < length; r++) {
            if (ret[r] === ret[n]) {
              ret.splice(n--, 1);
              break;
            }
          }
        }
      }
    }

    return iQ(ret);
  },

  
  
  
  remove: function(unused) {
    Utils.assert('does not accept a selector', unused === undefined);
    for (let i = 0; this[i] != null; i++) {
      let elem = this[i];
      if (elem.parentNode) {
        elem.parentNode.removeChild(elem);
      }
    }
    return this;
  },

  
  
  
  empty: function() {
    for (let i = 0; this[i] != null; i++) {
      let elem = this[i];
      while (elem.firstChild) {
        elem.removeChild(elem.firstChild);
      }
    }
    return this;
  },

  
  
  
  width: function(unused) {
    Utils.assert('does not yet support setting', unused === undefined);
    return parseInt(this.css('width'));
  },

  
  
  
  height: function(unused) {
    Utils.assert('does not yet support setting', unused === undefined);
    return parseInt(this.css('height'));
  },

  
  
  
  
  position: function(unused) {
    Utils.assert('does not yet support setting', unused === undefined);
    return {
      left: parseInt(this.css('left')),
      top: parseInt(this.css('top'))
    };
  },

  
  
  
  bounds: function() {
    let p = this.position();
    return new Rect(p.left, p.top, this.width(), this.height());
  },

  
  
  
  
  data: function(key, value) {
    let data = null;
    if (value === undefined) {
      Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
      data = this[0].iQData;
      if (data)
        return data[key];
      else
        return null;
    }

    for (let i = 0; this[i] != null; i++) {
      let elem = this[i];
      data = elem.iQData;

      if (!data)
        data = elem.iQData = {};

      data[key] = value;
    }

    return this;
  },

  
  
  
  
  html: function(value) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    if (value === undefined)
      return this[0].innerHTML;

    this[0].innerHTML = value;
    return this;
  },

  
  
  
  
  text: function(value) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    if (value === undefined) {
      return this[0].textContent;
    }

    return this.empty().append((this[0] && this[0].ownerDocument || document).createTextNode(value));
  },

  
  
  
  val: function(value) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    if (value === undefined) {
      return this[0].value;
    }

    this[0].value = value;
    return this;
  },

  
  
  
  appendTo: function(selector) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    iQ(selector).append(this);
    return this;
  },

  
  
  
  append: function(selector) {
    let object = iQ(selector);
    Utils.assert('does not yet support multi-objects (or null objects)', object.length == 1 && this.length == 1);
    this[0].appendChild(object[0]);
    return this;
  },

  
  
  
  attr: function(key, value) {
    try {
      Utils.assert('string key', typeof key === 'string');
      if (value === undefined) {
        Utils.assert('retrieval does not support multi-objects (or null objects)', this.length == 1);
        return this[0].getAttribute(key);
      }
      for (let i = 0; this[i] != null; i++) {
        this[i].setAttribute(key, value);
      }
    } catch(e) {
      Utils.log(e);
    }

    return this;
  },

  
  
  
  
  
  
  
  
  
  css: function(a, b) {
    let properties = null;

    if (typeof a === 'string') {
      let key = a;
      if (b === undefined) {
        Utils.assert('retrieval does not support multi-objects (or null objects)', this.length == 1);

        let substitutions = {
          'MozTransform': '-moz-transform',
          'zIndex': 'z-index'
        };

        return window.getComputedStyle(this[0], null).getPropertyValue(substitutions[key] || key);
      }
      properties = {};
      properties[key] = b;
    } else {
      properties = a;
    }

    let pixels = {
      'left': true,
      'top': true,
      'right': true,
      'bottom': true,
      'width': true,
      'height': true
    };

    for (let i = 0; this[i] != null; i++) {
      let elem = this[i];
      for (let key in properties) {
        let value = properties[key];
        if (pixels[key] && typeof(value) != 'string')
          value += 'px';

        if (key.indexOf('-') != -1)
          elem.style.setProperty(key, value, '');
        else
          elem.style[key] = value;
      }
    }

    return this;
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  animate: function(css, options) {
    try {
      Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);

      if (!options)
        options = {};

      let easings = {
        tabviewBounce: "cubic-bezier(0.0, 0.63, .6, 1.29)",
        easeInQuad: 'ease-in', 
        fast: 'cubic-bezier(0.7,0,1,1)'
      };

      let duration = (options.duration || 400);
      let easing = (easings[options.easing] || 'ease');

      
      
      
      let rupper = /([A-Z])/g;
      this.each(function(elem){
        let cStyle = window.getComputedStyle(elem, null);
        for (let prop in css){
          prop = prop.replace(rupper, "-$1").toLowerCase();
          iQ(elem).css(prop, cStyle.getPropertyValue(prop));
        }
      });

      this.css({
        '-moz-transition-property': 'all', 
        '-moz-transition-duration': (duration / 1000) + 's',
        '-moz-transition-timing-function': easing
      });

      this.css(css);

      let self = this;
      Utils.timeout(function() {
        self.css({
          '-moz-transition-property': 'none',
          '-moz-transition-duration': '',
          '-moz-transition-timing-function': ''
        });

        if (typeof options.complete == "function")
          options.complete.apply(self);
      }, duration);
    } catch(e) {
      Utils.log(e);
    }

    return this;
  },

  
  
  
  fadeOut: function(callback) {
    Utils.assert('does not yet support duration', typeof callback == "function" || callback === undefined);
    this.animate({
      opacity: 0
    }, {
      duration: 400,
      complete: function() {
        iQ(this).css({display: 'none'});
        if (typeof callback == "function")
          callback.apply(this);
      }
    });

    return this;
  },

  
  
  
  fadeIn: function() {
    try {
      this.css({display: ''});
      this.animate({
        opacity: 1
      }, {
        duration: 400
      });
    } catch(e) {
      Utils.log(e);
    }

    return this;
  },

  
  
  
  hide: function() {
    try {
      this.css({display: 'none', opacity: 0});
    } catch(e) {
      Utils.log(e);
    }

    return this;
  },

  
  
  
  show: function() {
    try {
      this.css({display: '', opacity: 1});
    } catch(e) {
      Utils.log(e);
    }

    return this;
  },

  
  
  
  
  bind: function(type, func) {
    Utils.assert('does not support eventData argument', typeof func == "function");

    let handler = function(event) {
      try {
        return func.apply(this, [event]);
      } catch(e) {
        Utils.log(e);
      }
    };

    for (let i = 0; this[i] != null; i++) {
      let elem = this[i];
      if (!elem.iQEventData)
        elem.iQEventData = {};

      if (!elem.iQEventData[type])
        elem.iQEventData[type] = [];

      elem.iQEventData[type].push({
        original: func,
        modified: handler
      });

      elem.addEventListener(type, handler, false);
    }

    return this;
  },

  
  
  
  
  one: function(type, func) {
    Utils.assert('does not support eventData argument', typeof func == "function");

    let handler = function(e) {
      iQ(this).unbind(type, handler);
      return func.apply(this, [e]);
    };

    return this.bind(type, handler);
  },

  
  
  
  unbind: function(type, func) {
    Utils.assert('Must provide a function', typeof func == "function");

    for (let i = 0; this[i] != null; i++) {
      let elem = this[i];
      let handler = func;
      if (elem.iQEventData && elem.iQEventData[type]) {
        let count = elem.iQEventData[type].length;
        for (let a = 0; a < count; a++) {
          let pair = elem.iQEventData[type][a];
          if (pair.original == func) {
            handler = pair.modified;
            elem.iQEventData[type].splice(a, 1);
            break;
          }
        }
      }

      elem.removeEventListener(type, handler, false);
    }

    return this;
  }
};



let events = [
  'keyup',
  'keydown',
  'mouseup',
  'mousedown',
  'mouseover',
  'mouseout',
  'mousemove',
  'click',
  'resize',
  'change',
  'blur',
  'focus'
];

events.forEach(function(event) {
  iQClass.prototype[event] = function(func) {
    return this.bind(event, func);
  };
});
