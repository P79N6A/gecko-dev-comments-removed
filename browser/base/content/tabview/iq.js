
























































function iQ(selector, context) {
  
  return new iQClass(selector, context);
};



let quickExpr = /^[^<]*(<[\w\W]+>)[^>]*$|^#([\w-]+)$/;


let rsingleTag = /^<(\w+)\s*\/?>(?:<\/\1>)?$/;









function iQClass(selector, context) {

  
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
            Utils.assert(false, 'does not support HTML creation with context');
          } else {
            selector = [doc.createElement(ret[1])];
          }

        } else {
          Utils.assert(false, 'does not support complex HTML creation');
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

  if ("selector" in selector) {
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
};
  
iQClass.prototype = {

  
  
  
  toString: function iQClass_toString() {
    if (this.length > 1) {
      if (this.selector)
        return "[iQ (" + this.selector + ")]";
      else
        return "[iQ multi-object]";
    }

    if (this.length == 1)
      return "[iQ (" + this[0].toString() + ")]";

    return "[iQ non-object]";
  },

  
  selector: "",

  
  length: 0,

  
  
  
  each: function iQClass_each(callback) {
    if (typeof callback != "function") {
      Utils.assert(false, "each's argument must be a function");
      return null;
    }
    for (let i = 0; this[i] != null && callback(this[i]) !== false; i++) {}
    return this;
  },

  
  
  
  addClass: function iQClass_addClass(value) {
    Utils.assertThrow(typeof value == "string" && value,
                      'requires a valid string argument');

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

  
  
  
  removeClass: function iQClass_removeClass(value) {
    if (typeof value != "string" || !value) {
      Utils.assert(false, 'does not support function argument');
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

  
  
  
  hasClass: function iQClass_hasClass(singleClassName) {
    let length = this.length;
    for (let i = 0; i < length; i++) {
      if (this[i].classList.contains(singleClassName)) {
        return true;
      }
    }
    return false;
  },

  
  
  
  
  find: function iQClass_find(selector) {
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

  
  
  
  contains: function iQClass_contains(selector) {
    Utils.assert(this.length == 1, 'does not yet support multi-objects (or null objects)');

    
    if ('string' == typeof selector)
      return null != this[0].querySelector(selector);

    let object = iQ(selector);
    Utils.assert(object.length <= 1, 'does not yet support multi-objects');

    let elem = object[0];
    if (!elem || !elem.parentNode)
      return false;

    do {
      elem = elem.parentNode;
    } while (elem && this[0] != elem);

    return this[0] == elem;
  },

  
  
  
  remove: function iQClass_remove() {
    for (let i = 0; this[i] != null; i++) {
      let elem = this[i];
      if (elem.parentNode) {
        elem.parentNode.removeChild(elem);
      }
    }
    return this;
  },

  
  
  
  empty: function iQClass_empty() {
    for (let i = 0; this[i] != null; i++) {
      let elem = this[i];
      while (elem.firstChild) {
        elem.removeChild(elem.firstChild);
      }
    }
    return this;
  },

  
  
  
  width: function iQClass_width() {
    return Math.floor(this[0].offsetWidth);
  },

  
  
  
  height: function iQClass_height() {
    return Math.floor(this[0].offsetHeight);
  },

  
  
  
  
  position: function iQClass_position() {
    let bounds = this.bounds();
    return new Point(bounds.left, bounds.top);
  },

  
  
  
  bounds: function iQClass_bounds() {
    Utils.assert(this.length == 1, 'does not yet support multi-objects (or null objects)');
    let rect = this[0].getBoundingClientRect();
    return new Rect(Math.floor(rect.left), Math.floor(rect.top),
                    Math.floor(rect.width), Math.floor(rect.height));
  },

  
  
  
  
  data: function iQClass_data(key, value) {
    let data = null;
    if (value === undefined) {
      Utils.assert(this.length == 1, 'does not yet support multi-objects (or null objects)');
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

  
  
  
  
  html: function iQClass_html(value) {
    Utils.assert(this.length == 1, 'does not yet support multi-objects (or null objects)');
    if (value === undefined)
      return this[0].innerHTML;

    this[0].innerHTML = value;
    return this;
  },

  
  
  
  
  text: function iQClass_text(value) {
    Utils.assert(this.length == 1, 'does not yet support multi-objects (or null objects)');
    if (value === undefined) {
      return this[0].textContent;
    }

    return this.empty().append((this[0] && this[0].ownerDocument || document).createTextNode(value));
  },

  
  
  
  val: function iQClass_val(value) {
    Utils.assert(this.length == 1, 'does not yet support multi-objects (or null objects)');
    if (value === undefined) {
      return this[0].value;
    }

    this[0].value = value;
    return this;
  },

  
  
  
  appendTo: function iQClass_appendTo(selector) {
    Utils.assert(this.length == 1, 'does not yet support multi-objects (or null objects)');
    iQ(selector).append(this);
    return this;
  },

  
  
  
  append: function iQClass_append(selector) {
    let object = iQ(selector);
    Utils.assert(object.length == 1 && this.length == 1, 
        'does not yet support multi-objects (or null objects)');
    this[0].appendChild(object[0]);
    return this;
  },

  
  
  
  attr: function iQClass_attr(key, value) {
    Utils.assert(typeof key === 'string', 'string key');
    if (value === undefined) {
      Utils.assert(this.length == 1, 'retrieval does not support multi-objects (or null objects)');
      return this[0].getAttribute(key);
    }

    for (let i = 0; this[i] != null; i++)
      this[i].setAttribute(key, value);

    return this;
  },

  
  
  
  
  
  
  
  
  
  css: function iQClass_css(a, b) {
    let properties = null;

    if (typeof a === 'string') {
      let key = a;
      if (b === undefined) {
        Utils.assert(this.length == 1, 'retrieval does not support multi-objects (or null objects)');

        return window.getComputedStyle(this[0], null).getPropertyValue(key);
      }
      properties = {};
      properties[key] = b;
    } else if (a instanceof Rect) {
      properties = {
        left: a.left,
        top: a.top,
        width: a.width,
        height: a.height
      };
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

        if (pixels[key] && typeof value != 'string')
          value += 'px';

        if (value == null) {
          elem.style.removeProperty(key);
        } else if (key.indexOf('-') != -1)
          elem.style.setProperty(key, value, '');
        else
          elem.style[key] = value;
      }
    }

    return this;
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  animate: function iQClass_animate(css, options) {
    Utils.assert(this.length == 1, 'does not yet support multi-objects (or null objects)');

    if (!options)
      options = {};

    let easings = {
      tabviewBounce: "cubic-bezier(0.0, 0.63, .6, 1.29)", 
      easeInQuad: 'ease-in', 
      fast: 'cubic-bezier(0.7,0,1,1)'
    };

    let duration = (options.duration || 400);
    let easing = (easings[options.easing] || 'ease');

    if (css instanceof Rect) {
      css = {
        left: css.left,
        top: css.top,
        width: css.width,
        height: css.height
      };
    }


    
    
    
    let rupper = /([A-Z])/g;
    this.each(function(elem) {
      let cStyle = window.getComputedStyle(elem, null);
      for (let prop in css) {
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
    setTimeout(function() {
      self.css({
        '-moz-transition-property': 'none',
        '-moz-transition-duration': '',
        '-moz-transition-timing-function': ''
      });

      if (typeof options.complete == "function")
        options.complete.apply(self);
    }, duration);

    return this;
  },

  
  
  
  fadeOut: function iQClass_fadeOut(callback) {
    Utils.assert(typeof callback == "function" || callback === undefined, 
        'does not yet support duration');

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

  
  
  
  fadeIn: function iQClass_fadeIn() {
    this.css({display: ''});
    this.animate({
      opacity: 1
    }, {
      duration: 400
    });

    return this;
  },

  
  
  
  hide: function iQClass_hide() {
    this.css({display: 'none', opacity: 0});
    return this;
  },

  
  
  
  show: function iQClass_show() {
    this.css({display: '', opacity: 1});
    return this;
  },

  
  
  
  
  bind: function iQClass_bind(type, func) {
    let handler = function(event) func.apply(this, [event]);

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

  
  
  
  
  one: function iQClass_one(type, func) {
    Utils.assert(typeof func == "function", 'does not support eventData argument');

    let handler = function(e) {
      iQ(this).unbind(type, handler);
      return func.apply(this, [e]);
    };

    return this.bind(type, handler);
  },

  
  
  
  unbind: function iQClass_unbind(type, func) {
    Utils.assert(typeof func == "function", 'Must provide a function');

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
  'keypress',
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
