












































const kDoubleClickInterval = 400;


const kTapRadius = 25;


const kStateActive = 0x00000001;





























































function InputHandler(browserViewContainer) {
  
  this._modules = [];

  
  this._grabber = null;
  this._grabDepth = 0;

  
  this._ignoreEvents = false;

  
  this._suppressNextClick = false;

  
  this.listenFor(window, "mousedown");
  this.listenFor(window, "mouseup");
  this.listenFor(window, "mousemove");
  this.listenFor(window, "click");

  
  this.listenFor(browserViewContainer, "keydown");
  this.listenFor(browserViewContainer, "keyup");
  this.listenFor(browserViewContainer, "DOMMouseScroll");
  this.listenFor(browserViewContainer, "MozMousePixelScroll");

  this.addModule(new MouseModule(this, browserViewContainer));
  this.addModule(new ScrollwheelModule(this, browserViewContainer));
}


InputHandler.prototype = {
  



  listenFor: function listenFor(target, eventType) {
    target.addEventListener(eventType, this, true);
  },

  



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
    if (grabber == null) {
      this._grabber = null;
      this._grabDepth = -1;   
    }

    if (!this._grabber || this._grabber == grabber) {

      if (!this._grabber) {
        
        let mods = this._modules;
        for (let i = 0, len = mods.length; i < len; ++i)
          if (mods[i] != grabber)
            mods[i].cancelPending();
      }

      this._grabber = grabber;
      this._grabDepth++;
      return true;
    }

    return false;
  },

  














  
  
  
  ungrab: function ungrab(grabber, restoreEventInfos) {
    if (this._grabber == null && grabber == null) {
      this._grabber = null;
      this._grabDepth = 1;  
    }

    if (this._grabber == grabber) {  
      this._grabDepth--;

      if (this._grabDepth == 0) {    
        this._grabber = null;

        if (restoreEventInfos) {
          let mods = this._modules;
          let grabberIndex = 0;

          for (let i = 0, len = mods.length; i < len; ++i) {
            if (mods[i] == grabber) {
              grabberIndex = i;      
              break;
            }
          }

          for (i = 0, len = restoreEventInfos.length; i < len; ++i)
            this._passToModules(restoreEventInfos[i], grabberIndex + 1);
        }
      }
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

    
    if (aEvent.view != window)
      return;

    if (this._suppressNextClick && aEvent.type == "click") {
      this._suppressNextClick = false;
      aEvent.stopPropagation();
      aEvent.preventDefault();
      return;
    }

    this._passToModules(new InputHandler.EventInfo(aEvent));
  },

  



  _passToModules: function _passToModules(evInfo, skipToIndex) {
    if (this._grabber) {
      this._grabber.handleEvent(evInfo);
    } else {
      let mods = this._modules;
      let i = skipToIndex || 0;

      for (let len = mods.length; i < len; ++i) {
        mods[i].handleEvent(evInfo);  
        if (this._grabbed)            
          break;
      }
    }
  }
};





InputHandler.EventInfo = function EventInfo(aEvent, timestamp) {
  this.event = aEvent;
  this.time = timestamp || Date.now();
};

InputHandler.EventInfo.prototype = {
  toString: function toString() {
    return '[EventInfo] { event=' + this.event + 'time=' + this.time + ' }';
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
  this._kinetic = new KineticController(Util.bind(this._dragBy, this),
                                        Util.bind(this._kineticStop, this));
}


MouseModule.prototype = {
  handleEvent: function handleEvent(evInfo) {
    if (evInfo.event.button !== 0) 
      return;

    switch (evInfo.event.type) {
      case "mousedown":
        this._onMouseDown(evInfo);
        break;
      case "mousemove":
        this._onMouseMove(evInfo);
        break;
      case "mouseup":
        this._onMouseUp(evInfo);
        break;
    }
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

  








  _onMouseDown: function _onMouseDown(evInfo) {
    this._owner.allowClicks();

    let dragData = this._dragData;
    if (dragData.dragging) {
      
      let [sX, sY] = dragData.panPosition();
      this._doDragStop(sX, sY, !dragData.isPan());
    }
    dragData.reset();

    
    
    let [targetScrollbox, targetScrollInterface]
      = this.getScrollboxFromElement(evInfo.event.target);

    
    let oldInterface = this._targetScrollInterface;
    if (this._kinetic.isActive() && targetScrollInterface != oldInterface)
      this._kinetic.end();

    let targetClicker = this.getClickerFromElement(evInfo.event.target);

    this._targetScrollInterface = targetScrollInterface;
    this._dragger = (targetScrollInterface) ? (targetScrollbox.customDragger || this._defaultDragger)
                                            : null;
    this._clicker = (targetClicker) ? targetClicker.customClicker : null;

    this._owner.grab(this);

    if (this._clicker)
      this._clicker.mouseDown(evInfo.event.clientX, evInfo.event.clientY);

    if (targetScrollInterface && this._dragger.isDraggable(targetScrollbox, targetScrollInterface)) {
      this._doDragStart(evInfo.event);
    }

    if (this._targetIsContent(evInfo.event)) {
      this._recordEvent(evInfo);
    }
    else {
      if (this._clickTimeout) {
        
        window.clearTimeout(this._clickTimeout);
        this._cleanClickBuffer();
      }

      if (targetScrollInterface) {
        
        let cX = {}, cY = {};
        targetScrollInterface.getScrolledSize(cX, cY);
        let rect = targetScrollbox.getBoundingClientRect();
        dragData.locked = ((cX.value > rect.width) != (cY.value > rect.height));
      }
    }
  },

  








  _onMouseUp: function _onMouseUp(evInfo) {
    let dragData = this._dragData;
    let oldIsPan = dragData.isPan();
    if (dragData.dragging) {
      dragData.setDragPosition(evInfo.event.screenX, evInfo.event.screenY);
      let [sX, sY] = dragData.panPosition();
      this._doDragStop(sX, sY, !dragData.isPan());
    }

    if (this._targetIsContent(evInfo.event)) {
      
      this._recordEvent(evInfo);
      let commitToClicker = this._clicker && dragData.isClick();
      if (commitToClicker)
        
        this._commitAnotherClick();
      else
        
        
        
        this._cleanClickBuffer();
    }
    else if (dragData.isPan()) {
      
      
      
      let generatesClick = evInfo.event.detail;
      if (generatesClick)
        this._owner.suppressNextClick();
    }

    let clicker = this._clicker;
    if (clicker) {
      
      if (!oldIsPan && dragData.isPan())
        clicker.panBegin();
      clicker.mouseUp(evInfo.event.clientX, evInfo.event.clientY);
    }

    this._owner.ungrab(this);
  },

  


  _onMouseMove: function _onMouseMove(evInfo) {
    let dragData = this._dragData;

    if (dragData.dragging) {
      let oldIsPan = dragData.isPan();
      dragData.setDragPosition(evInfo.event.screenX, evInfo.event.screenY);
      evInfo.event.stopPropagation();
      evInfo.event.preventDefault();
      if (dragData.isPan()) {
        
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
    while (target) {
      if (target === window)
        return false;
      if (target === this._browserViewContainer)
        return true;

      target = target.parentNode;
    }
    return false;
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
      this._clickTimeout = window.setTimeout(function _clickTimeout(self) { self._doSingleClick(); },
                                             kDoubleClickInterval, this);
    }
  },

  


  _doSingleClick: function _doSingleClick() {
    let ev = this._downUpEvents[1].event;
    this._cleanClickBuffer(2);

    
    let modifiers =
      (ev.altKey   ? Ci.nsIDOMNSEvent.ALT_MASK     : 0) |
      (ev.ctrlKey  ? Ci.nsIDOMNSEvent.CONTROL_MASK : 0) |
      (ev.shiftKey ? Ci.nsIDOMNSEvent.SHIFT_MASK   : 0) |
      (ev.metaKey  ? Ci.nsIDOMNSEvent.META_MASK    : 0);
    this._clicker.singleClick(ev.clientX, ev.clientY, modifiers);
  },

  


  _doDoubleClick: function _doDoubleClick() {
    let mouseUp1 = this._downUpEvents[1].event;
    let mouseUp2 = this._downUpEvents[3].event;
    this._cleanClickBuffer(4);
    this._clicker.doubleClick(mouseUp1.clientX, mouseUp1.clientY,
                              mouseUp2.clientX, mouseUp2.clientY);
  },

  



  _recordEvent: function _recordEvent(evInfo) {
    this._downUpEvents.push(evInfo);
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
      return sX.value > rect.width || sY.value > rect.height;
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
    let prev = null;

    for (; elem; elem = elem.parentNode) {
      try {
        if (elem.ignoreDrag) {
          prev = elem;
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
        }
      } catch (e) { 

 }
      prev = elem;
    }
    return [scrollbox, qinterface, prev];
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
      if (this._isPan) {
        
        let target = document.documentElement;
        let state = this._domUtils.getContentState(target);
        this._domUtils.setContentState(target, state & kStateActive);
      }
    }

    
    if (this._isPan) {
      let absX = Math.abs(this._originX - sX);
      let absY = Math.abs(this._originY - sY);

      
      if (this.lockedX && absX > 3 * absY)
        this.lockedX = null;
      else if (this.lockedY && absY > 3 * absX)
        this.lockedY = null;

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
  },

  endDrag: function endDrag() {
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

  toString: function toString() {
    return '[DragData] { sX,sY=' + this.sX + ',' + this.sY + ', dragging=' + this.dragging + ' }';
  }
};














function KineticController(aPanBy, aEndCallback) {
  this._panBy = aPanBy;
  this._timer = null;
  this._beforeEnd = aEndCallback;

  
  
  this._position = new Point(0, 0);
  this._velocity = new Point(0, 0);
  this._acceleration = new Point(0, 0);
  this._time = 0;
  this._timeStart = 0;

  
  
  this._updateInterval = gPrefService.getIntPref("browser.ui.kinetic.updateInterval");
  
  this._decelerationRate = gPrefService.getIntPref("browser.ui.kinetic.decelerationRate") / 10000;
  
  this._speedSensitivity = gPrefService.getIntPref("browser.ui.kinetic.speedSensitivity") / 100;
  
  this._swipeLength = gPrefService.getIntPref("browser.ui.kinetic.swipeLength");

  this._reset();
}

KineticController.prototype = {
  _reset: function _reset() {
    if (this._timer != null) {
      this._timer.cancel();
      this._timer = null;
    }

    this.momentumBuffer = [];
    this._velocity.set(0, 0);
  },

  isActive: function isActive() {
    return (this._timer != null);
  },

  _startTimer: function _startTimer() {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    let lastx = this._position;  
    let v0 = this._velocity;  
    let a = this._acceleration;  

    
    let aBin = new Point(0, 0);
    let v0Bin = new Point(0, 0);

    let callback = {
      _self: this,
      notify: function kineticTimerCallback(timer) {
        let self = this._self;

        if (!self.isActive())  
          return;

        
        
        
        let realt = Date.now() - self._initialTime;
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
      }
    };

    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    
    this._timer.initWithCallback(callback,
                                 this._updateInterval,
                                 this._timer.TYPE_REPEATING_SLACK);
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
    this._initialTime = Date.now();
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





function ScrollwheelModule(owner, browserViewContainer) {
  this._owner = owner;
  this._browserViewContainer = browserViewContainer;
}

ScrollwheelModule.prototype = {
  pendingEvent : 0,
  handleEvent: function handleEvent(evInfo) {
    if (evInfo.event.type == "DOMMouseScroll" || evInfo.event.type == "MozMousePixelScroll") {
      



      if (this.pendingEvent)
        clearTimeout(this.pendingEvent);
      this.pendingEvent = setTimeout(this.handleEventImpl, 0, evInfo.event.detail);
      evInfo.event.stopPropagation();
      evInfo.event.preventDefault();
    }
  },

  handleEventImpl: function handleEventImpl(zoomlevel) {
    this.pendingEvent = 0;
    Browser.zoom(zoomlevel);
  },

  
  cancelPending: function cancelPending() {}
};
