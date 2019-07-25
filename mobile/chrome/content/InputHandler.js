










































function getScrollboxFromElement(elem) {
  
  let scrollbox = null;

  while (elem.parentNode) {
    try {
      if ("scrollBoxObject" in elem && elem.scrollBoxObject) {
        scrollbox = elem.scrollBoxObject;
        break;
      }
      else {
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

  let stack = document.getElementById("browser-container");
  stack.addEventListener("DOMMouseScroll", this, true);

  
  
  stack.addEventListener("mousedown", this, true);
  stack.addEventListener("mouseup", this, true);
  stack.addEventListener("mousemove", this, true);
  stack.addEventListener("click", this, true);

  let browserCanvas = document.getElementById("browser-canvas");
  browserCanvas.addEventListener("keydown", this, true);
  browserCanvas.addEventListener("keyup", this, true);

  let prefsvc = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch2);
  let allowKinetic = prefsvc.getBoolPref("browser.ui.panning.kinetic");

  this._modules.push(new ChromeInputModule(this, browserCanvas));
  this._modules.push(new ContentPanningModule(this, browserCanvas, allowKinetic));
  this._modules.push(new ContentClickingModule(this));
  this._modules.push(new ScrollwheelModule(this));
}

InputHandler.prototype = {
  _modules : [],
  _grabbed : null,
  _ignoreEvents: false,

  grab: function grab(obj) {
    this._grabbed = obj;

    for each(mod in this._modules) {
      if (mod != obj)
        mod.cancelPending();
    }
    
    
  },

  ungrab: function ungrab(obj) {
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






function ChromeInputModule(owner, browserCanvas, useKinetic) {
  this._owner = owner;
  if (useKinetic !== undefined)
    this._dragData.useKinetic = useKinetic;
  this._browserCanvas = browserCanvas;
}

ChromeInputModule.prototype = {
  _owner: null,
  _ignoreNextClick: false,

  _dragData: {
    dragging: false,
    sX: 0,
    sY: 0,
    dragStartTimeout: -1,
    targetScrollbox: null,

    reset: function reset() {
      this.dragging = false;
      this.sX = 0;
      this.sY = 0;
      if (this.dragStartTimeout != -1)
        clearTimeout(this.dragStartTimeout);
      this.dragStartTimeout = -1;
      this.targetScrollbox = null;
    }
  },

  _clickData: {
    _events : [],
    reset: function reset() {
      this._events = [];
    }
  },

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
  },

  _dragStart: function _dragStart(sX, sY) {
    let dragData = this._dragData;
    dragData.dragging = true;
    dragData.dragStartTimeout = -1;

    
    ws.dragStart(sX, sY);

    
    this._clickData.reset();
  },

  _dragStop: function _dragStop(sX, sY) {
    let dragData = this._dragData;

    if (dragData.targetScrollbox)
      dragData.targetScrollbox.scrollBy(dragData.sX - sX, dragData.sY - sY);
  },

  _dragMove: function _dragMove(sX, sY) {
    let dragData = this._dragData;
    if (dragData.targetScrollbox)
      dragData.targetScrollbox.scrollBy(dragData.sX - sX, dragData.sY - sY);
  },

  _onMouseDown: function _onMouseDown(aEvent) {
    
    if (aEvent.target === this._browserCanvas) {
      return;
    }

    let dragData = this._dragData;

    dragData.targetScrollbox = getScrollboxFromElement(aEvent.target);
    if (!dragData.targetScrollbox)
      return;

    
    this._owner.grab(this);

    aEvent.stopPropagation();
    aEvent.preventDefault();

    dragData.sX = aEvent.screenX;
    dragData.sY = aEvent.screenY;

    dragData.dragStartTimeout = setTimeout(function(self, sX, sY) { self._dragStart(sX, sY); },
                                  200, this, aEvent.screenX, aEvent.screenY);

    
    let clickData = this._clickData;
    let clickEvent = document.createEvent("MouseEvent");
    clickEvent.initMouseEvent(aEvent.type, aEvent.bubbles, aEvent.cancelable,
                              aEvent.view, aEvent.detail,
                              aEvent.screenX, aEvent.screenY, aEvent.clientX, aEvent.clientY,
                              aEvent.ctrlKey, aEvent.altKey, aEvent.shiftKeyArg, aEvent.metaKeyArg,
                              aEvent.button, aEvent.relatedTarget);
    clickData._events.push({event: clickEvent, target: aEvent.target, time: Date.now()});
  },

  _onMouseUp: function _onMouseUp(aEvent) {
    
    let dragData = this._dragData;
    if (!dragData.targetScrollbox)
      return;

    let clickData = this._clickData;

    
    if (!(clickData._events.length % 2)) {
      clickData.reset();
    }
    else {
      let clickEvent = document.createEvent("MouseEvent");
      clickEvent.initMouseEvent(aEvent.type, aEvent.bubbles, aEvent.cancelable,
                                aEvent.view, aEvent.detail,
                                aEvent.screenX, aEvent.screenY, aEvent.clientX, aEvent.clientY,
                                aEvent.ctrlKey, aEvent.altKey, aEvent.shiftKeyArg, aEvent.metaKeyArg,
                                aEvent.button, aEvent.relatedTarget);
      clickData._events.push({event: clickEvent, target: aEvent.target, time: Date.now()});

      this._ignoreNextClick = true;
      this._sendSingleClick();
    }

    aEvent.stopPropagation();
    aEvent.preventDefault();

    if (dragData.dragging)
      this._dragStop(aEvent.screenX, aEvent.screenY);

    dragData.reset(); 
    this._owner.ungrab(this);
  },

  _onMouseMove: function _onMouseMove(aEvent) {
    let dragData = this._dragData;

    
    if (!dragData.targetScrollbox)
      return;

    aEvent.stopPropagation();
    aEvent.preventDefault();

    let dx = dragData.sX - aEvent.screenX;
    let dy = dragData.sY - aEvent.screenY;

    if (!dragData.dragging && dragData.dragStartTimeout != -1) {
      if ((Math.abs(dx*dx) + Math.abs(dy*dy)) > 100) {
        clearTimeout(dragData.dragStartTimeout);
        this._dragStart(aEvent.screenX, aEvent.screenY);
      }
    }
    if (!dragData.dragging)
      return;

    this._dragMove(aEvent.screenX, aEvent.screenY);

    dragData.sX = aEvent.screenX;
    dragData.sY = aEvent.screenY;
  },


  
  _sendSingleClick: function _sendSingleClick() {
    let clickData = this._clickData;

    this._owner.grab(this);
    this._owner.stopListening();

    
    this._redispatchChromeMouseEvent(clickData._events[0].event);
    this._redispatchChromeMouseEvent(clickData._events[1].event);

    this._owner.startListening();
    this._owner.ungrab(this);

    clickData.reset();
  },

  _redispatchChromeMouseEvent: function _redispatchChromeMouseEvent(aEvent) {
    if (!(aEvent instanceof MouseEvent)) {
      Cu.reportError("_redispatchChromeMouseEvent called with a non-mouse event");
      return;
    }

    var cwu = window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);

    
    cwu.sendMouseEvent(aEvent.type, aEvent.clientX, aEvent.clientY,
                       aEvent.button, aEvent.detail, 0, true);
  }
};





function ContentPanningModule(owner, browserCanvas, useKinetic) {
  this._owner = owner;
  if (useKinetic !== undefined)
    this._dragData.useKinetic = useKinetic;
  this._browserCanvas = browserCanvas;
}

ContentPanningModule.prototype = {
  _owner: null,
  _dragData: {
    dragging: false,
    sX: 0,
    sY: 0,
    useKinetic: true,
    dragStartTimeout: -1,

    reset: function reset() {
      this.dragging = false;
      this.sX = 0;
      this.sY = 0;
      if (this.dragStartTimeout != -1)
        clearTimeout(this.dragStartTimeout);
      this.dragStartTimeout = -1;
    }
  },

  _kineticData: {
    
    kineticStepSize: 15,
    kineticDecelloration: 0.004,
    momentumBufferSize: 3,

    momentumBuffer: [],
    momentumBufferIndex: 0,
    lastTime: 0,
    kineticDuration: 0,
    kineticDirX: 0,
    kineticDirY: 0,
    kineticHandle : -1,
    kineticStep : 0,
    kineticStartX : 0,
    kineticStartY : 0,
    kineticInitialVel: 0,

    reset: function reset() {
      if (this.kineticHandle != -1) {
        window.clearInterval(this.kineticHandle);
        this.kineticHandle = -1;
      }

      this.momentumBuffer = [];
      this.momentumBufferIndex = 0;
      this.lastTime = 0;
      this.kineticDuration = 0;
      this.kineticDirX = 0;
      this.kineticDirY = 0;
      this.kineticStep  = 0;
      this.kineticStartX  = 0;
      this.kineticStartY  = 0;
      this.kineticInitialVel = 0;
    }
  },

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
    this._dragData.reset();
    
  },

  _dragStart: function _dragStart(sX, sY) {
    let dragData = this._dragData;
    dragData.dragging = true;
    dragData.dragStartTimeout = -1;

    
    this._owner.grab(this);
    ws.dragStart(sX, sY);

    Browser.canvasBrowser.startPanning();

    
    this._kineticData.lastTime = Date.now();
  },

  _dragStop: function _dragStop(sX, sY) {
    let dragData = this._dragData;

    this._owner.ungrab(this);

    if (dragData.useKinetic) {
      
      if (!this._startKinetic(sX, sY))
        this._endKinetic(sX, sY);
    }
    else {
      ws.dragStop(sX, sY);
    }

    
    Browser.canvasBrowser.endPanning();
  },

  _dragMove: function _dragMove(sX, sY) {
    let dragData = this._dragData;
    ws.dragMove(sX, sY);
  },

  _onMouseDown: function _onMouseDown(aEvent) {
    
    if (this._kineticData.kineticHandle != -1)
      this._endKinetic(aEvent.screenX, aEvent.screenY);

    let dragData = this._dragData;

    dragData.sX = aEvent.screenX;
    dragData.sY = aEvent.screenY;

    dragData.dragStartTimeout = setTimeout(function(self, sX, sY) { self._dragStart(sX, sY); },
                                           200, this, aEvent.screenX, aEvent.screenY);
  },

  _onMouseUp: function _onMouseUp(aEvent) {
    let dragData = this._dragData;

    if (dragData.dragging)
      this._dragStop(aEvent.screenX, aEvent.screenY);

    dragData.reset(); 
  },

  _onMouseMove: function _onMouseMove(aEvent) {
    
    if (this._kineticData.kineticHandle != -1)
      return;

    let dragData = this._dragData;

    let dx = dragData.sX - aEvent.screenX;
    let dy = dragData.sY - aEvent.screenY;

    if (!dragData.dragging && dragData.dragStartTimeout != -1) {
      if ((Math.abs(dx*dx) + Math.abs(dy*dy)) > 100) {
        clearTimeout(dragData.dragStartTimeout);
        this._dragStart(aEvent.screenX, aEvent.screenY);
      }
    }
    if (!dragData.dragging)
      return;

    this._dragMove(aEvent.screenX, aEvent.screenY);

    dragData.sX = aEvent.screenX;
    dragData.sY = aEvent.screenY;

    if (dragData.useKinetic) {
      
      let kineticData = this._kineticData;
      let t = Date.now();
      let dt = t - kineticData.lastTime;
      kineticData.lastTime = t;
      let momentumBuffer = { dx: -dx, dy: -dy, dt: dt };

      kineticData.momentumBuffer[kineticData.momentumBufferIndex] = momentumBuffer;
      kineticData.momentumBufferIndex++;
      kineticData.momentumBufferIndex %= kineticData.momentumBufferSize;
    }
  },

  _startKinetic: function _startKinetic(sX, sY) {
    let kineticData = this._kineticData;

    let dx = 0;
    let dy = 0;
    let dt = 0;
    if (kineticData.kineticInitialVel)
      return true;

    if (!kineticData.momentumBuffer)
      return false;

    for (let i = 0; i < kineticData.momentumBufferSize; i++) {
      let me = kineticData.momentumBuffer[(kineticData.momentumBufferIndex + i) % kineticData.momentumBufferSize];
      if (!me)
        return false;

      dx += me.dx;
      dy += me.dy;
      dt += me.dt;
    }
    if (dt <= 0)
      return false;

    let dist = Math.sqrt(dx*dx+dy*dy);
    let vel  = dist/dt;
    if (vel < 1)
      return false;

    kineticData.kineticDirX = dx/dist;
    kineticData.kineticDirY = dy/dist;
    if (kineticData.kineticDirX > 0.9) {
      kineticData.kineticDirX = 1;
      kineticData.kineticDirY = 0;
    }
    else if (kineticData.kineticDirY < -0.9) {
      kineticData.kineticDirX = 0;
      kineticData.kineticDirY = -1;
    }
    else if (kineticData.kineticDirX < -0.9) {
      kineticData.kineticDirX = -1;
      kineticData.kineticDirY = 0;
    }
    else if (kineticData.kineticDirY > 0.9) {
      kineticData.kineticDirX = 0;
      kineticData.kineticDirY = 1;
    }

    kineticData.kineticDuration = vel/(2 * kineticData.kineticDecelloration);
    kineticData.kineticStep = 0;
    kineticData.kineticStartX =  sX;
    kineticData.kineticStartY =  sY;
    kineticData.kineticInitialVel = vel;
    kineticData.kineticHandle = window.setInterval(this._doKinetic, kineticData.kineticStepSize, this);
    return true;
  },

  _doKinetic: function _doKinetic(self) {
    let kineticData = self._kineticData;

    let t = kineticData.kineticStep * kineticData.kineticStepSize;
    kineticData.kineticStep++;
    if (t > kineticData.kineticDuration)
      t = kineticData.kineticDuration;
    let dist = kineticData.kineticInitialVel * t -
               kineticData.kineticDecelloration * t * t;
    let newX = Math.floor(kineticData.kineticDirX * dist + kineticData.kineticStartX);
    let newY = Math.floor(kineticData.kineticDirY * dist + kineticData.kineticStartY);

    self._dragMove(newX, newY);

    if(t >= kineticData.kineticDuration)
      self._endKinetic(newX, newY);
  },

  _endKinetic: function _endKinetic(sX, sY) {
    ws.dragStop(sX, sY);
    this._owner.ungrab(this);
    this._dragData.reset();
    this._kineticData.reset();

    
    
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
  }
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
          
          clearTimeout(this._clickTimeout);
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
          this._clickTimeout = setTimeout(function(self) { self._sendSingleClick(); }, 400, this);
        }
        else {
          clearTimeout(this._clickTimeout);
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
      clearTimeout(this._clickTimeout);
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
