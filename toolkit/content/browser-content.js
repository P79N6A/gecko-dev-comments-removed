




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

var global = this;

let ClickEventHandler = {
  init: function init() {
    this._scrollable = null;
    this._scrolldir = "";
    this._startX = null;
    this._startY = null;
    this._screenX = null;
    this._screenY = null;
    this._lastFrame = null;

    Cc["@mozilla.org/eventlistenerservice;1"]
      .getService(Ci.nsIEventListenerService)
      .addSystemEventListener(global, "mousedown", this, true);

    addMessageListener("Autoscroll:Stop", this);
  },

  isAutoscrollBlocker: function(node) {
    let mmPaste = Services.prefs.getBoolPref("middlemouse.paste");
    let mmScrollbarPosition = Services.prefs.getBoolPref("middlemouse.scrollbarPosition");

    while (node) {
      if ((node instanceof content.HTMLAnchorElement || node instanceof content.HTMLAreaElement) &&
          node.hasAttribute("href")) {
        return true;
      }

      if (mmPaste && (node instanceof content.HTMLInputElement ||
                      node instanceof content.HTMLTextAreaElement)) {
        return true;
      }

      if (node instanceof content.XULElement && mmScrollbarPosition
          && (node.localName == "scrollbar" || node.localName == "scrollcorner")) {
        return true;
      }

      node = node.parentNode;
    }
    return false;
  },

  startScroll: function(event) {
    
    const scrollingAllowed = ['scroll', 'auto'];

    
    
    for (this._scrollable = event.originalTarget; this._scrollable;
         this._scrollable = this._scrollable.parentNode) {
      
      
      
      if (!(this._scrollable instanceof content.HTMLElement) ||
          ((this._scrollable instanceof content.HTMLSelectElement) && !this._scrollable.multiple)) {
        continue;
      }

      var overflowx = this._scrollable.ownerDocument.defaultView
                          .getComputedStyle(this._scrollable, '')
                          .getPropertyValue('overflow-x');
      var overflowy = this._scrollable.ownerDocument.defaultView
                          .getComputedStyle(this._scrollable, '')
                          .getPropertyValue('overflow-y');
      
      
      
      var scrollVert = this._scrollable.scrollTopMax &&
        (this._scrollable instanceof content.HTMLSelectElement ||
         scrollingAllowed.indexOf(overflowy) >= 0);

      
      
      if (!(this._scrollable instanceof content.HTMLSelectElement) &&
          this._scrollable.scrollLeftMax &&
          scrollingAllowed.indexOf(overflowx) >= 0) {
        this._scrolldir = scrollVert ? "NSEW" : "EW";
        break;
      } else if (scrollVert) {
        this._scrolldir = "NS";
        break;
      }
    }

    if (!this._scrollable) {
      this._scrollable = event.originalTarget.ownerDocument.defaultView;
      if (this._scrollable.scrollMaxX > 0) {
        this._scrolldir = this._scrollable.scrollMaxY > 0 ? "NSEW" : "EW";
      } else if (this._scrollable.scrollMaxY > 0) {
        this._scrolldir = "NS";
      } else {
        this._scrollable = null; 
        return;
      }
    }

    let [enabled] = sendSyncMessage("Autoscroll:Start",
                                    {scrolldir: this._scrolldir,
                                     screenX: event.screenX,
                                     screenY: event.screenY});
    if (!enabled) {
      this._scrollable = null;
      return;
    }

    Cc["@mozilla.org/eventlistenerservice;1"]
      .getService(Ci.nsIEventListenerService)
      .addSystemEventListener(global, "mousemove", this, true);
    addEventListener("pagehide", this, true);

    this._ignoreMouseEvents = true;
    this._startX = event.screenX;
    this._startY = event.screenY;
    this._screenX = event.screenX;
    this._screenY = event.screenY;
    this._scrollErrorX = 0;
    this._scrollErrorY = 0;
    this._lastFrame = content.mozAnimationStartTime;

    content.mozRequestAnimationFrame(this);
  },

  stopScroll: function() {
    if (this._scrollable) {
      this._scrollable = null;

      Cc["@mozilla.org/eventlistenerservice;1"]
        .getService(Ci.nsIEventListenerService)
        .removeSystemEventListener(global, "mousemove", this, true);
      removeEventListener("pagehide", this, true);
    }
  },

  accelerate: function(curr, start) {
    const speed = 12;
    var val = (curr - start) / speed;

    if (val > 1)
      return val * Math.sqrt(val) - 1;
    if (val < -1)
      return val * Math.sqrt(-val) + 1;
    return 0;
  },

  roundToZero: function(num) {
    if (num > 0)
      return Math.floor(num);
    return Math.ceil(num);
  },

  autoscrollLoop: function(timestamp) {
    if (!this._scrollable) {
      
      return;
    }

    
    
    const maxTimeDelta = 100;
    var timeDelta = Math.min(maxTimeDelta, timestamp - this._lastFrame);
    
    var timeCompensation = timeDelta / 20;
    this._lastFrame = timestamp;

    var actualScrollX = 0;
    var actualScrollY = 0;
    
    
    if (this._scrolldir != 'EW') {
      var y = this.accelerate(this._screenY, this._startY) * timeCompensation;
      var desiredScrollY = this._scrollErrorY + y;
      actualScrollY = this.roundToZero(desiredScrollY);
      this._scrollErrorY = (desiredScrollY - actualScrollY);
    }
    if (this._scrolldir != 'NS') {
      var x = this.accelerate(this._screenX, this._startX) * timeCompensation;
      var desiredScrollX = this._scrollErrorX + x;
      actualScrollX = this.roundToZero(desiredScrollX);
      this._scrollErrorX = (desiredScrollX - actualScrollX);
    }

    if (this._scrollable instanceof content.Window) {
      this._scrollable.scrollBy(actualScrollX, actualScrollY);
    } else { 
      this._scrollable.scrollLeft += actualScrollX;
      this._scrollable.scrollTop += actualScrollY;
    }
    content.mozRequestAnimationFrame(this);
  },

  sample: function(timestamp) {
    this.autoscrollLoop(timestamp);
  },

  handleEvent: function(event) {
    if (event.type == "mousemove") {
      this._screenX = event.screenX;
      this._screenY = event.screenY;
    } else if (event.type == "mousedown") {
      if (event.isTrusted &
          !event.defaultPrevented &&
          event.button == 1 &&
          !this._scrollable &&
          !this.isAutoscrollBlocker(event.originalTarget)) {
        this.startScroll(event);
      }
    } else if (event.type == "pagehide") {
      if (this._scrollable) {
        var doc =
          this._scrollable.ownerDocument || this._scrollable.document;
        if (doc == event.target) {
          sendAsyncMessage("Autoscroll:Cancel");
        }
      }
    }
  },

  receiveMessage: function(msg) {
    switch (msg.name) {
      case "Autoscroll:Stop": {
        this.stopScroll();
        break;
      }
    }
  },
};
ClickEventHandler.init();
