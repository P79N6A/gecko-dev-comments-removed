








































let Ci = Components.interfaces;


const kViewportMinScale  = 0;
const kViewportMaxScale  = 10;
const kViewportMinWidth  = 200;
const kViewportMaxWidth  = 10000;
const kViewportMinHeight = 223;
const kViewportMaxHeight = 10000;





let Util = {
  bind: function bind(f, thisObj) {
    return function() {
      return f.apply(thisObj, arguments);
    };
  },

  bindAll: function bindAll(instance) {
    let bind = Util.bind;
    for (let key in instance)
      if (instance[key] instanceof Function)
        instance[key] = bind(instance[key], instance);
  },

  
  dumpf: function dumpf(str) {
    var args = arguments;
    var i = 1;
    dump(str.replace(/%s/g, function() {
      if (i >= args.length) {
        throw "dumps received too many placeholders and not enough arguments";
      }
      return args[i++].toString();
    }));
  },

  
  dumpLn: function dumpLn() {
    for (var i = 0; i < arguments.length; i++) { dump(arguments[i] + " "); }
    dump("\n");
  },

  getWindowUtils: function getWindowUtils(aWindow) {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
  },

  getScrollOffset: function getScrollOffset(aWindow) {
    var cwu = Util.getWindowUtils(aWindow);
    var scrollX = {};
    var scrollY = {};
    cwu.getScrollXY(false, scrollX, scrollY);
    return new Point(scrollX.value, scrollY.value);
  },

  
  executeSoon: function executeSoon(aFunc) {
    let tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
    tm.mainThread.dispatch({
      run: function() {
        aFunc();
      }
    }, Ci.nsIThread.DISPATCH_NORMAL);
  },

  getHrefForElement: function getHrefForElement(target) {
    
    
    

    let link = null;
    while (target) {
      if (target instanceof HTMLAnchorElement || 
          target instanceof HTMLAreaElement ||
          target instanceof HTMLLinkElement) {
          if (target.hasAttribute("href"))
            link = target;
      }
      target = target.parentNode;
    }

    if (link && link.hasAttribute("href"))
      return link.href;
    else
      return null;
  },

  makeURLAbsolute: function makeURLAbsolute(base, url) {
    
    return makeURI(url, null, makeURI(base)).spec;
  },

  clamp: function(num, min, max) {
    return Math.max(min, Math.min(max, num));
  },

  







  needHomepageOverride: function needHomepageOverride() {
    let savedmstone = null;
    try {
      savedmstone = gPrefService.getCharPref("browser.startup.homepage_override.mstone");
    } catch (e) {}

    if (savedmstone == "ignore")
      return "none";

#expand    let ourmstone = "__MOZ_APP_VERSION__";

    if (ourmstone != savedmstone) {
      gPrefService.setCharPref("browser.startup.homepage_override.mstone", ourmstone);

      return (savedmstone ? "new version" : "new profile");
    }

    return "none";
  },

  
  isURLEmpty: function isURLEmpty(aURL) {
    return (!aURL || aURL == "about:blank" || aURL == "about:home");
  },

  
  getAllDocuments: function getAllDocuments(doc, resultSoFar) {
    resultSoFar = resultSoFar || [doc];
    if (!doc.defaultView)
      return resultSoFar;
    let frames = doc.defaultView.frames;
    if (!frames)
      return resultSoFar;

    let i;
    let currentDoc;
    for (i = 0; i < frames.length; i++) {
      currentDoc = frames[i].document;
      resultSoFar.push(currentDoc);
      this.getAllDocuments(currentDoc, resultSoFar);
    }

    return resultSoFar;
  },

  
  
  forceOnline: function forceOnline() {
#ifdef MOZ_ENABLE_LIBCONIC
    gIOService.offline = false;
#endif
  },

  
  capitalize: function(str) {
    return str.charAt(0).toUpperCase() + str.substring(1);
  },
  
  isPortrait: function isPortrait() {
    return (window.innerWidth < 500);
  }
};






Util.Timeout = function(aCallback) {
  this._callback = aCallback;
  this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this._active = false;
}

Util.Timeout.prototype = {
  
  notify: function notify() {
    this._active = false;
    if (this._callback.notify)
      this._callback.notify();
    else
      this._callback.apply(null);
  },

  
  once: function once(aDelay, aCallback) {
    if (aCallback)
      this._callback = aCallback;
    this.clear();
    this._timer.initWithCallback(this, aDelay, this._timer.TYPE_ONE_SHOT);
    this._active = true;
    return this;
  },

  
  interval: function interval(aDelay, aCallback) {
    if (aCallback)
      this._callback = aCallback;
    this.clear();
    this._timer.initWithCallback(this, aDelay, this._timer.TYPE_REPEATING_SLACK);
    this._active = true;
    return this;
  },

  
  clear: function clear() {
    if (this._active) {
      this._timer.cancel();
      this._active = false;
    }
    return this;
  },

  
  flush: function flush() {
    if (this._active) {
      this.clear();
      this.notify();
    }
    return this;
  },

  
  isPending: function isPending() {
    return this._active;
  }
};






let Elements = {};

[
  ["browserBundle",      "bundle_browser"],
  ["contentShowing",     "bcast_contentShowing"],
  ["stack",              "stack"],
  ["panelUI",            "panel-container"],
  ["viewBuffer",         "view-buffer"],
  ["toolbarContainer",   "toolbar-container"],
].forEach(function (elementGlobal) {
  let [name, id] = elementGlobal;
  Elements.__defineGetter__(name, function () {
    let element = document.getElementById(id);
    if (!element)
      return null;
    delete Elements[name];
    return Elements[name] = element;
  });
});







function Point(x, y) {
  this.set(x, y);
}

Point.prototype = {
  clone: function clone() {
    return new Point(this.x, this.y);
  },

  set: function set(x, y) {
    this.x = x;
    this.y = y;
    return this;
  },
  
  equals: function equals(x, y) {
    return this.x == x && this.y == y;
  },

  toString: function toString() {
    return "(" + this.x + "," + this.y + ")";
  },

  map: function map(f) {
    this.x = f.call(this, this.x);
    this.y = f.call(this, this.y);
    return this;
  },

  add: function add(x, y) {
    this.x += x;
    this.y += y;
    return this;
  },

  subtract: function subtract(x, y) {
    this.x -= x;
    this.y -= y;
    return this;
  },

  scale: function scale(s) {
    this.x *= s;
    this.y *= s;
    return this;
  },

  isZero: function() {
    return this.x == 0 && this.y == 0;
  }
};

(function() {
  function takePointOrArgs(f) {
    return function(arg1, arg2) {
      if (arg2 === undefined)
        return f.call(this, arg1.x, arg1.y);
      else
        return f.call(this, arg1, arg2);
    };
  }

  for each (let f in ['add', 'subtract', 'equals', 'set'])
    Point.prototype[f] = takePointOrArgs(Point.prototype[f]);
})();










function Rect(x, y, w, h) {
  this.left = x;
  this.top = y;
  this.right = x + w;
  this.bottom = y + h;
};

Rect.fromRect = function fromRect(r) {
  return new Rect(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

Rect.prototype = {
  get x() { return this.left; },
  get y() { return this.top; },
  get width() { return this.right - this.left; },
  get height() { return this.bottom - this.top; },
  set x(v) {
    let diff = this.left - v;
    this.left = v;
    this.right -= diff;
  },
  set y(v) {
    let diff = this.top - v;
    this.top = v;
    this.bottom -= diff;
  },
  set width(v) { this.right = this.left + v; },
  set height(v) { this.bottom = this.top + v; },

  isEmpty: function isEmpty() {
    return this.left >= this.right || this.top >= this.bottom;
  },

  setRect: function(x, y, w, h) {
    this.left = x;
    this.top = y;
    this.right = x+w;
    this.bottom = y+h;

    return this;
  },

  setBounds: function(l, t, r, b) {
    this.top = t;
    this.left = l;
    this.bottom = b;
    this.right = r;

    return this;
  },

  equals: function equals(other) {
    return other != null &&
            (this.isEmpty() && other.isEmpty() ||
            this.top == other.top &&
            this.left == other.left &&
            this.bottom == other.bottom &&
            this.right == other.right);
  },

  clone: function clone() {
    return new Rect(this.left, this.top, this.right - this.left, this.bottom - this.top);
  },

  center: function center() {
    if (this.isEmpty())
      throw "Empty rectangles do not have centers";
    return new Point(this.left + (this.right - this.left) / 2,
                          this.top + (this.bottom - this.top) / 2);
  },

  copyFrom: function(other) {
    this.top = other.top;
    this.left = other.left;
    this.bottom = other.bottom;
    this.right = other.right;

    return this;
  },

  translate: function(x, y) {
    this.left += x;
    this.right += x;
    this.top += y;
    this.bottom += y;

    return this;
  },

  toString: function() {
    return "[" + this.x + "," + this.y + "," + this.width + "," + this.height + "]";
  },

  
  union: function(other) {
    return this.clone().expandToContain(other);
  },

  contains: function(other) {
    if (other.isEmpty()) return true;
    if (this.isEmpty()) return false;

    return (other.left >= this.left &&
            other.right <= this.right &&
            other.top >= this.top &&
            other.bottom <= this.bottom);
  },

  intersect: function(other) {
    return this.clone().restrictTo(other);
  },

  intersects: function(other) {
    if (this.isEmpty() || other.isEmpty())
      return false;

    let x1 = Math.max(this.left, other.left);
    let x2 = Math.min(this.right, other.right);
    let y1 = Math.max(this.top, other.top);
    let y2 = Math.min(this.bottom, other.bottom);
    return x1 < x2 && y1 < y2;
  },

  
  restrictTo: function restrictTo(other) {
    if (this.isEmpty() || other.isEmpty())
      return this.setRect(0, 0, 0, 0);

    let x1 = Math.max(this.left, other.left);
    let x2 = Math.min(this.right, other.right);
    let y1 = Math.max(this.top, other.top);
    let y2 = Math.min(this.bottom, other.bottom);
    
    return this.setRect(x1, y1, Math.max(0, x2 - x1), Math.max(0, y2 - y1));
  },

  
  expandToContain: function expandToContain(other) {
    if (this.isEmpty()) return this.copyFrom(other);
    if (other.isEmpty()) return this;

    let l = Math.min(this.left, other.left);
    let r = Math.max(this.right, other.right);
    let t = Math.min(this.top, other.top);
    let b = Math.max(this.bottom, other.bottom);
    return this.setRect(l, t, r-l, b-t);
  },

  



  expandToIntegers: function round() {
    this.left = Math.floor(this.left);
    this.top = Math.floor(this.top);
    this.right = Math.ceil(this.right);
    this.bottom = Math.ceil(this.bottom);
    return this;
  },

  scale: function scale(xscl, yscl) {
    this.left *= xscl;
    this.right *= xscl;
    this.top *= yscl;
    this.bottom *= yscl;
    return this;
  },

  map: function map(f) {
    this.left = f.call(this, this.left);
    this.top = f.call(this, this.top);
    this.right = f.call(this, this.right);
    this.bottom = f.call(this, this.bottom);
    return this;
  },

  
  translateInside: function translateInside(other) {
    let offsetX = (this.left < other.left ? other.left - this.left :
        (this.right > other.right ? other.right - this.right : 0));
    let offsetY = (this.top < other.top ? other.top - this.top :
        (this.bottom > other.bottom ? other.bottom - this.bottom : 0));
    return this.translate(offsetX, offsetY);
  },

  
  subtract: function subtract(other) {
    let r = new Rect(0, 0, 0, 0);
    let result = [];
    other = other.intersect(this);
    if (other.isEmpty())
      return [this.clone()];

    
    r.setBounds(this.left, this.top, other.left, this.bottom);
    if (!r.isEmpty())
      result.push(r.clone());
    
    r.setBounds(other.left, this.top, other.right, other.top);
    if (!r.isEmpty())
      result.push(r.clone());
    r.setBounds(other.left, other.bottom, other.right, this.bottom);
    if (!r.isEmpty())
      result.push(r.clone());
    
    r.setBounds(other.right, this.top, this.right, this.bottom);
    if (!r.isEmpty())
      result.push(r.clone());

    return result;
  },
};
