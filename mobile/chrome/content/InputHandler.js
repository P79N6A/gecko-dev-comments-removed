







































































































function InputHandler(browserViewContainer) {
  
  this._modules = [];

  
  this._grabber = null;
  this._grabDepth = 0;

  
  this._ignoreEvents = false;

  
  this._suppressNextClick = true;

  
  this.listenFor(window, "URLChanged");
  this.listenFor(window, "TabSelect");

  
  this.listenFor(window, "mousedown");
  this.listenFor(window, "mouseup");
  this.listenFor(window, "mousemove");
  this.listenFor(window, "click");

  
  this.listenFor(browserViewContainer, "keydown");
  this.listenFor(browserViewContainer, "keyup");
  this.listenFor(browserViewContainer, "DOMMouseScroll");

  

  this.addModule(new MouseModule(this));
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

InputHandler.EventInfo.prototype = {
  toString: function toString() {
    return '[EventInfo] { event=' + this.event + 'time=' + this.time + ' }';
  }
};






























































function MouseModule(owner) {
  this._owner = owner;
  this._dragData = new DragData(this, 15, 200);

  this._dragger = null;
  this._clicker = null;

  this._downUpEvents = [];
  this._downUpDispatchedIndex = 0;
  this._targetScrollInterface = null;

  var self = this;
  this._kinetic = new KineticController(
    function _dragByBound(dx, dy) { return self._dragBy(dx, dy); },
    function _dragStopBound() { return self._doDragStop(0, 0, true); }
  );
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
    if (this._kinetic.isActive())
      this._kinetic.end();

    
    
    let [targetScrollbox, targetScrollInterface]
      = this.getScrollboxFromElement(evInfo.event.target);

    let targetClicker = this.getClickerFromElement(evInfo.event.target);

    this._movedOutOfRadius = false;
    this._targetScrollInterface = targetScrollInterface;
    this._dragger = (targetScrollInterface) ? (targetScrollbox.customDragger || this._defaultDragger)
                                            : null;
    this._clicker = (targetClicker) ? targetClicker.customClicker : null;

    if (this._dragger && !this._dragger.allowRealtimeDownUp) {
      evInfo.event.stopPropagation();
      evInfo.event.preventDefault();
    }

    this._owner.grab(this);

    if (targetScrollInterface) {
      this._doDragStart(evInfo.event);
    }

    this._recordEvent(evInfo);
  },

  








  _onMouseUp: function _onMouseUp(evInfo) {
    let dragData = this._dragData;

    if (this._dragger && !this._dragger.allowRealtimeDownUp) {
      evInfo.event.stopPropagation();
      evInfo.event.preventDefault();

      
      
      this._owner.suppressNextClick();
    }

    let [sX, sY] = dragData.lockAxis(evInfo.event.screenX, evInfo.event.screenY);

    this._movedOutOfRadius = this._movedOutOfRadius || dragData.isPointOutsideRadius(sX, sY);

    if (dragData.dragging)       
      this._doDragStop(sX, sY);  

    this._recordEvent(evInfo);

    this._doClick(this._movedOutOfRadius);

    this._owner.ungrab(this);
  },

  


  _onMouseMove: function _onMouseMove(evInfo) {
    let dragData = this._dragData;

    if (dragData.dragging) {
      let [sX, sY] = dragData.lockAxis(evInfo.event.screenX, evInfo.event.screenY);
      evInfo.event.stopPropagation();
      evInfo.event.preventDefault();
      this._doDragMove(sX, sY);
    }

    this._movedOutOfRadius = this._movedOutOfRadius || 
      dragData.isPointOutsideRadius(evInfo.event.screenX, evInfo.event.screenY);
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

  _skipAllDownUpEvents: function _skipAllDownUpEvents() {
    this._downUpDispatchedIndex = this._downUpEvents.length;
  },

  



  _redispatchChromeMouseEvent: function _redispatchChromeMouseEvent(aEvent) {
    if (!(aEvent instanceof MouseEvent)) {
      Cu.reportError("_redispatchChromeMouseEvent called with a non-mouse event");
      return;
    }

    
    Browser.windowUtils.sendMouseEvent(aEvent.type, aEvent.clientX, aEvent.clientY,
                                       aEvent.button, aEvent.detail, 0, true);
  },

  








  _clearDownUpEvents: function _clearDownUpEvents(howMany) {
    if (howMany === undefined)
      howMany = this._downUpEvents.length;

    this._downUpEvents.splice(0, howMany);
    this._downUpDispatchedIndex = Math.max(this._downUpDispatchedIndex - howMany, 0);
  },

  


  _doDragStart: function _doDragStart(event) {
    let dragData = this._dragData;

    dragData.setDragStart(event.screenX, event.screenY);
    this._kinetic.addData(event.screenX, event.screenY);

    this._dragger.dragStart(event.clientX, event.clientY, event.target, this._targetScrollInterface);
  },

  




  _doDragStop: function _doDragStop(sX, sY, kineticStop) {
    let dragData = this._dragData;

    if (!kineticStop) {    
                           
      let dx = dragData.sX - sX;
      let dy = dragData.sY - sY;

      dragData.endDrag();

      this._kinetic.addData(sX, sY);

      this._kinetic.start();
    } else {               
      this._dragger.dragStop(0, 0, this._targetScrollInterface);
      dragData.reset();
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
    let needToRedispatch = this._dragger && !this._dragger.allowRealtimeDownUp && !movedOutOfRadius;

    if (commitToClicker) {
      this._commitAnotherClick();  
    }

    if (needToRedispatch) {
      this._redispatchDownUpEvents();
    } else {
      this._skipAllDownUpEvents();
    }

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
    dump('doing single click with ' + this._downUpEvents.length + '\n');
    for (let i = 0; i < this._downUpEvents.length; ++i)
      dump('      ' + this._downUpEvents[i].event.type
           + " :: " + this._downUpEvents[i].event.button
           + " :: " + this._downUpEvents[i].event.detail + '\n');

    let ev = this._downUpEvents[1].event;
    this._cleanClickBuffer(2);
    this._clicker.singleClick(ev.clientX, ev.clientY);
  },

  


  _doDoubleClick: function _doDoubleClick() {
    dump('doing double click with ' + this._downUpEvents.length + '\n');
    for (let i = 0; i < this._downUpEvents.length; ++i)
      dump('      ' + this._downUpEvents[i].event.type
           + " :: " + this._downUpEvents[i].event.button
           + " :: " + this._downUpEvents[i].event.detail + '\n');

    let mouseUp1 = this._downUpEvents[1].event;
    let mouseUp2 = this._downUpEvents[3].event;
    this._cleanClickBuffer(4);
    this._clicker.doubleClick(mouseUp1.clientX, mouseUp1.clientY,
                              mouseUp2.clientX, mouseUp2.clientY);
  },

  







  _cleanClickBuffer: function _cleanClickBuffer(howMany) {
    delete this._clickTimeout;
    this._clearDownUpEvents(howMany);
  },

  




  _defaultDragger: {
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
      + '\n\tdownUpIndex=' + this._downUpDispatchedIndex + ', '
      + 'length=' + this._downUpEvents.length + ', '
      + '\n\ttargetScroller=' + this._targetScrollInterface + ', '
      + '\n\tclickTimeout=' + this._clickTimeout + '\n  }';
  }
};





function DragData(owner, dragRadius, dragStartTimeoutLength) {
  this._owner = owner;
  this._dragRadius = dragRadius;
  this.reset();
};


const kMsUntilLock = 50;

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
    this.sX = this._originX = screenX;
    this.sY = this._originY = screenY;
    this.dragging = true;
    this._dragStartTime = Date.now();
    this.alreadyLocked = false;
  },

  endDrag: function endDrag() {
    this.dragging = false;
  },

  lockAxis: function lockAxis(sX, sY) {
    if (this.alreadyLocked) {
      if (this.lockedX !== null) {
        sX = this.lockedX;
      }
      else if (this.lockedY !== null) {
        sY = this.lockedY;
      }
      return [sX, sY];
    }
    
    let now = Date.now();
    if (now - this._dragStartTime < kMsUntilLock) {
      
      return [this.sX, this.sY];      
    }
     
    

    
    
    let absX = Math.abs(this.sX - sX);
    let absY = Math.abs(this.sY - sY);
    
    

    
    
    
    
    
    if (absX > 2 * absY) {
      this.lockedY = this.sY;
      sY = this.sY;
    }
    else if (absY > 2 * absX) {
      this.lockedX = this.sX;
      sX = this.sX;
    }

    
    this.alreadyLocked = true;

    return [sX, sY];
  },

  isPointOutsideRadius: function isPointOutsideRadius(sX, sY) {
    if (this._originX === null)
      return false;
    return (Math.pow(sX - this._originX, 2) + Math.pow(sY - this._originY, 2)) >
      (2 * Math.pow(this._dragRadius, 2));
  },

  toString: function toString() {
    return '[DragData] { sX,sY=' + this.sX + ',' + this.sY + ', dragging=' + this.dragging + ' }';
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
    if (this._beforeEnd)
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





function ScrollwheelModule(owner, browserViewContainer) {
  this._owner = owner;
  this._browserViewContainer = browserViewContainer;
}

ScrollwheelModule.prototype = {
  handleEvent: function handleEvent(evInfo) {
    if (evInfo.event.type == "DOMMouseScroll") {
      Browser.zoom(evInfo.event.detail);
      evInfo.event.stopPropagation();
      evInfo.event.preventDefault();
    }
  },

  
  cancelPending: function cancelPending() {}
};
