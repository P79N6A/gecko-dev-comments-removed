

















































let EXPORTED_SYMBOLS = ["Point", "Rect", "Range", "Subscribable", "Utils"];


const Ci = Components.interfaces;
const Cu = Components.utils;

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
  
  
  
  toString: function Point_toString() {
    return "[Point (" + this.x + "," + this.y + ")]";
  },

  
  
  
  distance: function Point_distance(point) {
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
  
  
  
  toString: function Rect_toString() {
    return "[Rect (" + this.left + "," + this.top + "," +
            this.width + "," + this.height + ")]";
  },

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

  
  
  
  intersects: function Rect_intersects(rect) {
    return (rect.right > this.left &&
            rect.left < this.right &&
            rect.bottom > this.top &&
            rect.top < this.bottom);
  },

  
  
  
  
  intersection: function Rect_intersection(rect) {
    var box = new Rect(Math.max(rect.left, this.left), Math.max(rect.top, this.top), 0, 0);
    box.right = Math.min(rect.right, this.right);
    box.bottom = Math.min(rect.bottom, this.bottom);
    if (box.width > 0 && box.height > 0)
      return box;

    return null;
  },

  
  
  
  
  
  
  
  contains: function Rect_contains(a) {
    if (Utils.isPoint(a))
      return (a.x > this.left &&
              a.x < this.right &&
              a.y > this.top &&
              a.y < this.bottom);

    return (a.left >= this.left &&
            a.right <= this.right &&
            a.top >= this.top &&
            a.bottom <= this.bottom);
  },

  
  
  
  center: function Rect_center() {
    return new Point(this.left + (this.width / 2), this.top + (this.height / 2));
  },

  
  
  
  size: function Rect_size() {
    return new Point(this.width, this.height);
  },

  
  
  
  position: function Rect_position() {
    return new Point(this.left, this.top);
  },

  
  
  
  area: function Rect_area() {
    return this.width * this.height;
  },

  
  
  
  
  
  
  
  inset: function Rect_inset(a, b) {
    if (Utils.isPoint(a)) {
      b = a.y;
      a = a.x;
    }

    this.left += a;
    this.width -= a * 2;
    this.top += b;
    this.height -= b * 2;
  },

  
  
  
  
  
  
  offset: function Rect_offset(a, b) {
    if (Utils.isPoint(a)) {
      this.left += a.x;
      this.top += a.y;
    } else {
      this.left += a;
      this.top += b;
    }
  },

  
  
  
  equals: function Rect_equals(rect) {
    return (rect.left == this.left &&
            rect.top == this.top &&
            rect.width == this.width &&
            rect.height == this.height);
  },

  
  
  
  union: function Rect_union(a) {
    var newLeft = Math.min(a.left, this.left);
    var newTop = Math.min(a.top, this.top);
    var newWidth = Math.max(a.right, this.right) - newLeft;
    var newHeight = Math.max(a.bottom, this.bottom) - newTop;
    var newRect = new Rect(newLeft, newTop, newWidth, newHeight);

    return newRect;
  },

  
  
  
  copy: function Rect_copy(a) {
    this.left = a.left;
    this.top = a.top;
    this.width = a.width;
    this.height = a.height;
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
  
  
  
  toString: function Range_toString() {
    return "[Range (" + this.min + "," + this.max + ")]";
  },

  
  
  get extent() {
    return (this.max - this.min);
  },

  set extent(extent) {
    this.max = extent - this.min;
  },

  
  
  
  
  
  
  contains: function Range_contains(value) {
    if (Utils.isNumber(value))
      return value >= this.min && value <= this.max;
    if (Utils.isRange(value))
      return value.min >= this.min && value.max <= this.max;
    return false;
  },

  
  
  
  
  
  
  overlaps: function Rect_overlaps(value) {
    if (Utils.isNumber(value))
      return this.contains(value);
    if (Utils.isRange(value))
      return !(value.max < this.min || this.max < value.min);
    return false;
  },

  
  
  
  
  
  
  
  
  proportion: function Range_proportion(value, smooth) {
    if (value <= this.min)
      return 0;
    if (this.max <= value)
      return 1;

    var proportion = (value - this.min) / this.extent;

    if (smooth) {
      
      
      
      
      function tanh(x) {
        var e = Math.exp(x);
        return (e - 1/e) / (e + 1/e);
      }
      return .5 - .5 * tanh(2 - 4 * proportion);
    }

    return proportion;
  },

  
  
  
  
  
  
  scale: function Range_scale(value) {
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
  
  
  
  
  addSubscriber: function Subscribable_addSubscriber(refObject, eventName, callback) {
    try {
      Utils.assertThrow(refObject, "refObject");
      Utils.assertThrow(typeof callback == "function", "callback must be a function");
      Utils.assertThrow(eventName && typeof eventName == "string",
          "eventName must be a non-empty string");
    } catch(e) {
      Utils.log(e);
      return;
    }

    if (!this.subscribers)
      this.subscribers = {};

    if (!this.subscribers[eventName])
      this.subscribers[eventName] = [];

    var subs = this.subscribers[eventName];
    var existing = subs.filter(function(element) {
      return element.refObject == refObject;
    });

    if (existing.length) {
      Utils.assert(existing.length == 1, 'should only ever be one');
      existing[0].callback = callback;
    } else {
      subs.push({
        refObject: refObject,
        callback: callback
      });
    }
  },

  
  
  
  removeSubscriber: function Subscribable_removeSubscriber(refObject, eventName) {
    try {
      Utils.assertThrow(refObject, "refObject");
      Utils.assertThrow(eventName && typeof eventName == "string",
          "eventName must be a non-empty string");
    } catch(e) {
      Utils.log(e);
      return;
    }

    if (!this.subscribers || !this.subscribers[eventName])
      return;

    this.subscribers[eventName] = this.subscribers[eventName].filter(function(element) {
      return element.refObject != refObject;
    });
  },

  
  
  
  _sendToSubscribers: function Subscribable__sendToSubscribers(eventName, eventInfo) {
    try {
      Utils.assertThrow(eventName && typeof eventName == "string",
          "eventName must be a non-empty string");
    } catch(e) {
      Utils.log(e);
      return;
    }

    if (!this.subscribers || !this.subscribers[eventName])
      return;

    var subsCopy = this.subscribers[eventName].concat();
    subsCopy.forEach(function(object) {
      try {
        object.callback(this, eventInfo);
      } catch(e) {
        Utils.log(e);
      }
    }, this);
  }
};




let Utils = {
  
  
  
  toString: function Utils_toString() {
    return "[Utils]";
  },

  
  useConsole: true, 
  showTime: false,

  
  
  
  
  log: function Utils_log() {
    var text = this.expandArgumentsForLog(arguments);
    var prefix = this.showTime ? Date.now() + ': ' : '';
    if (this.useConsole)    
      Services.console.logStringMessage(prefix + text);
    else
      dump(prefix + text + '\n');
  },

  
  
  
  
  error: function Utils_error() {
    var text = this.expandArgumentsForLog(arguments);
    var prefix = this.showTime ? Date.now() + ': ' : '';
    if (this.useConsole)    
      Cu.reportError(prefix + "tabview error: " + text);
    else
      dump(prefix + "TABVIEW ERROR: " + text + '\n');
  },

  
  
  
  
  
  trace: function Utils_trace() {
    var text = this.expandArgumentsForLog(arguments);
    
    let stack = Error().stack.replace(/^.*?\n.*?\n/, "");
    
    if (this.trace.caller.name == 'Utils_assert')
      stack = stack.replace(/^.*?\n/, "");
    this.log('trace: ' + text + '\n' + stack);
  },

  
  
  
  assert: function Utils_assert(condition, label) {
    if (!condition) {
      let text;
      if (typeof label != 'string')
        text = 'badly formed assert';
      else
        text = "tabview assert: " + label;

      this.trace(text);
    }
  },

  
  
  
  assertThrow: function Utils_assertThrow(condition, label) {
    if (!condition) {
      let text;
      if (typeof label != 'string')
        text = 'badly formed assert';
      else
        text = "tabview assert: " + label;

      
      text += Error().stack.replace(/^.*?\n.*?\n/, "");

      throw text;
    }
  },

  
  
  
  expandObject: function Utils_expandObject(obj) {
    var s = obj + ' = {';
    for (let prop in obj) {
      let value;
      try {
        value = obj[prop];
      } catch(e) {
        value = '[!!error retrieving property]';
      }

      s += prop + ': ';
      if (typeof value == 'string')
        s += '\'' + value + '\'';
      else if (typeof value == 'function')
        s += 'function';
      else
        s += value;

      s += ', ';
    }
    return s + '}';
  },

  
  
  
  expandArgumentsForLog: function Utils_expandArgumentsForLog(args) {
    var that = this;
    return Array.map(args, function(arg) {
      return typeof arg == 'object' ? that.expandObject(arg) : arg;
    }).join('; ');
  },

  

  
  
  
  isLeftClick: function Utils_isLeftClick(event) {
    return event.button == 0;
  },

  
  
  
  isMiddleClick: function Utils_isMiddleClick(event) {
    return event.button == 1;
  },

  
  
  
  isRightClick: function Utils_isRightClick(event) {
    return event.button == 2;
  },

  
  
  
  isDOMElement: function Utils_isDOMElement(object) {
    return object instanceof Ci.nsIDOMElement;
  },

  
  
  
  isNumber: function Utils_isNumber(n) {
    return typeof n == 'number' && !isNaN(n);
  },

  
  
  
  isRect: function Utils_isRect(r) {
    return (r &&
            this.isNumber(r.left) &&
            this.isNumber(r.top) &&
            this.isNumber(r.width) &&
            this.isNumber(r.height));
  },

  
  
  
  isRange: function Utils_isRange(r) {
    return (r &&
            this.isNumber(r.min) &&
            this.isNumber(r.max));
  },

  
  
  
  isPoint: function Utils_isPoint(p) {
    return (p && this.isNumber(p.x) && this.isNumber(p.y));
  },

  
  
  
  isPlainObject: function Utils_isPlainObject(obj) {
    
    
    if (!obj || Object.prototype.toString.call(obj) !== "[object Object]" ||
       obj.nodeType || obj.setInterval) {
      return false;
    }

    
    const hasOwnProperty = Object.prototype.hasOwnProperty;

    if (obj.constructor &&
       !hasOwnProperty.call(obj, "constructor") &&
       !hasOwnProperty.call(obj.constructor.prototype, "isPrototypeOf")) {
      return false;
    }

    
    

    var key;
    for (key in obj) {}

    return key === undefined || hasOwnProperty.call(obj, key);
  },

  
  
  
  isEmptyObject: function Utils_isEmptyObject(obj) {
    for (let name in obj)
      return false;
    return true;
  },

  
  
  
  
  copy: function Utils_copy(value) {
    if (value && typeof value == 'object') {
      if (Array.isArray(value))
        return this.extend([], value);
      return this.extend({}, value);
    }
    return value;
  },

  
  
  
  merge: function Utils_merge(first, second) {
    Array.forEach(second, function(el) Array.push(first, el));
    return first;
  },

  
  
  
  extend: function Utils_extend() {

    
    let target = arguments[0] || {};
    
    if (typeof target === "boolean") {
      this.assert(false, "The first argument of extend cannot be a boolean." +
          "Deep copy is not supported.");
      return target;
    }

    
    
    let length = arguments.length;
    if (length === 1) {
      this.assert(false, "Extending the iQ prototype using extend is not supported.");
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
