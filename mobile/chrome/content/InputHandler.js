









































































































function InputHandler(browserViewContainer) {
  
  this._modules = [];

  
  this._grabber = null;
  this._grabDepth = 0;

  
  this._ignoreEvents = false;

  
  this._suppressNextClick = true;

  
  this.listenFor(window, "URLChanged");
  this.listenFor(window, "TabSelect");

  
  this.listenFor(window, "mouseout");

  
  this.listenFor(window, "mousedown");
  this.listenFor(window, "mouseup");
  this.listenFor(window, "mousemove");
  this.listenFor(window, "click");

  
  this.listenFor(browserViewContainer, "keydown");
  this.listenFor(browserViewContainer, "keyup");
  this.listenFor(browserViewContainer, "DOMMouseScroll");

  

  this.addModule(new MouseModule(this));
  this.addModule(new ScrollwheelModule(this, Browser._browserView, browserViewContainer));
  
  
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

    
    if (aEvent.type == "URLChanged" || aEvent.type == "TabSelect") {
      this.grab(null);
      return;
    }

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





















































function MouseModule(owner) {
  this._owner = owner;
  this._dragData = new DragData(this, 50, 200);

  this._dragger = null;
  this._clicker = null;

  this._downUpEvents = [];
  this._downUpDispatchedIndex = 0;
  this._targetScrollInterface = null;

  this._dragging = false;
  this._fastPath = false;

  var self = this;
  this._kinetic = new KineticController(
    function _dragByBound(dx, dy) { return self._dragBy(dx, dy); },
    function _dragStopBound() { return self._doDragStop(0, 0, true); }
  );
}


MouseModule.prototype = {
  handleEvent: function handleEvent(evInfo) {
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
  },

  








  _onMouseDown: function _onMouseDown(evInfo) {
    this._owner.allowClicks();
    if (this._kinetic.isActive())
      this._kinetic.end();

    
    
    let [targetScrollbox, targetScrollInterface]
      = this.getScrollboxFromElement(evInfo.event.target);

    let targetClicker = this.getClickerFromElement(evInfo.event.target);

    this._targetScrollInterface = targetScrollInterface;
    if (!targetScrollInterface) { debugger; dump('*********** no TSI found, dragger will be null\n'); }
    this._dragger = (targetScrollInterface) ? (targetScrollbox.customDragger || this._defaultDragger)
                                            : null;
    this._clicker = (targetClicker) ? targetClicker.customClicker : null;

    evInfo.event.stopPropagation();
    evInfo.event.preventDefault();

    this._owner.grab(this);

    if (targetScrollInterface) {
      this._doDragStart(evInfo.event.screenX, evInfo.event.screenY);
    }

    this._recordEvent(evInfo);
  },

  








  _onMouseUp: function _onMouseUp(evInfo) {
    let dragData = this._dragData;

    evInfo.event.stopPropagation();
    evInfo.event.preventDefault();

    
    
    this._owner.suppressNextClick();

    let [sX, sY] = [evInfo.event.screenX, evInfo.event.screenY];

    let movedOutOfRadius = dragData.isPointOutsideRadius(sX, sY);

    if (dragData.dragging)
      this._doDragStop(sX, sY);

    dragData.reset();

    this._recordEvent(evInfo);

    this._doClick(movedOutOfRadius);

    this._owner.ungrab(this);
  },

  


  _onMouseMove: function _onMouseMove(evInfo) {
    let dragData = this._dragData;

    if (dragData.dragging) {
      evInfo.event.stopPropagation();
      evInfo.event.preventDefault();
      this._doDragMove(evInfo.event.screenX, evInfo.event.screenY);
    }
  },

  



  _recordEvent: function _recordEvent(evInfo) {
    this._downUpEvents.push(evInfo);
  },

  



  _redispatchDownUpEvents: function _redispatchDownUpEvents() {
    let evQueue = this._downUpEvents;

    this._owner.stopListening();

    let len = evQueue.length;

    for (let i = this._downUpDispatchedIndex; i < len; ++i)
      this._redispatchChromeMouseEvent(evQueue[i].event);

    this._downUpDispatchedIndex = len;

    this._owner.startListening();
  },

  



  _redispatchChromeMouseEvent: function _redispatchChromeMouseEvent(aEvent) {
    if (!(aEvent instanceof MouseEvent)) {
      Cu.reportError("_redispatchChromeMouseEvent called with a non-mouse event");
      return;
    }

    
    Browser.windowUtils.sendMouseEvent(aEvent.type, aEvent.clientX, aEvent.clientY,
                                       aEvent.button, aEvent.detail, 0, true);
  },

  





  _clearDownUpEvents: function _clearDownUpEvents() {
    this._downUpEvents.splice(0);
    this._downUpDispatchedIndex = 0;
  },

  


  _doDragStart: function _doDragStart(sX, sY) {
    let dragData = this._dragData;

    dragData.setDragStart(sX, sY);
    this._kinetic.addData(sX, sY);

    this._dragger.dragStart(this._targetScrollInterface);
  },

  




  _doDragStop: function _doDragStop(sX, sY, kineticStop) {
    if (!kineticStop) {    
                           
      let dragData = this._dragData;

      let dx = dragData.sX - sX;
      let dy = dragData.sY - sY;

      dragData.reset();
      this._kinetic.addData(sX, sY);

      this._kinetic.start();

    } else {               
      try {
      this._dragger.dragStop(0, 0, this._targetScrollInterface);
      } catch (e) {
        debugger; dump(e + '\n');
      }
    }
  },

  


  _doDragMove: function _doDragMove(sX, sY) {
    let dragData = this._dragData;
    let dX = dragData.sX - sX;
    let dY = dragData.sY - sY;
    this._kinetic.addData(sX, sY);
    return this._dragBy(dX, dY);
  },

  





  _dragBy: function _dragBy(dX, dY) {
    let dragData = this._dragData;
    let sX = dragData.sX - dX;
    let sY = dragData.sY - dY;

    dragData.setDragPosition(sX, sY);

    return this._dragger.dragMove(dX, dY, this._targetScrollInterface);
  },

  





  _doClick: function _doClick(movedOutOfRadius) {
    let commitToClicker = this._clicker && !movedOutOfRadius;

    if (commitToClicker) {
      this._commitAnotherClick();  
    }

    this._redispatchDownUpEvents();

    if (!commitToClicker) {
      this._cleanClickBuffer();    
                                   
    }                              
  },

  






  _commitAnotherClick: function _commitAnotherClick() {
    const doubleClickInterval = 400;

    if (this._clickTimeout) {   
      window.clearTimeout(this._clickTimeout);
      this._doDoubleClick();
    } else {
      this._clickTimeout = window.setTimeout(function _clickTimeout(self) { self._doSingleClick(); },
                                             doubleClickInterval, this);
    }
  },

  


  _doSingleClick: function _doSingleClick() {
    






    let ev = this._downUpEvents[1].event;
    this._cleanClickBuffer();
    this._clicker.singleClick(ev.clientX, ev.clientY);
  },

  


  _doDoubleClick: function _doDoubleClick() {
    






    let mouseUp1 = this._downUpEvents[1].event;
    let mouseUp2 = this._downUpEvents[3].event;
    this._cleanClickBuffer();
    this._clicker.doubleClick(mouseUp1.clientX, mouseUp1.clientY,
                              mouseUp2.clientX, mouseUp2.clientY);
  },

  




  _cleanClickBuffer: function _cleanClickBuffer() {
    delete this._clickTimeout;
    this._clearDownUpEvents();
  },

  




  _defaultDragger: {
    dragStart: function dragStart(scroller) {},

    dragStop : function dragStop(dx, dy, scroller)
    { return this.dragMove(dx, dy, scroller); },

    dragMove : function dragMove(dx, dy, scroller) {
      if (scroller.getPosition) {
        let oldX = {}, oldY = {};
        scroller.getPosition(oldX, oldY);

        scroller.scrollBy(dx, dy);

        let newX = {}, newY = {};
        scroller.getPosition(newX, newY);

        return (newX.value != oldX.value) || (newY.value != oldY.value);
      } else {
        scroller.scrollBy(dx, dy);
        
        return true;
      }
    }
  },

  
  

  







  getScrollboxFromElement: function getScrollboxFromElement(elem) {
    let scrollbox = null;
    let qinterface = null;

    for (; elem; elem = elem.parentNode) { dump(elem + '\n');
      try {

        if (elem.scrollBoxObject) {

          scrollbox = elem;
          qinterface = elem.scrollBoxObject;
          break;
        } else if (elem.boxObject) {

          let qi = (elem._cachedSBO) ? elem._cachedSBO
                                     : elem.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
          if (qi) {
            scrollbox = elem;
            elem._cachedSBO = qinterface = qi;
            break;
          }
        }
      } catch (e) { 

 }
    }
    return [scrollbox, qinterface];
  },

  



  getClickerFromElement: function getClickerFromElement(elem) {
    for (; elem; elem = elem.parentNode)
      if (elem.customClicker)
        break;

    return (elem) ? elem : null;
  }

};





function DragData(owner, dragRadius, dragStartTimeoutLength) {
  this._owner = owner;
  this._dragRadius = dragRadius;
  this.reset();
};

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










function KineticController(aPanBy, aEndCallback) {
  this._panBy = aPanBy;
  this._timer = null;
  this._beforeEnd = aEndCallback;

  try {
    this._updateInterval = gPrefService.getIntPref("browser.ui.kinetic.updateInterval");
    
    this._emaAlpha = gPrefService.getIntPref("browser.ui.kinetic.ema.alphaValue") / 10;
    
    this._decelerationRate = gPrefService.getIntPref("browser.ui.kinetic.decelerationRate") / 100;
  }
  catch (e) {
    this._updateInterval = 33;
    this._emaAlpha = .8;
    this._decelerationRate = .15;
  };

  this._reset();
}

KineticController.prototype = {
  _reset: function _reset() {
    if (this._timer != null) {
      this._timer.cancel();
      this._timer = null;
    }

    this.momentumBuffer = [];
    this._speedX = 0;
    this._speedY = 0;
  },

  isActive: function isActive() {
    return (this._timer != null);
  },

  _startTimer: function _startTimer() {
    let callback = {
      _self: this,
      notify: function kineticTimerCallback(timer) {
        let self = this._self;

        if (!self.isActive())  
          return;

        

        if (self._speedX == 0 && self._speedY == 0) {
          self.end();
          return;
        }
        let dx = Math.round(self._speedX * self._updateInterval);
        let dy = Math.round(self._speedY * self._updateInterval);
        

        let panned = false;
        try { panned = self._panBy(-dx, -dy); } catch (e) {}
        if (!panned) {
          self.end();
          return;
        }

        if (self._speedX < 0) {
          self._speedX = Math.min(self._speedX + self._decelerationRate, 0);
        } else if (self._speedX > 0) {
          self._speedX = Math.max(self._speedX - self._decelerationRate, 0);
        }
        if (self._speedY < 0) {
          self._speedY = Math.min(self._speedY + self._decelerationRate, 0);
        } else if (self._speedY > 0) {
          self._speedY = Math.max(self._speedY - self._decelerationRate, 0);
        }

        if (self._speedX == 0 && self._speedY == 0)
          self.end();
      }
    };

    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    
    this._timer.initWithCallback(callback,
                                 this._updateInterval,
                                 this._timer.TYPE_REPEATING_SLACK);
  },

  start: function start() {
    let mb = this.momentumBuffer;
    let mblen = this.momentumBuffer.length;

    
    if (mblen < 2) {
      this.end();
      return false;
    }

    function ema(currentSpeed, lastSpeed, alpha) {
      return alpha * currentSpeed + (1 - alpha) * lastSpeed;
    };

    
    let prev = mb[0];
    for (let i = 1; i < mblen; i++) {
      let me = mb[i];

      let timeDiff = me.t - prev.t;

      this._speedX = ema( ((me.sx - prev.sx) / timeDiff), this._speedX, this._emaAlpha);
      this._speedY = ema( ((me.sy - prev.sy) / timeDiff), this._speedY, this._emaAlpha);

      prev = me;
    }

    
    this._startTimer();

    return true;
  },

  end: function end() {
    this._beforeEnd();
    this._reset();
  },

  addData: function addData(sx, sy) {
    
    if (this.isActive())
      this.end();

    let mbLength = this.momentumBuffer.length;
    
    let now = Date.now();

    if (mbLength > 0) {
      let mbLast = this.momentumBuffer[mbLength - 1];
      if ((mbLast.sx == sx && mbLast.sy == sy) || mbLast.t == now)
        return;
    }

    this.momentumBuffer.push({'t': now, 'sx' : sx, 'sy' : sy});
  }
};


function ContentPanningModule(owner, browserCanvas, useEarlyMouseMoves) {
  this._owner = owner;
  this._browserCanvas = browserCanvas;
  this._dragData = new DragData(this, 50, 200);
  this._useEarlyMouseMoves = useEarlyMouseMoves;

  var self = this;
  this._kinetic = new KineticController( function (dx, dy) { return self._dragBy(dx, dy); } );
}

ContentPanningModule.prototype = {
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
      case "mouseout":
      case "mouseup":
        this._onMouseUp(aEvent);
        break;
    }
  },


  


  cancelPending: function cancelPending() {
    this._kinetic.end();
    this._dragData.reset();
  },

  _dragStart: function _dragStart(sX, sY) {
    let dragData = this._dragData;

    dragData.setDragStart(sX, sY);

    [sX, sY] = dragData.lockAxis(sX, sY);

    

    
  },

  _dragStop: function _dragStop(sX, sY) {
    let dragData = this._dragData;

    this._owner.ungrab(this);

    [sX, sY] = dragData.lockMouseMove(sX, sY);

    
    this._kinetic.start(sX, sY);

    dragData.reset();
  },

  _dragBy: function _dragBy(dx, dy) {
    



    return false;
  },

  _dragMove: function _dragMove(sX, sY) {
    let dragData = this._dragData;
    [sX, sY] = dragData.lockMouseMove(sX, sY);
    
    let panned = false;
    dragData.setDragPosition(sX, sY);
    return panned;
  },

  _onMouseDown: function _onMouseDown(aEvent) {
    let dragData = this._dragData;
    
    if (this._kinetic.isActive()) {
      this._kinetic.end();
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
    
    if (this._kinetic.isActive())
      return;

    let dragData = this._dragData;

    
    if (dragData.isPointOutsideRadius(aEvent.screenX, aEvent.screenY))
      this._owner.grab(this);

    
    if (!dragData.sX)
      dragData.setDragPosition(aEvent.screenX, aEvent.screenY);

    let [sX, sY] = dragData.lockMouseMove(aEvent.screenX, aEvent.screenY);

    
    
    if (this._useEarlyMouseMoves || dragData.dragging)
      this._kinetic.addData(sX, sY);

    if (dragData.dragging)
      this._dragMove(sX, sY);
  }
};





function ContentClickingModule(owner) {
  this._owner = owner;
  this._clickTimeout = -1;
  this._events = [];
  this._zoomedTo = null;
}

ContentClickingModule.prototype = {
  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      
      case "mousedown":
        this._events.push({event: aEvent, time: Date.now()});


      case "mouseup":
        
        if (!(this._events.length % 2)) {
          this._reset();
            break;
        }

        this._events.push({event: aEvent, time: Date.now()});

        if (this._clickTimeout == -1) {
          this._clickTimeout = window.setTimeout(function _clickTimeout(self) { self._sendSingleClick(); }, 400, this);
        } else {
          window.clearTimeout(this._clickTimeout);
          this._clickTimeout = -1;
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
      return element;
    }

    let firstEvent = this._events[0].event;
    let zoomElement = optimalElementForPoint(firstEvent.clientX, firstEvent.clientY);

    if (zoomElement) {
      if (zoomElement != this._zoomedTo) {
        this._zoomedTo = zoomElement;
        Browser.canvasBrowser.zoomToElement(zoomElement);
      } else {
        this._zoomedTo = null;
        Browser.canvasBrowser.zoomFromElement(zoomElement);
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






function ScrollwheelModule(owner, browserView, browserViewContainer) {
  this._owner = owner;
  this._browserView = browserView;
  this._browserViewContainer = browserViewContainer;
}

ScrollwheelModule.prototype = {
  handleEvent: function handleEvent(evInfo) {
    
      if (evInfo.event.type == "DOMMouseScroll") {
        dump('got scrollwheel event on target ' + evInfo.event.target + '\n');
        this._browserView.zoom(evInfo.event.detail);
        evInfo.event.stopPropagation();
        evInfo.event.preventDefault();
      }
    
  },

  
  cancelPending: function cancelPending() {}
};
