












































const kDoubleClickInterval = 400;



const kDoubleClickThreshold = 200;


const kTapRadius = Services.prefs.getIntPref("ui.dragThresholdX");


const kAxisLockRevertThreshold = 200;


const kStateActive = 0x00000001;



















































function InputHandler(browserViewContainer) {
  
  this._modules = [];

  
  this._grabber = null;
  this._grabDepth = 0;

  
  this._ignoreEvents = false;

  
  this._suppressNextClick = false;

  
  window.addEventListener("mousedown", this, true);
  window.addEventListener("mouseup", this, true);
  window.addEventListener("mousemove", this, true);
  window.addEventListener("click", this, true);
  window.addEventListener("contextmenu", this, false);
  window.addEventListener("MozSwipeGesture", this, true);
  window.addEventListener("MozMagnifyGestureStart", this, true);
  window.addEventListener("MozMagnifyGestureUpdate", this, true);
  window.addEventListener("MozMagnifyGesture", this, true);

  
  browserViewContainer.addEventListener("keypress", this, false);
  browserViewContainer.addEventListener("keyup", this, false);
  browserViewContainer.addEventListener("keydown", this, false);
  browserViewContainer.addEventListener("DOMMouseScroll", this, true);
  browserViewContainer.addEventListener("MozMousePixelScroll", this, true);

  this.addModule(new MouseModule(this, browserViewContainer));
  this.addModule(new KeyModule(this, browserViewContainer));
  this.addModule(new GestureModule(this, browserViewContainer));
  this.addModule(new ScrollwheelModule(this, browserViewContainer));
}


InputHandler.prototype = {
  



  addModule: function addModule(m) {
    this._modules.push(m);
  },

  


  startListening: function startListening() {
    this._ignoreEvents = false;
  },

  


  stopListening: function stopListening() {
    this._ignoreEvents = true;
  },

  









  grab: function grab(grabber) {
    if (!this._grabber || this._grabber == grabber) {
      if (!this._grabber) {
        
        let mods = this._modules;
        for (let i = 0, len = mods.length; i < len; ++i)
          if (mods[i] != grabber)
            mods[i].cancelPending();
      }
      this._grabber = grabber;
      return true;
    }
    return false;
  },

  






  ungrab: function ungrab(grabber) {
    if (this._grabber == grabber) {  
      this._grabber = null;
    }
  },

  







  suppressNextClick: function suppressNextClick() {
    this._suppressNextClick = true;
  },

  



  allowClicks: function allowClicks() {
    this._suppressNextClick = false;
  },

  


  handleEvent: function handleEvent(aEvent) {
    if (this._ignoreEvents)
      return;

    
    if (aEvent.target.localName == "browser")
      return;

    if (this._suppressNextClick && aEvent.type == "click") {
      this._suppressNextClick = false;
      aEvent.stopPropagation();
      aEvent.preventDefault();
      return;
    }

    aEvent.time = Date.now();
    this._passToModules(aEvent);
  },

  



  _passToModules: function _passToModules(aEvent, aSkipToIndex) {
    if (this._grabber) {
      this._grabber.handleEvent(aEvent);
    } else {
      let mods = this._modules;
      let i = aSkipToIndex || 0;

      for (let len = mods.length; i < len; ++i) {
        mods[i].handleEvent(aEvent);  
        if (this._grabber)            
          break;
      }
    }
  }
};



























































function MouseModule(owner, browserViewContainer) {
  this._owner = owner;
  this._browserViewContainer = browserViewContainer;
  this._dragData = new DragData(this, kTapRadius);

  this._dragger = null;
  this._clicker = null;

  this._downUpEvents = [];
  this._targetScrollInterface = null;

  var self = this;
  this._kinetic = new KineticController(this._dragBy.bind(this),
                                        this._kineticStop.bind(this));

  messageManager.addMessageListener("Browser:ContextMenu", this);
}


MouseModule.prototype = {
  handleEvent: function handleEvent(aEvent) {
    if (aEvent.button !== 0 && aEvent.type != "contextmenu")
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
      case "contextmenu":
        if (ContextHelper.popupState && this._dragData.dragging)
          this._doDragStop(0, 0, true);
        break;
      case "MozMagnifyGestureStart":
      case "MozMagnifyGesture":
        
        if (this._dragData.dragging)
          this._doDragStop(0, 0, true);
        break;
    }
  },

  receiveMessage: function receiveMessage(aMessage) {
    
    
    if (aMessage.name != "Browser:ContextMenu" || !ContextHelper.popupState)
      return;

    if (this._clicker)
      this._clicker.panBegin();
    if (this._dragger)
      this._dragger.dragStop(0, 0, this._targetScrollInterface);
    this.cancelPending();
  },

  




  cancelPending: function cancelPending() {
    if (this._kinetic.isActive())
      this._kinetic.end();

    this._dragData.reset();
    this._targetScrollInterface = null;

    if (this._clickTimeout)
      window.clearTimeout(this._clickTimeout);

    this._cleanClickBuffer();
  },

  








  _onMouseDown: function _onMouseDown(aEvent) {
    this._owner.allowClicks();

    let dragData = this._dragData;
    if (dragData.dragging) {
      
      let [sX, sY] = dragData.panPosition();
      this._doDragStop(sX, sY, !dragData.isPan());
    }
    dragData.reset();

    
    
    let [targetScrollbox, targetScrollInterface, dragger]
      = this.getScrollboxFromElement(aEvent.target);

    
    let oldDragger = this._dragger;
    if (this._kinetic.isActive() && this._dragger != dragger)
      this._kinetic.end();

    let targetClicker = this.getClickerFromElement(aEvent.target);

    this._targetScrollInterface = targetScrollInterface;
    this._dragger = dragger;
    this._clicker = (targetClicker) ? targetClicker.customClicker : null;

    if (this._clicker)
      this._clicker.mouseDown(aEvent.clientX, aEvent.clientY);

    if (this._dragger && this._dragger.isDraggable(targetScrollbox, targetScrollInterface))
      this._doDragStart(aEvent);

    if (this._targetIsContent(aEvent)) {
      this._recordEvent(aEvent);
    }
    else {
      if (this._clickTimeout) {
        
        window.clearTimeout(this._clickTimeout);
        this._cleanClickBuffer();
      }

      if (this._dragger) {
        
        let draggable = this._dragger.isDraggable(targetScrollbox, targetScrollInterface);
        dragData.locked = !draggable.x || !draggable.y;
      }
    }
  },

  








  _onMouseUp: function _onMouseUp(aEvent) {
    let dragData = this._dragData;
    let oldIsPan = dragData.isPan();
    if (dragData.dragging) {
      dragData.setDragPosition(aEvent.screenX, aEvent.screenY);
      let [sX, sY] = dragData.panPosition();
      this._doDragStop(sX, sY, !dragData.isPan());
    }

    if (this._targetIsContent(aEvent)) {
      
      this._recordEvent(aEvent);
      let commitToClicker = this._clicker && dragData.isClick() && (this._downUpEvents.length > 1);
      if (commitToClicker)
        
        this._commitAnotherClick();
      else
        
        
        
        this._cleanClickBuffer();
    }
    else if (dragData.isPan()) {
      
      
      
      let generatesClick = aEvent.detail;
      if (generatesClick)
        this._owner.suppressNextClick();
    }

    let clicker = this._clicker;
    if (clicker) {
      
      if (!oldIsPan && dragData.isPan())
        clicker.panBegin();
      clicker.mouseUp(aEvent.clientX, aEvent.clientY);
    }

    this._owner.ungrab(this);
  },

  


  _onMouseMove: function _onMouseMove(aEvent) {
    let dragData = this._dragData;

    if (dragData.dragging) {
      let oldIsPan = dragData.isPan();
      dragData.setDragPosition(aEvent.screenX, aEvent.screenY);
      aEvent.stopPropagation();
      aEvent.preventDefault();
      if (dragData.isPan()) {
        this._owner.grab(this);
        
        let [sX, sY] = dragData.panPosition();
        this._doDragMove(sX, sY);

        
        let clicker = this._clicker;
        if (!oldIsPan && clicker)
          clicker.panBegin();
      }
    }
  },

  


  _targetIsContent: function _targetIsContent(aEvent) {
    let target = aEvent.target;
    return target && target.id == "inputhandler-overlay";
  },

  


  _doDragStart: function _doDragStart(event) {
    let dragData = this._dragData;
    dragData.setDragStart(event.screenX, event.screenY);
    this._kinetic.addData(0, 0);
    if (!this._kinetic.isActive())
      this._dragger.dragStart(event.clientX, event.clientY, event.target, this._targetScrollInterface);
  },

  




  _doDragStop: function _doDragStop(sX, sY, kineticStop) {
    let dragData = this._dragData;
    dragData.endDrag();

    if (!kineticStop) {
      
      let dX = dragData.prevPanX - sX;
      let dY = dragData.prevPanY - sY;
      this._kinetic.addData(-dX, -dY);
      this._kinetic.start();
    } else {
      
      this._dragger.dragStop(0, 0, this._targetScrollInterface);

      if (dragData.isPan()) {
        let event = document.createEvent("Events");
        event.initEvent("PanFinished", true, false);
        this._browserViewContainer.dispatchEvent(event);
      }
    }
  },

  


  _doDragMove: function _doDragMove(sX, sY) {
    let dragData = this._dragData;
    let dX = dragData.prevPanX - sX;
    let dY = dragData.prevPanY - sY;
    this._kinetic.addData(-dX, -dY);
    this._dragBy(dX, dY);
  },

  





  _dragBy: function _dragBy(dX, dY) {
    let dragData = this._dragData;
    return this._dragger.dragMove(dX, dY, this._targetScrollInterface);
  },

  
  _kineticStop: function _kineticStop() {
    let dragData = this._dragData;
    if (!dragData.dragging)
      this._doDragStop(0, 0, true);
  },

  






  _commitAnotherClick: function _commitAnotherClick() {
    if (this._clickTimeout) {   
      window.clearTimeout(this._clickTimeout);
      this._doDoubleClick();
    } else {
      let time = this._downUpEvents[1].time - this._downUpEvents[0].time;
      if (time >= kDoubleClickThreshold)
        this._doSingleClick();
      else
        this._clickTimeout = window.setTimeout(function _clickTimeout(self) { self._doSingleClick(); },
                                               kDoubleClickInterval, this);
    }
  },

  


  _doSingleClick: function _doSingleClick() {
    let ev = this._downUpEvents[1];
    this._cleanClickBuffer(2);

    
    let modifiers =
      (ev.altKey   ? Ci.nsIDOMNSEvent.ALT_MASK     : 0) |
      (ev.ctrlKey  ? Ci.nsIDOMNSEvent.CONTROL_MASK : 0) |
      (ev.shiftKey ? Ci.nsIDOMNSEvent.SHIFT_MASK   : 0) |
      (ev.metaKey  ? Ci.nsIDOMNSEvent.META_MASK    : 0);
    this._clicker.singleClick(ev.clientX, ev.clientY, modifiers);
  },

  


  _doDoubleClick: function _doDoubleClick() {
    let mouseUp1 = this._downUpEvents[1];
    
    let mouseUp2 = this._downUpEvents[Math.min(3, this._downUpEvents.length - 1)];
    this._cleanClickBuffer(4);
    this._clicker.doubleClick(mouseUp1.clientX, mouseUp1.clientY,
                              mouseUp2.clientX, mouseUp2.clientY);
  },

  



  _recordEvent: function _recordEvent(aEvent) {
    this._downUpEvents.push(aEvent);
  },

  







  _cleanClickBuffer: function _cleanClickBuffer(howMany) {
    delete this._clickTimeout;

    if (howMany == undefined)
      howMany = this._downUpEvents.length;

    this._downUpEvents.splice(0, howMany);
  },

  




  _defaultDragger: {
    isDraggable: function isDraggable(target, scroller) {
      let sX = {}, sY = {};
      scroller.getScrolledSize(sX, sY);
      let rect = target.getBoundingClientRect();
      return { x: sX.value > rect.width, y: sY.value > rect.height };
    },

    dragStart: function dragStart(cx, cy, target, scroller) {},

    dragStop : function dragStop(dx, dy, scroller) {
      return this.dragMove(dx, dy, scroller);
    },

    dragMove : function dragMove(dx, dy, scroller) {
      if (scroller.getPosition) {
        try {

          let oldX = {}, oldY = {};
          scroller.getPosition(oldX, oldY);

          scroller.scrollBy(dx, dy);

          let newX = {}, newY = {};
          scroller.getPosition(newX, newY);

          return (newX.value != oldX.value) || (newY.value != oldY.value);

        } catch (e) {  }
      }

      return false;
    }
  },

  
  

  







  getScrollboxFromElement: function getScrollboxFromElement(elem) {
    let scrollbox = null;
    let qinterface = null;

    for (; elem; elem = elem.parentNode) {
      try {
        if (elem.ignoreDrag) {
          break;
        }

        if (elem.scrollBoxObject) {
          scrollbox = elem;
          qinterface = elem.scrollBoxObject;
          break;
        } else if (elem.boxObject) {
          let qi = (elem._cachedSBO) ? elem._cachedSBO
                                     : elem.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
          if (qi) {
            scrollbox = elem;
            scrollbox._cachedSBO = qinterface = qi;
            break;
          }
        } else if (elem.customDragger) {
          scrollbox = elem;
          break;
        }
      } catch (e) { 

 }
    }
    return [scrollbox, qinterface, (scrollbox ? (scrollbox.customDragger || this._defaultDragger) : null)];
  },

  



  getClickerFromElement: function getClickerFromElement(elem) {
    for (; elem; elem = elem.parentNode)
      if (elem.customClicker)
        break;

    return (elem) ? elem : null;
  },

  toString: function toString() {
    return '[MouseModule] {'
      + '\n\tdragData=' + this._dragData + ', '
      + 'dragger=' + this._dragger + ', '
      + 'clicker=' + this._clicker + ', '
      + '\n\tdownUpEvents=' + this._downUpEvents + ', '
      + 'length=' + this._downUpEvents.length + ', '
      + '\n\ttargetScroller=' + this._targetScrollInterface + ', '
      + '\n\tclickTimeout=' + this._clickTimeout + '\n  }';
  }
};





function DragData(owner, dragRadius) {
  this._owner = owner;
  this._dragRadius = dragRadius;
  this._domUtils = Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
  this.reset();
};

DragData.prototype = {
  reset: function reset() {
    this.dragging = false;
    this.sX = null;
    this.sY = null;
    this.locked = false;
    this.stayLocked = false;
    this.lockedX = null;
    this.lockedY = null;
    this._originX = null;
    this._originY = null;
    this.prevPanX = null;
    this.prevPanY = null;
    this._isPan = false;
  },

  
  _lockAxis: function _lockAxis(sX, sY) {
    if (this.locked) {
      if (this.lockedX !== null)
        sX = this.lockedX;
      else if (this.lockedY !== null)
        sY = this.lockedY;
      return [sX, sY];
    }
    else {
      return [this._originX, this._originY];
    }
  },

  setDragPosition: function setDragPosition(sX, sY) {
    
    if (!this._isPan) {
      let distanceSquared = (Math.pow(sX - this._originX, 2) + Math.pow(sY - this._originY, 2));
      this._isPan = (distanceSquared > Math.pow(this._dragRadius, 2));
      if (this._isPan)
        this._resetActive();
    }

    
    if (this._isPan) {
      let absX = Math.abs(this._originX - sX);
      let absY = Math.abs(this._originY - sY);

      if (absX > kAxisLockRevertThreshold || absY > kAxisLockRevertThreshold)
        this.stayLocked = true;

      
      if (!this.stayLocked) {
        if (this.lockedX && absX > 3 * absY)
          this.lockedX = null;
        else if (this.lockedY && absY > 3 * absX)
          this.lockedY = null;
      }

      if (!this.locked) {
        
        

        
        
        
        
        

        if (absX > 2.5 * absY)
          this.lockedY = sY;
        else if (absY > absX)
          this.lockedX = sX;

        this.locked = true;
      }

      
      
      let [prevX, prevY] = this._lockAxis(this.sX, this.sY);
      this.prevPanX = prevX;
      this.prevPanY = prevY;
    }

    this.sX = sX;
    this.sY = sY;
  },

  setDragStart: function setDragStart(screenX, screenY) {
    this.sX = this._originX = screenX;
    this.sY = this._originY = screenY;
    this.dragging = true;
    this.locked = false;
    this.stayLocked = false;
  },

  endDrag: function endDrag() {
    this._resetActive();
    this.dragging = false;
  },

  
  isPan: function isPan() {
    return this._isPan;
  },

  
  isClick: function isClick() {
    return !this._isPan;
  },

  



  panPosition: function panPosition() {
    return this._lockAxis(this.sX, this.sY);
  },

  _resetActive: function _resetActive() {
    
    let target = document.documentElement;
    let state = this._domUtils.getContentState(target);
    this._domUtils.setContentState(target, state & kStateActive);
  },

  toString: function toString() {
    return '[DragData] { sX,sY=' + this.sX + ',' + this.sY + ', dragging=' + this.dragging + ' }';
  }
};














function KineticController(aPanBy, aEndCallback) {
  this._panBy = aPanBy;
  this._beforeEnd = aEndCallback;

  
  
  this._position = new Point(0, 0);
  this._velocity = new Point(0, 0);
  this._acceleration = new Point(0, 0);
  this._time = 0;
  this._timeStart = 0;

  
  
  this._updateInterval = Services.prefs.getIntPref("browser.ui.kinetic.updateInterval");
  
  this._decelerationRate = Services.prefs.getIntPref("browser.ui.kinetic.decelerationRate") / 10000;
  
  this._speedSensitivity = Services.prefs.getIntPref("browser.ui.kinetic.speedSensitivity") / 100;
  
  this._swipeLength = Services.prefs.getIntPref("browser.ui.kinetic.swipeLength");

  this._reset();
}

KineticController.prototype = {
  _reset: function _reset() {
    if (this._callback) {
      removeEventListener("MozBeforePaint", this._callback, false);
      this._callback = null;
    }

    this.momentumBuffer = [];
    this._velocity.set(0, 0);
  },

  isActive: function isActive() {
    return !!this._callback;
  },

  _startTimer: function _startTimer() {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    let lastx = this._position;  
    let v0 = this._velocity;  
    let a = this._acceleration;  

    
    let aBin = new Point(0, 0);
    let v0Bin = new Point(0, 0);
    let self = this;

    let callback = {
      handleEvent: function kineticHandleEvent(event) {

        if (!self.isActive())  
          return;

        
        
        
        let realt = event.timeStamp - self._initialTime;
        self._time += self._updateInterval;
        let t = (self._time + realt) / 2;

        
        let x = v0Bin.set(v0).scale(t).add(aBin.set(a).scale(0.5 * t * t));
        let dx = x.x - lastx.x;
        let dy = x.y - lastx.y;
        lastx.set(x);

        
        
        if (t >= -v0.x / a.x) {
          
          dx = -v0.x * v0.x / 2 / a.x - lastx.x;
          
          lastx.x = 0;
          v0.x = 0;
          a.x = 0;
        }
        
        if (t >= -v0.y / a.y) {
          dy = -v0.y * v0.y / 2 / a.y - lastx.y;
          lastx.y = 0;
          v0.y = 0;
          a.y = 0;
        }

        let panned = false;
        try { panned = self._panBy(Math.round(-dx), Math.round(-dy)); } catch (e) {}
        if (!panned)
          self.end();
        else
          mozRequestAnimationFrame();
      }
    };

    this._callback = callback;
    addEventListener("MozBeforePaint", callback, false);
    mozRequestAnimationFrame();
  },

  start: function start() {
    function sign(x) {
      return x ? ((x > 0) ? 1 : -1) : 0;
    }

    let mb = this.momentumBuffer;
    let mblen = this.momentumBuffer.length;

    let lastTime = mb[mblen - 1].t;
    let distanceX = 0;
    let distanceY = 0;
    let swipeLength = this._swipeLength;

    
    let me;
    for (let i = 0; i < mblen; i++) {
      me = mb[i];
      if (lastTime - me.t < swipeLength) {
        distanceX += me.dx;
        distanceY += me.dy;
      }
    }

    
    this._velocity.x = (distanceX < 0 ? Math.min : Math.max)((distanceX / swipeLength) * this._speedSensitivity, this._velocity.x);
    this._velocity.y = (distanceY < 0 ? Math.min : Math.max)((distanceY / swipeLength) * this._speedSensitivity, this._velocity.y);

    
    this._acceleration.set(this._velocity.clone().map(sign).scale(-this._decelerationRate));

    this._position.set(0, 0);
    this._initialTime = mozAnimationStartTime;
    this._time = 0;
    this.momentumBuffer = [];

    if (!this.isActive())
      this._startTimer();

    return true;
  },

  end: function end() {
    if (this._beforeEnd)
      this._beforeEnd();
    this._reset();
  },

  addData: function addData(dx, dy) {
    let mbLength = this.momentumBuffer.length;
    let now = Date.now();

    if (this.isActive()) {
      
      if (dx * this._velocity.x < 0 || dy * this._velocity.y < 0)
        this.end();
    }

    this.momentumBuffer.push({'t': now, 'dx' : dx, 'dy' : dy});
  }
};





function KeyModule(owner, browserViewContainer) {
  this._owner = owner;
  this._browserViewContainer = browserViewContainer;
}

KeyModule.prototype = {
  getClickerFromElement: function getClickerFromElement(elem) {
    for (; elem; elem = elem.parentNode)
      if (elem.customKeySender)
        break;
    return (elem) ? elem : null;
  },

  handleEvent: function handleEvent(aEvent) {
    if (aEvent.type == "keydown" || aEvent.type == "keyup" || aEvent.type == "keypress") {
      let keyer = this._browserViewContainer.customKeySender;
      if (keyer) {
        keyer.dispatchKeyEvent(aEvent);
        aEvent.stopPropagation();
        aEvent.preventDefault();
      }
    }
  },

  
  cancelPending: function cancelPending() {}
};






function ScrollwheelModule(owner, browserViewContainer) {
  this._owner = owner;
  this._browserViewContainer = browserViewContainer;
}

ScrollwheelModule.prototype = {
  pendingEvent : 0,
  handleEvent: function handleEvent(aEvent) {
    if (aEvent.type == "DOMMouseScroll" || aEvent.type == "MozMousePixelScroll") {
      



      if (this.pendingEvent)
        clearTimeout(this.pendingEvent);

      this.pendingEvent = setTimeout(function handleEventImpl(self) {
        self.pendingEvent = 0;
        Browser.zoom(aEvent.detail);
      }, 0, this);

      aEvent.stopPropagation();
      aEvent.preventDefault();
    }
  },

  
  cancelPending: function cancelPending() {}
};













function GestureModule(owner, browserViewContainer) {
  this._owner = owner;
  this._browserViewContainer = browserViewContainer;
}

GestureModule.prototype = {
  






  handleEvent: function handleEvent(aEvent) {
    try {
      let consume = false;
      switch (aEvent.type) {
        case "MozSwipeGesture":
          let gesture = Ci.nsIDOMSimpleGestureEvent;
          switch (aEvent.direction) {
            case gesture.DIRECTION_UP:
              Browser.scrollContentToBottom();
              break;
            case gesture.DIRECTION_DOWN:
              Browser.scrollContentToTop();
              break;
            case gesture.DIRECTION_LEFT:
              CommandUpdater.doCommand("cmd_back");
              break;
            case gesture.DIRECTION_RIGHT:
              CommandUpdater.doCommand("cmd_forward");
              break;
          }
          break;

        case "MozMagnifyGestureStart":
          consume = true;
          this._pinchStart(aEvent);
          break;

        case "MozMagnifyGestureUpdate":
          consume = true;
          if (this._ignoreNextUpdate)
            this._ignoreNextUpdate = false;
          else
            this._pinchUpdate(aEvent);
          break;

        case "MozMagnifyGesture":
          consume = true;
          this._pinchEnd(aEvent);
          break;

        case "contextmenu":
          
          if (this._pinchZoom)
            consume = true;
          break;
      }
      if (consume) {
        
        aEvent.stopPropagation();
        aEvent.preventDefault();
      }
    }
    catch (e) {
      Util.dumpLn("Error while handling gesture event", aEvent.type,
                  "\nPlease report error at:", e);
      Cu.reportError(e);
    }
  },

  cancelPending: function cancelPending() {
    
    if (this._pinchZoom) {
      this._pinchZoom.finish();
      this._pinchZoom = null;
    }
  },

  _pinchStart: function _pinchStart(aEvent) {
    
    if (this._pinchZoom || (aEvent.target instanceof XULElement) || !Browser.selectedTab.allowZoom)
      return;

    
    this._owner.grab(this);

    
    
    document.getElementById("inputhandler-overlay").customClicker.panBegin();

    
    this._pinchZoom = animatedZoom;

    
    this._pinchZoomLevel = getBrowser().scale;
    this._pinchDelta = 0;
    this._ignoreNextUpdate = true; 

    
    this._maxGrowth = Services.prefs.getIntPref("browser.ui.pinch.maxGrowth");
    this._maxShrink = Services.prefs.getIntPref("browser.ui.pinch.maxShrink");
    this._scalingFactor = Services.prefs.getIntPref("browser.ui.pinch.scalingFactor");

    
    [this._pinchStartX, this._pinchStartY] =
        Browser.transformClientToBrowser(aEvent.clientX, aEvent.clientY);

    let scrollX = {}, scrollY = {};
    getBrowser().getPosition(scrollX, scrollY);
    this._pinchStartX += scrollX.value;
    this._pinchStartY += scrollY.value;
  },

  _pinchUpdate: function _pinchUpdate(aEvent) {
    if (!this._pinchZoom || !aEvent.delta)
      return;

    
    this._pinchDelta += aEvent.delta;

    
    let delta = Math.max(-this._maxShrink, Math.min(this._maxGrowth, this._pinchDelta));
    this._pinchZoomLevel *= (1 + delta / this._scalingFactor);
    this._pinchZoomLevel = Browser.selectedTab.clampZoomLevel(this._pinchZoomLevel);
    this._pinchDelta = 0;

    
    let [pX, pY] =
        Browser.transformClientToBrowser(aEvent.clientX, aEvent.clientY);

    let scrollX = {}, scrollY = {};
    getBrowser().getPosition(scrollX, scrollY);
    pX += scrollX.value;
    pY += scrollY.value;

    
    let rect = Browser._getZoomRectForPoint(2 * this._pinchStartX - pX,
                                            2 * this._pinchStartY - pY,
                                            this._pinchZoomLevel);
    this._pinchZoom.updateTo(rect);
  },

  _pinchEnd: function _pinchEnd(aEvent) {
    
    this._owner.ungrab(this);

    
    if (this._pinchZoom) {
      
      this._pinchZoom.finish();
      this._pinchZoom = null;
    }
  }
};

