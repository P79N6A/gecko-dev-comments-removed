















































let EXPORTED_SYMBOLS = ["Point", "Rect", "Range", "Subscribable", "Utils"];


const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");









function Point(a, y) {
  if (Utils.isPoint(a)) {
    this.x = a.x;
    this.y = a.y;
  } else {
    this.x = (Utils.isNumber(a) ? a : 0);
    this.y = (Utils.isNumber(y) ? y : 0);
  }
};

Point.prototype = {
  
  
  
  distance: function(point) {
    var ax = this.x - point.x;
    var ay = this.y - point.y;
    return Math.sqrt((ax * ax) + (ay * ay));
  }
};










function Rect(a, top, width, height) {
  
  if (Utils.isRect(a)) {
    this.left = a.left;
    this.top = a.top;
    this.width = a.width;
    this.height = a.height;
  } else {
    this.left = a;
    this.top = top;
    this.width = width;
    this.height = height;
  }
};

Rect.prototype = {

  get right() this.left + this.width,
  set right(value) {
    this.width = value - this.left;
  },

  get bottom() this.top + this.height,
  set bottom(value) {
    this.height = value - this.top;
  },

  
  
  
  get xRange() new Range(this.left, this.right),

  
  
  
  get yRange() new Range(this.top, this.bottom),

  
  
  
  intersects: function(rect) {
    return (rect.right > this.left
        && rect.left < this.right
        && rect.bottom > this.top
        && rect.top < this.bottom);
  },

  
  
  
  
  intersection: function(rect) {
    var box = new Rect(Math.max(rect.left, this.left), Math.max(rect.top, this.top), 0, 0);
    box.right = Math.min(rect.right, this.right);
    box.bottom = Math.min(rect.bottom, this.bottom);
    if (box.width > 0 && box.height > 0)
      return box;

    return null;
  },

  
  
  
  
  
  
  
  contains: function(rect){
    return(rect.left > this.left
         && rect.right < this.right
         && rect.top > this.top
         && rect.bottom < this.bottom)
  },

  
  
  
  center: function() {
    return new Point(this.left + (this.width / 2), this.top + (this.height / 2));
  },

  
  
  
  size: function() {
    return new Point(this.width, this.height);
  },

  
  
  
  position: function() {
    return new Point(this.left, this.top);
  },

  
  
  
  area: function() {
    return this.width * this.height;
  },

  
  
  
  
  
  
  
  inset: function(a, b) {
    if (Utils.isPoint(a)) {
      b = a.y;
      a = a.x;
    }

    this.left += a;
    this.width -= a * 2;
    this.top += b;
    this.height -= b * 2;
  },

  
  
  
  
  
  
  offset: function(a, b) {
    if (Utils.isPoint(a)) {
      this.left += a.x;
      this.top += a.y;
    } else {
      this.left += a;
      this.top += b;
    }
  },

  
  
  
  equals: function(rect) {
    return (rect.left == this.left
        && rect.top == this.top
        && rect.width == this.width
        && rect.height == this.height);
  },

  
  
  
  union: function(a){
    var newLeft = Math.min(a.left, this.left);
    var newTop = Math.min(a.top, this.top);
    var newWidth = Math.max(a.right, this.right) - newLeft;
    var newHeight = Math.max(a.bottom, this.bottom) - newTop;
    var newRect = new Rect(newLeft, newTop, newWidth, newHeight);

    return newRect;
  },

  
  
  
  copy: function(a) {
    this.left = a.left;
    this.top = a.top;
    this.width = a.width;
    this.height = a.height;
  },

  
  
  
  
  
  
  
  css: function() {
    return {
      left: this.left,
      top: this.top,
      width: this.width,
      height: this.height
    };
  }
};







function Range(min, max) {
  if (Utils.isRange(min) && !max) { 
    this.min = min.min;
    this.max = min.max;
  } else {
    this.min = min || 0;
    this.max = max || 0;
  }
};

Range.prototype = {
  
  
  get extent() {
    return (this.max - this.min);
  },

  set extent(extent) {
    this.max = extent - this.min;
  },

  
  
  
  
  
  
  contains: function(value) {
    return Utils.isNumber(value) ?
      value >= this.min && value <= this.max :
      Utils.isRange(value) ?
        (value.min <= this.max && this.min <= value.max) :
        false;
  },

  
  
  
  
  
  
  
  
  proportion: function(value, smooth) {
    if (value <= this.min)
      return 0;
    if (this.max <= value)
      return 1;

    var proportion = (value - this.min) / this.extent;

    if (smooth) {
      
      
      
      
      function tanh(x){
        var e = Math.exp(x);
        return (e - 1/e) / (e + 1/e);
      }
      return .5 - .5 * tanh(2 - 4 * proportion);
    }

    return proportion;
  },

  
  
  
  
  
  
  scale: function(value) {
    if (value > 1)
      value = 1;
    if (value < 0)
      value = 0;
    return this.min + this.extent * value;
  }
};




function Subscribable() {
  this.subscribers = null;
};

Subscribable.prototype = {
  
  
  
  
  addSubscriber: function(refObject, eventName, callback) {
    try {
      Utils.assertThrow("refObject", refObject);
      Utils.assertThrow("callback must be a function", typeof callback == "function");
      Utils.assertThrow("eventName must be a non-empty string",
          eventName && typeof(eventName) == "string");

      if (!this.subscribers)
        this.subscribers = {};

      if (!this.subscribers[eventName])
        this.subscribers[eventName] = [];

      var subs = this.subscribers[eventName];
      var existing = subs.filter(function(element) {
        return element.refObject == refObject;
      });

      if (existing.length) {
        Utils.assert('should only ever be one', existing.length == 1);
        existing[0].callback = callback;
      } else {
        subs.push({
          refObject: refObject,
          callback: callback
        });
      }
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  removeSubscriber: function(refObject, eventName) {
    try {
      Utils.assertThrow("refObject", refObject);
      Utils.assertThrow("eventName must be a non-empty string",
          eventName && typeof(eventName) == "string");

      if (!this.subscribers || !this.subscribers[eventName])
        return;

      this.subscribers[eventName] = this.subscribers[eventName].filter(function(element) {
        return element.refObject != refObject;
      });
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  _sendToSubscribers: function(eventName, eventInfo) {
    try {
      Utils.assertThrow("eventName must be a non-empty string",
          eventName && typeof(eventName) == "string");

      if (!this.subscribers || !this.subscribers[eventName])
        return;

      var self = this;
      var subsCopy = this.subscribers[eventName].concat();
      subsCopy.forEach(function(object) {
        object.callback(self, eventInfo);
      });
    } catch(e) {
      Utils.log(e);
    }
  }
};




let Utils = {
  

  
  
  
  
  log: function() {
    var text = this.expandArgumentsForLog(arguments);
    Services.console.logStringMessage(text);
  },

  
  
  
  
  error: function() {
    var text = this.expandArgumentsForLog(arguments);
    Cu.reportError("tabview error: " + text);
  },

  
  
  
  
  
  trace: function() {
    var text = this.expandArgumentsForLog(arguments);
    
    let stack = Error().stack.replace(/^.*?\n.*?\n/, "");
    
    if (this.trace.caller.name == 'Utils_assert')
      stack = stack.replace(/^.*?\n/, "");
    this.log('trace: ' + text + '\n' + stack);
  },

  
  
  
  assert: function Utils_assert(label, condition) {
    if (!condition) {
      let text;
      if (typeof(label) == 'undefined')
        text = 'badly formed assert';
      else
        text = "tabview assert: " + label;

      this.trace(text);
    }
  },

  
  
  
  assertThrow: function(label, condition) {
    if (!condition) {
      let text;
      if (typeof(label) == 'undefined')
        text = 'badly formed assert';
      else
        text = "tabview assert: " + label;

      
      text += Error().stack.replace(/^.*?\n.*?\n/, "");

      throw text;
    }
  },

  
  
  
  expandObject: function(obj) {
    var s = obj + ' = {';
    for (let prop in obj) {
      let value;
      try {
        value = obj[prop];
      } catch(e) {
        value = '[!!error retrieving property]';
      }

      s += prop + ': ';
      if (typeof(value) == 'string')
        s += '\'' + value + '\'';
      else if (typeof(value) == 'function')
        s += 'function';
      else
        s += value;

      s += ', ';
    }
    return s + '}';
  },

  
  
  
  expandArgumentsForLog: function(args) {
    var that = this;
    return Array.map(args, function(arg) {
      return typeof(arg) == 'object' ? that.expandObject(arg) : arg;
    }).join('; ');
  },

  

  
  
  
  isRightClick: function(event) {
    return event.button == 2;
  },

  
  
  
  isDOMElement: function(object) {
    return (object && typeof(object.nodeType) != 'undefined' ? true : false);
  },

  
  
  
  isNumber: function(n) {
    return (typeof(n) == 'number' && !isNaN(n));
  },

  
  
  
  isRect: function(r) {
    return (r
        && this.isNumber(r.left)
        && this.isNumber(r.top)
        && this.isNumber(r.width)
        && this.isNumber(r.height));
  },

  
  
  
  isRange: function(r) {
    return (r
        && this.isNumber(r.min)
        && this.isNumber(r.max));
  },

  
  
  
  isPoint: function(p) {
    return (p && this.isNumber(p.x) && this.isNumber(p.y));
  },

  
  
  
  isPlainObject: function(obj) {
    
    
    if (!obj || Object.prototype.toString.call(obj) !== "[object Object]"
        || obj.nodeType || obj.setInterval) {
      return false;
    }

    
    const hasOwnProperty = Object.prototype.hasOwnProperty;

    if (obj.constructor
      && !hasOwnProperty.call(obj, "constructor")
      && !hasOwnProperty.call(obj.constructor.prototype, "isPrototypeOf")) {
      return false;
    }

    
    

    var key;
    for (key in obj) {}

    return key === undefined || hasOwnProperty.call(obj, key);
  },

  
  
  
  isEmptyObject: function(obj) {
    for (let name in obj)
      return false;
    return true;
  },

  
  
  
  
  copy: function(value) {
    if (value && typeof(value) == 'object') {
      if (Array.isArray(value))
        return this.extend([], value);
      return this.extend({}, value);
    }
    return value;
  },

  
  
  
  merge: function(first, second) {
    Array.forEach(second, function(el) Array.push(first, el));
    return first;
  },

  
  
  
  extend: function() {
    
    var target = arguments[0] || {}, i = 1, length = arguments.length, options, name, src, copy;

    
    if (typeof target === "boolean") {
      this.assert("The first argument of extend cannot be a boolean."
                   +"Deep copy is not supported.", false);
      return target;
    }

    
    
    if (length === 1) {
      this.assert("Extending the iQ prototype using extend is not supported.", false);
      return target;
    }

    
    if (typeof target != "object" && typeof target != "function") {
      target = {};
    }

    for (; i < length; i++) {
      
      if ((options = arguments[i]) != null) {
        
        for (name in options) {
          src = target[name];
          copy = options[name];

          
          if (target === copy)
            continue;

          if (copy !== undefined)
            target[name] = copy;
        }
      }
    }

    
    return target;
  },

  
  
  
  timeout: function(func, delay) {
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback({
      notify: function notify() {
        try {
          func();
        }
        catch(ex) {
          Utils.log(timer, ex);
        }
      }
    }, delay, timer.TYPE_ONE_SHOT);
  }
};
