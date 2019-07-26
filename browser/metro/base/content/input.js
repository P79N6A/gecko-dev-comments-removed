




Components.utils.import("resource://gre/modules/Geometry.jsm");






const kAxisLockRevertThreshold = 0.8;


const kStateActive = 0x00000001;



const kStopKineticPanOnDragTimeout = 300;


const kMinVelocity = 0.4;
const kMaxVelocity = 6;











const kDebugMouseInputPref = "metro.debug.treatmouseastouch";


const kDebugSelectionDisplayPref = "metro.debug.selection.displayRanges";


const kDebugSelectionDumpPref = "metro.debug.selection.dumpRanges";

const kDebugSelectionDumpEvents = "metro.debug.selection.dumpEvents";
































var TouchModule = {
  _debugEvents: false,
  _isCancelled: false,
  _isCancellable: false,

  init: function init() {
    this._dragData = new DragData();

    this._dragger = null;

    this._targetScrollbox = null;
    this._targetScrollInterface = null;

    this._kinetic = new KineticController(this._dragBy.bind(this),
                                          this._kineticStop.bind(this));

    
    window.addEventListener("CancelTouchSequence", this, true);
    window.addEventListener("dblclick", this, true);

    
    window.addEventListener("contextmenu", this, false);
    window.addEventListener("touchstart", this, false);
    window.addEventListener("touchmove", this, false);
    window.addEventListener("touchend", this, false);

    try {
      this._treatMouseAsTouch = Services.prefs.getBoolPref(kDebugMouseInputPref);
    } catch (e) {}
  },

  



  _treatMouseAsTouch: false,

  



  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "contextmenu":
        this._onContextMenu(aEvent);
        break;

      case "CancelTouchSequence":
        this.cancelPending();
        break;

      default: {
        if (this._debugEvents) {
          if (aEvent.type != "touchmove")
            Util.dumpLn("TouchModule:", aEvent.type, aEvent.target);
        }

        switch (aEvent.type) {
          case "touchstart":
            this._onTouchStart(aEvent);
            break;
          case "touchmove":
            this._onTouchMove(aEvent);
            break;
          case "touchend":
            this._onTouchEnd(aEvent);
            break;
          case "dblclick":
            
            
            
            
            setTimeout(function () {
              let contextInfo = { name: "",
                                  json: { xPos: aEvent.clientX, yPos: aEvent.clientY },
                                  target: Browser.selectedTab.browser };
              SelectionHelperUI.attachEditSession(contextInfo);
            }, 50);
            break;
        }
      }
    }
  },

  sample: function sample(aTimeStamp) {
    this._waitingForPaint = false;
  },

  




  cancelPending: function cancelPending() {
    this._doDragStop();

    
    
    this._kinetic.end();

    this._targetScrollbox = null;
    this._targetScrollInterface = null;
  },

  _onContextMenu: function _onContextMenu(aEvent) {
    
    
    if (this._treatMouseAsTouch) {
      let event = document.createEvent("Events");
      event.initEvent("MozEdgeUIGesture", true, false);
      window.dispatchEvent(event);
      return;
    }

    
    
    if (ContextMenuUI.popupState) {
      this.cancelPending();
    }
  },

  
  _onTouchStart: function _onTouchStart(aEvent) {
    if (aEvent.touches.length > 1)
      return;

    this._isCancelled = false;
    this._isCancellable = true;

    if (aEvent.defaultPrevented) {
      this._isCancelled = true;
      return;
    }

    let dragData = this._dragData;
    if (dragData.dragging) {
      
      this._doDragStop();
    }
    dragData.reset();
    this.dX = 0;
    this.dY = 0;

    
    
    let [targetScrollbox, targetScrollInterface, dragger]
      = ScrollUtils.getScrollboxFromElement(aEvent.originalTarget);

    
    if (this._kinetic.isActive() && this._dragger != dragger)
      this._kinetic.end();

    this._targetScrollbox = targetScrollInterface ? targetScrollInterface.element : targetScrollbox;
    this._targetScrollInterface = targetScrollInterface;

    if (!this._targetScrollbox) {
      return;
    }

    
    if (dragger) {
      let draggable = dragger.isDraggable(targetScrollbox, targetScrollInterface);
      dragData.locked = !draggable.x || !draggable.y;
      if (draggable.x || draggable.y) {
        this._dragger = dragger;
        if (dragger.freeDrag)
          dragData.alwaysFreeDrag = dragger.freeDrag();
        this._doDragStart(aEvent, draggable);
      }
    }
  },

  
  _onTouchEnd: function _onTouchEnd(aEvent) {
    if (aEvent.touches.length > 0 || this._isCancelled || !this._targetScrollbox)
      return;

    
    
    this._waitingForPaint = false;
    this._onTouchMove(aEvent);

    let dragData = this._dragData;
    this._doDragStop();
  },

  


  _onTouchMove: function _onTouchMove(aEvent) {
    if (aEvent.touches.length > 1)
      return;

    if (this._isCancellable) {
      
      this._isCancellable = false;
      if (aEvent.defaultPrevented)
        this._isCancelled = true;
    }

    if (this._isCancelled)
      return;

    let touch = aEvent.changedTouches[0];
    if (!this._targetScrollbox) {
      return;
    }

    let dragData = this._dragData;

    if (dragData.dragging) {
      let oldIsPan = dragData.isPan();
      dragData.setDragPosition(touch.screenX, touch.screenY);
      dragData.setMousePosition(touch);

      
      
      
      
      
      
      
      
      
      let [sX, sY] = dragData.panPosition();
      this.dX += dragData.prevPanX - sX;
      this.dY += dragData.prevPanY - sY;

      if (dragData.isPan()) {
        
        this._kinetic.addData(sX - dragData.prevPanX, sY - dragData.prevPanY);

        
        this._dragBy(this.dX, this.dY);

        
        if (!oldIsPan && dragData.isPan()) {
          

          let event = document.createEvent("Events");
          event.initEvent("PanBegin", true, false);
          this._targetScrollbox.dispatchEvent(event);

          Browser.selectedBrowser.messageManager.sendAsyncMessage("Browser:PanBegin", {});
        }
      }
    }
  },

  


  _doDragStart: function _doDragStart(aEvent, aDraggable) {
    let touch = aEvent.changedTouches[0];
    let dragData = this._dragData;
    dragData.setDragStart(touch.screenX, touch.screenY, aDraggable);
    this._kinetic.addData(0, 0);
    this._dragStartTime = Date.now();
    if (!this._kinetic.isActive()) {
      this._dragger.dragStart(touch.clientX, touch.clientY, touch.target, this._targetScrollInterface);
    }
  },

  
  _doDragStop: function _doDragStop() {
    let dragData = this._dragData;
    if (!dragData.dragging)
      return;

    dragData.endDrag();

    
    
    

    if (dragData.isPan()) {
      if (Date.now() - this._dragStartTime > kStopKineticPanOnDragTimeout)
        this._kinetic._velocity.set(0, 0);
      
      this._kinetic.start();
    } else {
      this._kinetic.end();
      if (this._dragger)
        this._dragger.dragStop(0, 0, this._targetScrollInterface);
      this._dragger = null;
    }
  },

  





  _dragBy: function _dragBy(dX, dY, aIsKinetic) {
    let dragged = true;
    let dragData = this._dragData;
    if (!this._waitingForPaint || aIsKinetic) {
      let dragData = this._dragData;
      dragged = this._dragger.dragMove(dX, dY, this._targetScrollInterface, aIsKinetic,
                                       dragData._mouseX, dragData._mouseY);
      if (dragged && !this._waitingForPaint) {
        this._waitingForPaint = true;
        mozRequestAnimationFrame(this);
      }
      this.dX = 0;
      this.dY = 0;
    }
    if (!dragData.isPan())
      this._kinetic.pause();

    return dragged;
  },

  
  _kineticStop: function _kineticStop() {
    
    
    let dragData = this._dragData;
    if (!dragData.dragging) {
      if (this._dragger)
        this._dragger.dragStop(0, 0, this._targetScrollInterface);
      this._dragger = null;

      let event = document.createEvent("Events");
      event.initEvent("PanFinished", true, false);
      this._targetScrollbox.dispatchEvent(event);
    }
  },

  toString: function toString() {
    return '[TouchModule] {'
      + '\n\tdragData=' + this._dragData + ', '
      + 'dragger=' + this._dragger + ', '
      + '\n\ttargetScroller=' + this._targetScrollInterface + '}';
  },
};

var ScrollUtils = {
  
  get tapRadius() {
    let dpi = Util.displayDPI;
    delete this.tapRadius;
    return this.tapRadius = Services.prefs.getIntPref("ui.dragThresholdX") / 240 * dpi;
  },

  







  getScrollboxFromElement: function getScrollboxFromElement(elem) {
    let scrollbox = null;
    let qinterface = null;
    
    if (elem.ownerDocument == Browser.selectedBrowser.contentDocument) {
      elem = Browser.selectedBrowser;
    }
    for (; elem; elem = elem.parentNode) {
      try {
        if (elem.anonScrollBox) {
          scrollbox = elem.anonScrollBox;
          qinterface = scrollbox.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
        } else if (elem.scrollBoxObject) {
          scrollbox = elem;
          qinterface = elem.scrollBoxObject;
          break;
        } else if (elem.customDragger) {
          scrollbox = elem;
          break;
        } else if (elem.boxObject) {
          let qi = (elem._cachedSBO) ? elem._cachedSBO
                                     : elem.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
          if (qi) {
            scrollbox = elem;
            scrollbox._cachedSBO = qinterface = qi;
            break;
          }
        }
      } catch (e) { 

 }
    }
    return [scrollbox, qinterface, (scrollbox ? (scrollbox.customDragger || this._defaultDragger) : null)];
  },

  
  isPan: function isPan(aPoint, aPoint2) {
    return (Math.abs(aPoint.x - aPoint2.x) > this.tapRadius ||
            Math.abs(aPoint.y - aPoint2.y) > this.tapRadius);
  },

  




  _defaultDragger: {
    isDraggable: function isDraggable(target, scroller) {
      let sX = {}, sY = {},
          pX = {}, pY = {};
      scroller.getPosition(pX, pY);
      scroller.getScrolledSize(sX, sY);
      let rect = target.getBoundingClientRect();
      return { x: (sX.value > rect.width  || pX.value != 0),
               y: (sY.value > rect.height || pY.value != 0) };
    },

    dragStart: function dragStart(cx, cy, target, scroller) {
      scroller.element.addEventListener("PanBegin", this._showScrollbars, false);
    },

    dragStop: function dragStop(dx, dy, scroller) {
      scroller.element.removeEventListener("PanBegin", this._showScrollbars, false);
      return this.dragMove(dx, dy, scroller);
    },

    dragMove: function dragMove(dx, dy, scroller) {
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
    },

    _showScrollbars: function _showScrollbars(aEvent) {
      let scrollbox = aEvent.target;
      scrollbox.setAttribute("panning", "true");

      let hideScrollbars = function() {
        scrollbox.removeEventListener("PanFinished", hideScrollbars, false);
        scrollbox.removeEventListener("CancelTouchSequence", hideScrollbars, false);
        scrollbox.removeAttribute("panning");
      }

      
      scrollbox.addEventListener("PanFinished", hideScrollbars, false);
      scrollbox.addEventListener("CancelTouchSequence", hideScrollbars, false);
    }
  }
};





function DragData() {
  this._domUtils = Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
  this._lockRevertThreshold = Util.displayDPI * kAxisLockRevertThreshold;
  this.reset();
};

DragData.prototype = {
  reset: function reset() {
    this.dragging = false;
    this.sX = null;
    this.sY = null;
    this.locked = false;
    this.stayLocked = false;
    this.alwaysFreeDrag = false;
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

  setMousePosition: function setMousePosition(aEvent) {
    this._mouseX = aEvent.clientX;
    this._mouseY = aEvent.clientY;
  },

  setDragPosition: function setDragPosition(sX, sY) {
    
    if (!this._isPan) {
      this._isPan = ScrollUtils.isPan(new Point(this._originX, this._originY), new Point(sX, sY));
      if (this._isPan) {
        this._resetActive();
      }
    }

    
    if (this._isPan) {
      let absX = Math.abs(this._originX - sX);
      let absY = Math.abs(this._originY - sY);

      if (absX > this._lockRevertThreshold || absY > this._lockRevertThreshold)
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
    }

    
    if (this.alwaysFreeDrag) {
      this.lockedY = null;
      this.lockedX = null;
    }

    
    
    let [prevX, prevY] = this._lockAxis(this.sX, this.sY);
    this.prevPanX = prevX;
    this.prevPanY = prevY;

    this.sX = sX;
    this.sY = sY;
  },

  setDragStart: function setDragStart(screenX, screenY, aDraggable) {
    this.sX = this._originX = screenX;
    this.sY = this._originY = screenY;
    this.dragging = true;

    
    
    this.lockedX = !aDraggable.x ? screenX : null;
    this.lockedY = !aDraggable.y ? screenY : null;
    this.stayLocked = this.lockedX || this.lockedY;
    this.locked = this.stayLocked;
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
    
    if (this._domUtils.getContentState(target) & kStateActive)
      this._domUtils.setContentState(target, kStateActive);
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
  
  this._exponentialC = Services.prefs.getIntPref("browser.ui.kinetic.exponentialC");
  this._polynomialC = Services.prefs.getIntPref("browser.ui.kinetic.polynomialC") / 1000000;
  
  this._swipeLength = Services.prefs.getIntPref("browser.ui.kinetic.swipeLength");

  this._reset();
}

KineticController.prototype = {
  _reset: function _reset() {
    this._active = false;
    this._paused = false;
    this.momentumBuffer = [];
    this._velocity.set(0, 0);
  },

  isActive: function isActive() {
    return this._active;
  },

  _startTimer: function _startTimer() {
    let self = this;

    let lastp = this._position;  
    let v0 = this._velocity;  
    let a = this._acceleration;  
    let c = this._exponentialC;
    let p = new Point(0, 0);
    let dx, dy, t, realt;

    function calcP(v0, a, t) {
      
      
      
      
      
      
      
      return v0 * Math.exp(-t / c) * -c + a * t * t + v0 * c;
    }

    this._calcV = function(v0, a, t) {
      return v0 * Math.exp(-t / c) + 2 * a * t;
    }

    let callback = {
      sample: function kineticHandleEvent(timeStamp) {
        
        
        if (!self.isActive() || self._paused)
          return;

        
        
        
        realt = timeStamp - self._initialTime;
        self._time += self._updateInterval;
        t = (self._time + realt) / 2;

        
        p.x = calcP(v0.x, a.x, t);
        p.y = calcP(v0.y, a.y, t);
        dx = Math.round(p.x - lastp.x);
        dy = Math.round(p.y - lastp.y);

        
        if (dx * a.x > 0) {
          dx = 0;
          lastp.x = 0;
          v0.x = 0;
          a.x = 0;
        }
        
        if (dy * a.y > 0) {
          dy = 0;
          lastp.y = 0;
          v0.y = 0;
          a.y = 0;
        }

        if (v0.x == 0 && v0.y == 0) {
          self.end();
        } else {
          let panStop = false;
          if (dx != 0 || dy != 0) {
            try { panStop = !self._panBy(-dx, -dy, true); } catch (e) {}
            lastp.add(dx, dy);
          }

          if (panStop)
            self.end();
          else
            mozRequestAnimationFrame(this);
        }
      }
    };

    this._active = true;
    this._paused = false;
    mozRequestAnimationFrame(callback);
  },

  start: function start() {
    function sign(x) {
      return x ? ((x > 0) ? 1 : -1) : 0;
    }

    function clampFromZero(x, closerToZero, furtherFromZero) {
      if (x >= 0)
        return Math.max(closerToZero, Math.min(furtherFromZero, x));
      return Math.min(-closerToZero, Math.max(-furtherFromZero, x));
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

    let currentVelocityX = 0;
    let currentVelocityY = 0;

    if (this.isActive()) {
      
      let currentTime = Date.now() - this._initialTime;
      currentVelocityX = Util.clamp(this._calcV(this._velocity.x, this._acceleration.x, currentTime), -kMaxVelocity, kMaxVelocity);
      currentVelocityY = Util.clamp(this._calcV(this._velocity.y, this._acceleration.y, currentTime), -kMaxVelocity, kMaxVelocity);
    }

    if (currentVelocityX * this._velocity.x <= 0)
      currentVelocityX = 0;
    if (currentVelocityY * this._velocity.y <= 0)
      currentVelocityY = 0;

    let swipeTime = Math.min(swipeLength, lastTime - mb[0].t);
    this._velocity.x = clampFromZero((distanceX / swipeTime) + currentVelocityX, Math.abs(currentVelocityX), kMaxVelocity);
    this._velocity.y = clampFromZero((distanceY / swipeTime) + currentVelocityY, Math.abs(currentVelocityY), kMaxVelocity);

    if (Math.abs(this._velocity.x) < kMinVelocity)
      this._velocity.x = 0;
    if (Math.abs(this._velocity.y) < kMinVelocity)
      this._velocity.y = 0;

    
    this._acceleration.set(this._velocity.clone().map(sign).scale(-this._polynomialC));

    this._position.set(0, 0);
    this._initialTime = mozAnimationStartTime;
    this._time = 0;
    this.momentumBuffer = [];

    if (!this.isActive() || this._paused)
      this._startTimer();

    return true;
  },

  pause: function pause() {
    this._paused = true;
  },

  end: function end() {
    if (this.isActive()) {
      if (this._beforeEnd)
        this._beforeEnd();
      this._reset();
    }
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






var ScrollwheelModule = {
  _pendingEvent : 0,
  _container: null,
  
  init: function init(container) {
    this._container = container;
    window.addEventListener("MozPrecisePointer", this, true);
    window.addEventListener("MozImprecisePointer", this, true);
  },

  handleEvent: function handleEvent(aEvent) {
    switch(aEvent.type) {
      case "DOMMouseScroll":
      case "MozMousePixelScroll":
        this._onScroll(aEvent);
        break;
      case "MozPrecisePointer":
        this._container.removeEventListener("DOMMouseScroll", this, true);
        this._container.removeEventListener("MozMousePixelScroll", this, true);
        break;
      case "MozImprecisePointer":
        this._container.addEventListener("DOMMouseScroll", this, true);
        this._container.addEventListener("MozMousePixelScroll", this, true);
        break;
    };
  },

  _onScroll: function _onScroll(aEvent) {
    
    
    
    if (this._pendingEvent)
      clearTimeout(this._pendingEvent);

    this._pendingEvent = setTimeout(function handleEventImpl(self) {
      self._pendingEvent = 0;
      Browser.zoom(aEvent.detail);
    }, 0, this);

    aEvent.stopPropagation();
    aEvent.preventDefault();
  },

  
  cancelPending: function cancelPending() {}
};






var GestureModule = {
  _debugEvents: false,

  init: function init() {
    window.addEventListener("MozSwipeGesture", this, true);
    




    window.addEventListener("CancelTouchSequence", this, true);
  },

  _initMouseEventFromGestureEvent: function _initMouseEventFromGestureEvent(aDestEvent, aSrcEvent, aType, aCanBubble, aCancellable) {
    aDestEvent.initMouseEvent(aType, aCanBubble, aCancellable, window, null,
                              aSrcEvent.screenX, aSrcEvent.screenY, aSrcEvent.clientX, aSrcEvent.clientY,
                              aSrcEvent.ctrlKey, aSrcEvent.altKey, aSrcEvent.shiftKey, aSrcEvent.metaKey,
                              aSrcEvent.button, aSrcEvent.relatedTarget);
  },

  









  handleEvent: function handleEvent(aEvent) {
    try {
      aEvent.stopPropagation();
      aEvent.preventDefault();
      if (this._debugEvents) Util.dumpLn("GestureModule:", aEvent.type);
      switch (aEvent.type) {
        case "MozSwipeGesture":
          if (this._onSwipe(aEvent)) {
            let event = document.createEvent("Events");
            event.initEvent("CancelTouchSequence", true, true);
            aEvent.target.dispatchEvent(event);
          }
          break;

        
        













        case "CancelTouchSequence":
          this.cancelPending();
          break;
      }
    } catch (e) {
      Util.dumpLn("Error while handling gesture event", aEvent.type,
                  "\nPlease report error at:", e);
      Cu.reportError(e);
    }
  },

  



  cancelPending: function cancelPending() {
    if (AnimatedZoom.isZooming())
      AnimatedZoom.finish();
  },

  _onSwipe: function _onSwipe(aEvent) {
    switch (aEvent.direction) {
      case Ci.nsIDOMSimpleGestureEvent.DIRECTION_LEFT:
        return this._tryCommand("cmd_forward");
      case Ci.nsIDOMSimpleGestureEvent.DIRECTION_RIGHT:
        return this._tryCommand("cmd_back");
    }
    return false;
  },

  _tryCommand: function _tryCommand(aId) {
     if (document.getElementById(aId).getAttribute("disabled") == "true")
       return false;
     CommandUpdater.doCommand(aId);
     return true;
  },

  _pinchStart: function _pinchStart(aEvent) {
    if (AnimatedZoom.isZooming())
      return;
    
    
    let event = document.createEvent("Events");
    event.initEvent("CancelTouchSequence", true, true);
    let success = aEvent.target.dispatchEvent(event);

    if (!success || !Browser.selectedTab.allowZoom)
      return;

    AnimatedZoom.start();
    this._pinchDelta = 0;

    

    
    this._maxGrowth = Services.prefs.getIntPref("browser.ui.pinch.maxGrowth");
    this._maxShrink = Services.prefs.getIntPref("browser.ui.pinch.maxShrink");
    this._scalingFactor = Services.prefs.getIntPref("browser.ui.pinch.scalingFactor");

    
    this._browserBCR = getBrowser().getBoundingClientRect();
    this._pinchStartX = aEvent.clientX - this._browserBCR.left;
    this._pinchStartY = aEvent.clientY - this._browserBCR.top;
  },

  _pinchUpdate: function _pinchUpdate(aEvent) {
    if (!AnimatedZoom.isZooming() || !aEvent.delta)
      return;

    let delta = 0;
    let browser = AnimatedZoom.browser;
    let oldScale = browser.scale;
    let bcr = this._browserBCR;

    
    this._pinchDelta += aEvent.delta;
    if (Math.abs(this._pinchDelta) >= oldScale) {
      delta = this._pinchDelta;
      this._pinchDelta = 0;
    }

    
    delta = Util.clamp(delta, -this._maxShrink, this._maxGrowth);

    let newScale = Browser.selectedTab.clampZoomLevel(oldScale * (1 + delta / this._scalingFactor));
    let startScale = AnimatedZoom.startScale;
    let scaleRatio = startScale / newScale;
    let cX = aEvent.clientX - bcr.left;
    let cY = aEvent.clientY - bcr.top;

    
    let rect = AnimatedZoom.zoomFrom.clone();
    rect.translate(this._pinchStartX - cX + (1-scaleRatio) * cX * rect.width / bcr.width,
                   this._pinchStartY - cY + (1-scaleRatio) * cY * rect.height / bcr.height);

    rect.width *= scaleRatio;
    rect.height *= scaleRatio;

    this.translateInside(rect, new Rect(0, 0, browser.contentDocumentWidth * startScale,
                                              browser.contentDocumentHeight * startScale));

    
    AnimatedZoom.updateTo(rect);
  },

  _pinchEnd: function _pinchEnd(aEvent) {
    if (AnimatedZoom.isZooming())
      AnimatedZoom.finish();
  },

  




  translateInside: function translateInside(r0, r1) {
    let offsetX = (r0.left < r1.left ? r1.left - r0.left :
        (r0.right > r1.right ? Math.max(r1.left - r0.left, r1.right - r0.right) : 0));
    let offsetY = (r0.top < r1.top ? r1.top - r0.top :
        (r0.bottom > r1.bottom ? Math.max(r1.top - r0.top, r1.bottom - r0.bottom) : 0));
    return r0.translate(offsetX, offsetY);
  }
};





var InputSourceHelper = {
  isPrecise: false,
  treatMouseAsTouch: false,

  init: function ish_init() {
    
    try {
      this.treatMouseAsTouch = Services.prefs.getBoolPref(kDebugMouseInputPref);
    } catch (e) {}
    if (!this.treatMouseAsTouch) {
      window.addEventListener("mousemove", this, true);
      window.addEventListener("mousedown", this, true);
    }
  },
  
  handleEvent: function ish_handleEvent(aEvent) {
    switch (aEvent.mozInputSource) {
      case Ci.nsIDOMMouseEvent.MOZ_SOURCE_MOUSE:
      case Ci.nsIDOMMouseEvent.MOZ_SOURCE_PEN:
      case Ci.nsIDOMMouseEvent.MOZ_SOURCE_ERASER:
      case Ci.nsIDOMMouseEvent.MOZ_SOURCE_CURSOR:
        if (!this.isPrecise && !this.treatMouseAsTouch) {
          this.isPrecise = true;
          this._fire("MozPrecisePointer");
        }
        break;

      case Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH:
        if (this.isPrecise) {
          this.isPrecise = false;
          this._fire("MozImprecisePointer");
        }
        break;
    }
  },
  
  fireUpdate: function fireUpdate() {
    if (this.treatMouseAsTouch) {
      this._fire("MozImprecisePointer");
    } else {
      if (this.isPrecise) {
        this._fire("MozPrecisePointer");
      } else {
        this._fire("MozImprecisePointer");
      }
    }
  },

  _fire: function (name) {
    let event = document.createEvent("Events");
    event.initEvent(name, true, true);
    window.dispatchEvent(event);
  }
};
