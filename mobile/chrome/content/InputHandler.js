










































function getScrollboxFromElement(elem) {
  
  let scrollbox = null;

  while (elem.parentNode) {
    try {
      if ("scrollBoxObject" in elem && elem.scrollBoxObject) {
        scrollbox = elem.scrollBoxObject;
        break;
      }
      else if (elem.boxObject) {
        scrollbox = elem.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
        break;
      }
    }
    catch (e) {
      
    }
    elem = elem.parentNode;
  }
  return scrollbox;
}












function InputHandler() {
  
  window.addEventListener("mouseout", this, true);

  
  window.addEventListener("mousedown", this, true);
  window.addEventListener("mouseup", this, true);
  window.addEventListener("mousemove", this, true);
  window.addEventListener("click", this, true);

  let stack = document.getElementById("browser-container");
  stack.addEventListener("DOMMouseScroll", this, true);

  let browserCanvas = document.getElementById("browser-canvas");
  browserCanvas.addEventListener("keydown", this, true);
  browserCanvas.addEventListener("keyup", this, true);

  let useEarlyMouseMoves = gPrefService.getBoolPref("browser.ui.panning.fixup.mousemove");

  this._modules.push(new ChromeInputModule(this, browserCanvas));
  this._modules.push(new ContentPanningModule(this, browserCanvas, useEarlyMouseMoves));
  this._modules.push(new ContentClickingModule(this));
  this._modules.push(new ScrollwheelModule(this));
}

InputHandler.prototype = {
  _modules : [],
  _grabbed : null,
  _ignoreEvents: false,

  grab: function grab(obj) {
    
    
    if ((obj == null) || (this._grabbed != obj)) {

      
      this._grabbed = obj;

      
      for each(mod in this._modules) {
        if (mod != obj)
          mod.cancelPending();
      }
    }
  },

  ungrab: function ungrab(obj) {
    if (this._grabbed == obj)
      this._grabbed = null;
  },

  startListening: function startListening() {
    this._ignoreEvents = false;
  },

  stopListening: function stopListening() {
    this._ignoreEvents = true;
  },

  handleEvent: function handleEvent(aEvent) {
    if (this._ignoreEvents)
      return;

    
    
    
    
    if (aEvent.type == "mouseout" && !aEvent.relatedTarget) {
      this.grab(null);
      return;
    }

    if (this._grabbed) {
      this._grabbed.handleEvent(aEvent);
    }
    else {
      for each(mod in this._modules) {
        mod.handleEvent(aEvent);
        
        if (this._grabbed)
          break;
      }
    }
  }
};





function DragData(owner, dragRadius, dragStartTimeoutLength) {
  this._owner = owner;
  this._dragRadius = dragRadius;
  this.reset();
}

DragData.prototype = {
  reset: function reset() {
    this.dragging = false;
    this.sX = null;
    this.sY = null;
    this.alreadyLocked = false;
    this.lockedX = null;
    this.lockedY = null;
    this._originX = null;
    this._originY = null;
  },

  setDragPosition: function setDragPosition(screenX, screenY) {
    this.sX = screenX;
    this.sY = screenY;
  },

  setDragStart: function setDragStart(screenX, screenY) {
    this.setDragPosition(screenX, screenY);
    this._originX = screenX;
    this._originY = screenY;
    this.dragging = true;
  },

  lockMouseMove: function lockMouseMove(sX, sY) {
    if (this.lockedX !== null)
      sX = this.lockedX;
    else if (this.lockedY !== null)
      sY = this.lockedY;
    return [sX, sY];
  },

  lockAxis: function lockAxis(sX, sY) {
    if (this.alreadyLocked)
      return this.lockMouseMove(sX, sY);

    
    
    let absX = Math.abs(this.sX - sX);
    let absY = Math.abs(this.sY - sY);

    
    
    if ((absX > (this._dragRadius / 2)) && ((absX * absX) > (2 * absY * absY))) {
      this.lockedY = this.sY;
      sY = this.sY;
    }
    else if ((absY > (this._dragRadius / 2)) && ((absY * absY) > (2 * absX * absX))) {
      this.lockedX = this.sX;
      sX = this.sX;
    }
    this.alreadyLocked = true;

    return [sX, sY];
  },

  isPointOutsideRadius: function isPointOutsideRadius(sX, sY) {
    if (this._originX == undefined)
      return false;
    return (Math.pow(sX - this._originX, 2) + Math.pow(sY - this._originY, 2)) >
      (2 * Math.pow(this._dragRadius, 2));
  }
};





function ChromeInputModule(owner, browserCanvas) {
  this._owner = owner;
  this._browserCanvas = browserCanvas;
  this._dragData = new DragData(this, 50, 200);
}

ChromeInputModule.prototype = {
  _owner: null,
  _ignoreNextClick: false,
  _dragData: null,
  _clickEvents : [],
  _targetScrollbox: null,

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "mousedown":
        this._onMouseDown(aEvent);
        break;
      case "mousemove":
        this._onMouseMove(aEvent);
        break;
      case "mouseup":
        this._onMouseUp(aEvent);
        break;
      case "click":
        if (this._ignoreNextClick) {
          aEvent.stopPropagation();
          aEvent.preventDefault();
          this._ignoreNextClick = false;
        }
        break;
    }
  },

  


  cancelPending: function cancelPending() {
    this._dragData.reset();
    this._targetScrollbox = null;
  },

  
  _dragStart: function _dragStart(sX, sY) {
    let dragData = this._dragData;

    dragData.setDragStart(sX, sY);

    [sX, sY] = dragData.lockAxis(sX, sY);

    
    ws.dragStart(sX, sY);
  },

  _dragStop: function _dragStop(sX, sY) {
    let dragData = this._dragData;
    [sX, sY] = dragData.lockMouseMove(sX, sY);
    if (this._targetScrollbox)
      this._targetScrollbox.scrollBy(dragData.sX - sX, dragData.sY - sY);
    this._targetScrollbox = null;
  },

  _dragMove: function _dragMove(sX, sY) {
    let dragData = this._dragData;
    if (dragData.isPointOutsideRadius(sX, sY))
      this._clickEvents = [];

    [sX, sY] = dragData.lockMouseMove(sX, sY);
    if (this._targetScrollbox)
      this._targetScrollbox.scrollBy(dragData.sX - sX, dragData.sY - sY);
    dragData.setDragPosition(sX, sY);
  },

  _onMouseDown: function _onMouseDown(aEvent) {
    
    if (aEvent.target === this._browserCanvas) {
      return;
    }

    let dragData = this._dragData;

    this._targetScrollbox = getScrollboxFromElement(aEvent.target);
    if (!this._targetScrollbox)
      return;

    
    this._owner.grab(this);

    aEvent.stopPropagation();
    aEvent.preventDefault();

    this._dragStart(aEvent.screenX, aEvent.screenY);
    this._onMouseMove(aEvent); 

    
    let clickEvent = document.createEvent("MouseEvent");
    clickEvent.initMouseEvent(aEvent.type, aEvent.bubbles, aEvent.cancelable,
                              aEvent.view, aEvent.detail,
                              aEvent.screenX, aEvent.screenY, aEvent.clientX, aEvent.clientY,
                              aEvent.ctrlKey, aEvent.altKey, aEvent.shiftKeyArg, aEvent.metaKeyArg,
                              aEvent.button, aEvent.relatedTarget);
    this._clickEvents.push({event: clickEvent, target: aEvent.target, time: Date.now()});
  },

  _onMouseUp: function _onMouseUp(aEvent) {
    
    let dragData = this._dragData;
    if (!this._targetScrollbox)
      return;

    
    if (!(this._clickEvents.length % 2)) {
      this._clickEvents = [];
    }
    else {
      let clickEvent = document.createEvent("MouseEvent");
      clickEvent.initMouseEvent(aEvent.type, aEvent.bubbles, aEvent.cancelable,
                                aEvent.view, aEvent.detail,
                                aEvent.screenX, aEvent.screenY, aEvent.clientX, aEvent.clientY,
                                aEvent.ctrlKey, aEvent.altKey, aEvent.shiftKeyArg, aEvent.metaKeyArg,
                                aEvent.button, aEvent.relatedTarget);
      this._clickEvents.push({event: clickEvent, target: aEvent.target, time: Date.now()});

      this._ignoreNextClick = true;
      this._sendSingleClick();
      this._targetScrollbox = null;
    }

    aEvent.stopPropagation();
    aEvent.preventDefault();

    if (dragData.dragging)
      this._dragStop(aEvent.screenX, aEvent.screenY);

    dragData.reset(); 
    this._targetScrollbox = null;
    this._owner.ungrab(this);
  },

  _onMouseMove: function _onMouseMove(aEvent) {
    let dragData = this._dragData;

    
    if (!this._targetScrollbox)
      return;

    aEvent.stopPropagation();
    aEvent.preventDefault();

    let sX = aEvent.screenX;
    let sY = aEvent.screenY;

    if (!dragData.sX)
      dragData.setDragPosition(aEvent.screenX, aEvent.screenY);

    [sX, sY] = dragData.lockMouseMove(aEvent.screenX, aEvent.screenY);

    if (!dragData.dragging)
      return;

    [sX, sY] = dragData.lockMouseMove(sX, sY);
    this._dragMove(sX, sY);
  },


  
  _sendSingleClick: function _sendSingleClick() {
    this._owner.grab(this);
    this._owner.stopListening();

    
    this._redispatchChromeMouseEvent(this._clickEvents[0].event);
    this._redispatchChromeMouseEvent(this._clickEvents[1].event);

    this._owner.startListening();
    this._owner.ungrab(this);

    this._clickEvents = [];
  },

  _redispatchChromeMouseEvent: function _redispatchChromeMouseEvent(aEvent) {
    if (!(aEvent instanceof MouseEvent)) {
      Cu.reportError("_redispatchChromeMouseEvent called with a non-mouse event");
      return;
    }

    
    let cwu = Browser.windowUtils;
    cwu.sendMouseEvent(aEvent.type, aEvent.clientX, aEvent.clientY,
                       aEvent.button, aEvent.detail, 0, true);
  }
};





function KineticData(owner) {
  this._owner = owner;
  this._kineticTimer = null;
  this.reset();
}

KineticData.prototype = {
   _updateInterval : 33, 

  reset: function reset() {
    if (this._kineticTimer != null) {
      this._kineticTimer.cancel();
      this._kineticTimer = null;
    }

    this.momentumBuffer = [];
    this._speedX = 0;
    this._speedY = 0;
  },

  _startKineticTimer: function _startKineticTimer() {
    let callback = {
      _self: this,
      notify: function(timer) {
        let self = this._self;

        const decelerationRate = 0.15;

        

        if (self._speedX == 0 && self._speedY == 0) {
          self.endKinetic();
          return;
        } else {
          let dx = Math.round(self._speedX * self._updateInterval);
          let dy = Math.round(self._speedY * self._updateInterval);
          

          let panned = self._owner._dragBy(dx, dy);
          if (!panned) {
            self.endKinetic();
            return;
          }
        }

        if (self._speedX < 0) {
          self._speedX = Math.min(self._speedX + decelerationRate, 0);
        } else if (self._speedX > 0) {
          self._speedX = Math.max(self._speedX - decelerationRate, 0);
        }
        if (self._speedY < 0) {
          self._speedY = Math.min(self._speedY + decelerationRate, 0);
        } else if (self._speedY > 0) {
          self._speedY = Math.max(self._speedY - decelerationRate, 0);
        }

        if (self._speedX == 0 && self._speedY == 0)
          self.endKinetic();
      }
    };

    this._kineticTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    
    this._kineticTimer.initWithCallback(callback,
                                        this._updateInterval,
                                        this._kineticTimer.TYPE_REPEATING_SLACK);
  },


  startKinetic: function startKinetic(sX, sY) {
    let mb = this.momentumBuffer;
    let mblen = this.momentumBuffer.length;

    
    if (mblen < 2)
      return false;

    let tempX = 0;
    let tempY = 0;

    
    let prev = mb[0];
    for (let i = 1; i < mblen; i++) {
      let me = mb[i];

      tempX += (me.sx - prev.sx) / (me.t - prev.t);
      tempY += (me.sy - prev.sy) / (me.t - prev.t);

      prev = me;
    }

    
    this.speedX = tempX / mblen;
    this.speedY = tempY / mblen;

    
    this._startKineticTimer();

    return true;
  },

  endKinetic: function endKinetic() {
    ws.dragStop();
    this.reset();

    
    
    let [leftVis,] = ws.getWidgetVisibility("tabs-container", false);
    let [rightVis,] = ws.getWidgetVisibility("browser-controls", false);
    if (leftVis != 0 && leftVis != 1) {
      let w = document.getElementById("tabs-container").getBoundingClientRect().width;
      if (leftVis >= 0.6666)
        ws.panBy(-w, 0, true);
      else
        ws.panBy(leftVis * w, 0, true);
    }
    else if (rightVis != 0 && rightVis != 1) {
      let w = document.getElementById("browser-controls").getBoundingClientRect().width;
      if (rightVis >= 0.6666)
        ws.panBy(w, 0, true);
      else
        ws.panBy(-rightVis * w, 0, true);
    }
  },

  addData: function addData(sx, sy) {
    let mbLength = this.momentumBuffer.length;
    
    if (mbLength > 0) {
      let mbLast = this.momentumBuffer[mbLength - 1];
      if (mbLast.sx == sx && mbLast.sy == sy)
	return;
    }

    this.momentumBuffer.push({'t': Date.now(), 'sx' : sx, 'sy' : sy});
  }
};

function ContentPanningModule(owner, browserCanvas, useEarlyMouseMoves) {
  this._owner = owner;
  this._browserCanvas = browserCanvas;
  this._dragData = new DragData(this, 50, 200);
  this._kineticData = new KineticData(this);
  this._useEarlyMouseMoves = useEarlyMouseMoves;
}

ContentPanningModule.prototype = {
  _owner: null,
  _dragData: null,

  _kineticData: null,

  handleEvent: function handleEvent(aEvent) {
    
    if (aEvent.target !== this._browserCanvas)
      return;

    switch (aEvent.type) {
      case "mousedown":
        this._onMouseDown(aEvent);
        break;
      case "mousemove":
        this._onMouseMove(aEvent);
        break;
      case "mouseup":
        this._onMouseUp(aEvent);
        break;
    }
  },


  


  cancelPending: function cancelPending() {
    let dragData = this._dragData;
    
    this._kineticData.endKinetic(dragData.sX, dragData.sY);
    this._owner.ungrab(this);
    dragData.reset();
  },

  _dragStart: function _dragStart(sX, sY) {
    let dragData = this._dragData;

    dragData.setDragStart(sX, sY);

    [sX, sY] = dragData.lockAxis(sX, sY);

    ws.dragStart(sX, sY);

    Browser.canvasBrowser.startPanning();
  },

  _dragStop: function _dragStop(sX, sY) {
    let dragData = this._dragData;

    this._owner.ungrab(this);

    [sX, sY] = dragData.lockMouseMove(sX, sY);

    
    if (!this._kineticData.startKinetic(sX, sY))
      this._kineticData.endKinetic(sX, sY);
    dragData.reset();
  },

  _dragBy: function _dragBy(dx, dy) {
    let panned = ws.dragBy(dx, dy);
    return panned;
  },

  _dragMove: function _dragMove(sX, sY) {
    let dragData = this._dragData;
    [sX, sY] = dragData.lockMouseMove(sX, sY);
    let panned = ws.dragMove(sX, sY);
    dragData.setDragPosition(sX, sY);
    return panned;
  },

  _onMouseDown: function _onMouseDown(aEvent) {
    let dragData = this._dragData;
    
    if (this._kineticData._kineticTimer != null) {
      this._kineticData.endKinetic(aEvent.screenX, aEvent.screenY);
      this._owner.ungrab(this);
      dragData.reset();
    }

    this._dragStart(aEvent.screenX, aEvent.screenY);
    this._onMouseMove(aEvent); 
  },

  _onMouseUp: function _onMouseUp(aEvent) {
    let dragData = this._dragData;

    if (dragData.dragging) {
      this._onMouseMove(aEvent); 
      this._dragStop(aEvent.screenX, aEvent.screenY);
    }

    dragData.reset(); 
  },

  _onMouseMove: function _onMouseMove(aEvent) {
    
    if (this._kineticData._kineticTimer != null)
      return;

    let dragData = this._dragData;

    
    if (dragData.isPointOutsideRadius(aEvent.screenX, aEvent.screenY))
      this._owner.grab(this);

    
    if (!dragData.sX)
      dragData.setDragPosition(aEvent.screenX, aEvent.screenY);

    let [sX, sY] = dragData.lockMouseMove(aEvent.screenX, aEvent.screenY);

    
    
    if (this._useEarlyMouseMoves || dragData.dragging)
      this._kineticData.addData(sX, sY);

    if (dragData.dragging)
      this._dragMove(sX, sY);
  },
};





function ContentClickingModule(owner) {
  this._owner = owner;
}


ContentClickingModule.prototype = {
  _clickTimeout : -1,
  _events : [],
  _zoomed : false,

  handleEvent: function handleEvent(aEvent) {
    
    if (aEvent.target !== document.getElementById("browser-canvas"))
      return;

    switch (aEvent.type) {
      
      case "mousedown":
        this._events.push({event: aEvent, time: Date.now()});

        
        if (this._clickTimeout != -1) {
          
          window.clearTimeout(this._clickTimeout);
          this.clickTimeout = -1;
        }
        break;
      case "mouseup":
        
        if (!(this._events.length % 2)) {
          this._reset();
          break;
        }

        this._events.push({event: aEvent, time: Date.now()});

        if (this._clickTimeout == -1) {
          this._clickTimeout = window.setTimeout(function(self) { self._sendSingleClick(); }, 400, this);
        }
        else {
          window.clearTimeout(this._clickTimeout);
          this.clickTimeout = -1;
          this._sendDoubleClick();
        }
        break;
    }
  },

  


  cancelPending: function cancelPending() {
    this._reset();
  },

  _reset: function _reset() {
    if (this._clickTimeout != -1)
      window.clearTimeout(this._clickTimeout);
    this._clickTimeout = -1;

    this._events = [];
  },

  _sendSingleClick: function _sendSingleClick() {
    this._owner.grab(this);
    this._dispatchContentMouseEvent(this._events[0].event);
    this._dispatchContentMouseEvent(this._events[1].event);
    this._owner.ungrab(this);

    this._reset();
  },

  _sendDoubleClick: function _sendDoubleClick() {
    this._owner.grab(this);

    function optimalElementForPoint(cX, cY) {
      var element = Browser.canvasBrowser.elementFromPoint(cX, cY);
      if (!element)
        return null;

      
      while (element.parentNode) {
        let display = window.getComputedStyle(element, "").getPropertyValue("display");
        let zoomable = /table/.test(display) || /block/.test(display);
        if (zoomable)
          break;

        element = element.parentNode;
      }
      return element;
    }

    let firstEvent = this._events[0].event;
    let zoomElement = optimalElementForPoint(firstEvent.clientX, firstEvent.clientY);

    if (zoomElement) {
      if (this._zoomed) {
        
        this._zoomed = false;
        Browser.canvasBrowser.zoomFromElement(zoomElement);
      }
      else {
        
        this._zoomed = true;
        Browser.canvasBrowser.zoomToElement(zoomElement);
      }

    }

    this._owner.ungrab(this);

    this._reset();
  },


  _dispatchContentMouseEvent: function _dispatchContentMouseEvent(aEvent, aType) {
    if (!(aEvent instanceof MouseEvent)) {
      Cu.reportError("_dispatchContentMouseEvent called with a non-mouse event");
      return;
    }

    let cb = Browser.canvasBrowser;
    var [x, y] = cb._clientToContentCoords(aEvent.clientX, aEvent.clientY);
    var cwu = cb.contentDOMWindowUtils;

    
    cwu.sendMouseEvent(aType || aEvent.type,
                       x, y,
                       aEvent.button || 0,
                       aEvent.detail || 1,
                       0, true);
  }
};





function ScrollwheelModule(owner) {
  this._owner = owner;
}

ScrollwheelModule.prototype = {
  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      
      case "DOMMouseScroll":
        this._owner.grab(this);
        Browser.canvasBrowser.zoom(aEvent.detail);
        this._owner.ungrab(this);
        break;
    }
  },

  


  cancelPending: function cancelPending() {
  }
};
