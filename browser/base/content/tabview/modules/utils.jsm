








































(function(){

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "gWindow", function() {
  let windows = Services.wm.getEnumerator("navigator:browser");
  while (windows.hasMoreElements()) {
    let browser = windows.getNext();
    let tabCandyFrame = browser.document.getElementById("tab-candy");
    if (tabCandyFrame.contentWindow == window)
      return browser;
  }
});

XPCOMUtils.defineLazyGetter(this, "gBrowser", function() gWindow.gBrowser);

XPCOMUtils.defineLazyGetter(this, "gTabViewDeck", function() {
  return gWindow.document.getElementById("tab-candy-deck");
});

XPCOMUtils.defineLazyGetter(this, "gTabViewFrame", function() {
  return gWindow.document.getElementById("tab-candy");
});

var consoleService = Cc["@mozilla.org/consoleservice;1"]
    .getService(Components.interfaces.nsIConsoleService);









window.Point = function(a, y) {
  if (Utils.isPoint(a)) {
    
    this.x = a.x;
    
    this.y = a.y;
  } else {
    this.x = (Utils.isNumber(a) ? a : 0);
    this.y = (Utils.isNumber(y) ? y : 0);
  }
};
window.Point.prototype = {
  
  
  
  distance: function(point) {
    var ax = this.x - point.x;
    var ay = this.y - point.y;
    return Math.sqrt((ax * ax) + (ay * ay));
  }
};










window.Rect = function(a, top, width, height) {
  
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

window.Rect.prototype = {
  
  
  get right() {
    return this.left + this.width;
  },

  
  set right(value) {
    this.width = value - this.left;
  },

  
  
  get bottom() {
    return this.top + this.height;
  },

  
  set bottom(value) {
    this.height = value - this.top;
  },

  
  
  
  get xRange() {
    return new Range(this.left,this.right);
  },

  
  
  
  get yRange() {
    return new Range(this.top,this.bottom);
  },

  
  
  
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
    return( rect.left > this.left
         && rect.right < this.right
         && rect.top > this.top
         && rect.bottom < this.bottom )
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







window.Range = function(min, max) {
  if (Utils.isRange(min) && !max) { 
    this.min = min.min;
    this.max = min.max;
  } else {
    this.min = min || 0;
    this.max = max || 0;
  }
};

window.Range.prototype = {
  
  
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
        ( value.min <= this.max && this.min <= value.max ) :
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




window.Subscribable = function() {
  this.subscribers = null;
};

window.Subscribable.prototype = {
  
  
  
  
  addSubscriber: function(refObject, eventName, callback) {
    try {
      Utils.assertThrow("refObject", refObject);
      Utils.assertThrow("callback must be a function", iQ.isFunction(callback));
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
      var subsCopy = iQ.merge([], this.subscribers[eventName]);
      subsCopy.forEach(function(object) {
        object.callback(self, eventInfo);
      });
    } catch(e) {
      Utils.log(e);
    }
  }
};




var Utils = {
  

  
  
  
  
  log: function() {
    var text = this.expandArgumentsForLog(arguments);
    consoleService.logStringMessage(text);
  },

  
  
  
  
  
  error: function() {
    var text = this.expandArgumentsForLog(arguments);
    Cu.reportError('tabcandy error: ' + text);
  },

  
  
  
  
  
  trace: function() {
    var text = this.expandArgumentsForLog(arguments);
    try { 
      throw new Error("error");
    } catch (e) {
      
      var stack = e.stack.replace(/^.*?\n.*?\n/,'');
      
      if (this.trace.caller.name == 'Utils_assert')
        stack = stack.replace(/^.*?\n/,'');
      this.log('trace: ' + text + '\n' + stack);
    }
  },

  
  
  
  assert: function Utils_assert(label, condition) {
    if (!condition) {
      var text;
      if (typeof(label) == 'undefined')
        text = 'badly formed assert';
      else
        text = 'tabcandy assert: ' + label;

      this.trace(text);
    }
  },

  
  
  
  assertThrow: function(label, condition) {
    if (!condition) {
      var text;
      if (typeof(label) == 'undefined')
        text = 'badly formed assert';
      else
        text = 'tabcandy assert: ' + label;

      try { 
        throw new Error("error");
      } catch (e) {
        
        text += e.stack.replace(/^.*?\n.*?\n/,'');
      }

      throw text;
    }
  },

  
  
  
  expandObject: function(obj) {
      var s = obj + ' = {';
      for (prop in obj) {
        var value;
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

        s += ", ";
      }
      return s + '}';
    },

  
  
  
  expandArgumentsForLog: function(args) {
    var s = '';
    var count = args.length;
    var a;
    for (a = 0; a < count; a++) {
      var arg = args[a];
      if (typeof(arg) == 'object')
        arg = this.expandObject(arg);

      s += arg;
      if (a < count - 1)
        s += '; ';
    }

    return s;
  },

  

  
  
  
  isRightClick: function(event) {
    if (event.which)
      return (event.which == 3);
    if (event.button)
      return (event.button == 2);
    return false;
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

  
  
  
  
  copy: function(value) {
    if (value && typeof(value) == 'object') {
      if (iQ.isArray(value))
        return iQ.extend([], value);

      return iQ.extend({}, value);
    }

    return value;
  }
};

window.Utils = Utils;

})();
