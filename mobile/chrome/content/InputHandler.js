










































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
  let stack = document.getElementById("browser-container");
  stack.addEventListener("DOMMouseScroll", this, true);

  
  stack.addEventListener("mouseout", this, true);
  stack.addEventListener("mousedown", this, true);
  stack.addEventListener("mouseup", this, true);
  stack.addEventListener("mousemove", this, true);

  let content = document.getElementById("browser-canvas");
  content.addEventListener("keydown", this, true);
  content.addEventListener("keyup", this, true);

  let prefsvc = Cc["@mozilla.org/preferences-service;1"].
    getService(Ci.nsIPrefBranch2);
  let allowKinetic = prefsvc.getBoolPref("browser.ui.panning.kinetic");

  this._modules.push(new PanningModule(this, allowKinetic));
  this._modules.push(new ClickingModule(this));
  this._modules.push(new ScrollwheelModule(this));
}

InputHandler.prototype = {
  _modules : [],
  _grabbed : null,

  grab: function(obj) {
    this._grabbed = obj;

    for each(mod in this._modules) {
      if (mod != obj)
        mod.cancelPending();
    }
    
    
  },

  ungrab: function(obj) {
    this._grabbed = null;
    
    
  },

  handleEvent: function (aEvent) {
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






function PanningModule(owner, useKinetic) {
  this._owner = owner;
  if (useKinetic !== undefined)
    this._dragData.useKinetic = useKinetic;
}

PanningModule.prototype = {
  _owner: null,
  _dragData: {
    dragging: false,
    sX: 0,
    sY: 0,
    useKinetic: true,
    dragStartTimeout: -1,
    
    targetScrollbox: null,
    
    panningContent: null,

    reset: function() {
      this.dragging = false;
      this.sX = 0;
      this.sY = 0;
      if (this.dragStartTimeout != -1)
        clearTimeout(this.dragStartTimeout);
      this.dragStartTimeout = -1;
      this.targetScrollbox = null;
      this.panningContent = false;
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

    reset: function() {
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

  handleEvent: function(aEvent) {
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

  


  cancelPending: function() {
    this._dragData.reset();
    
  },

  _dragStart: function(sX, sY) {
    let dragData = this._dragData;
    dragData.dragging = true;
    dragData.dragStartTimeout = -1;

    
    this._owner.grab(this);
    if (dragData.panningContent) {
      if (dragData.useKinetic)
        Browser.canvasBrowser.prepareForPanning();
      ws.dragStart(sX, sY);
    }

    
    this._kineticData.lastTime = Date.now();
  },

  _dragStop: function(sX, sY) {
    let dragData = this._dragData;

    this._owner.ungrab(this);

    if (dragData.targetScrollbox) {
      dragData.targetScrollbox.scrollBy(dragData.sX - sX, dragData.sY - sY);
    }
    else if (dragData.panningContent) {
      if (dragData.useKinetic) {
        
        if (!this._startKinetic(sX, sY))
          this._endKinetic(sX, sY);
      }
      else {
        ws.dragStop(sX, sY);
      }
    }
  },

  _dragMove: function(sX, sY) {
    let dragData = this._dragData;
    if (dragData.targetScrollbox)
      dragData.targetScrollbox.scrollBy(dragData.sX - sX, dragData.sY - sY);
    else if (dragData.panningContent)
      ws.dragMove(sX, sY);
  },

  _onMouseDown: function(aEvent) {
    
    if (this.kineticHandle != -1)
      this._endKinetic(aEvent.screenX, aEvent.screenY);

    let dragData = this._dragData;

    dragData.panningContent = (aEvent.target === document.getElementById("browser-canvas"));
    if (!dragData.panningContent)
      dragData.targetScrollbox = getScrollboxFromElement(aEvent.target);
    dragData.sX = aEvent.screenX;
    dragData.sY = aEvent.screenY;

    dragData.dragStartTimeout = setTimeout(function(self, sX, sY) { self._dragStart(sX, sY); },
                                           200, this, aEvent.screenX, aEvent.screenY);
  },

  _onMouseUp: function(aEvent) {
    let dragData = this._dragData;

    if (dragData.dragging)
      this._dragStop(aEvent.screenX, aEvent.screenY);

    dragData.reset(); 
  },

  _onMouseMove: function(aEvent) {
    
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

    if (dragData.panningContent && dragData.useKinetic) {
      
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

  _startKinetic: function(sX, sY) {
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

  _doKinetic: function(self) {
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

  _endKinetic: function(sX, sY) {
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





function ClickingModule(owner) {
  this._owner = owner;
}


ClickingModule.prototype = {
  _clickTimeout : -1,
  _events : [],
  _zoomed : false,

  handleEvent: function (aEvent) {
    
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
      case "mouseout":
        this._reset();
        break;
    }
  },

  


  cancelPending: function() {
    this._reset();
  },

  _reset: function() {
    if (this._clickTimeout != -1)
      clearTimeout(this._clickTimeout);
    this._clickTimeout = -1;

    this._events = [];
  },

  _sendSingleClick: function() {
    this._owner.grab(this);
    this._redispatchMouseEvent(this._events[0].event);
    this._redispatchMouseEvent(this._events[1].event);
    this._owner.ungrab(this);

    this._reset();
  },

  _sendDoubleClick: function() {
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


  _redispatchMouseEvent: function(aEvent, aType) {
    if (!(aEvent instanceof MouseEvent)) {
      Cu.reportError("_redispatchMouseEvent called with a non-mouse event");
      return;
    }

    var [x, y] = Browser.canvasBrowser._clientToContentCoords(aEvent.clientX, aEvent.clientY);

    var cwin = Browser.selectedBrowser.contentWindow;
    var cwu = cwin.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIDOMWindowUtils);

    
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
  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      
      case "DOMMouseScroll":
        this._owner.grab(this);
        Browser.canvasBrowser.zoom(aEvent.detail);
        this._owner.ungrab(this);
        break;
    }
  },

  


  cancelPending: function() {
  }
};
