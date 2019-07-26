















const kDisableOnScrollDistance = 25;


const kDragHysteresisDistance = 10;


const kChromeLayer = 1;
const kContentLayer = 2;





function MarkerDragger(aMarker) {
  this._marker = aMarker;
}

MarkerDragger.prototype = {
  _selectionHelperUI: null,
  _marker: null,
  _shutdown: false,
  _dragging: false,

  get marker() {
    return this._marker;
  },

  set shutdown(aVal) {
    this._shutdown = aVal;
  },

  get shutdown() {
    return this._shutdown;
  },

  get dragging() {
    return this._dragging;
  },

  freeDrag: function freeDrag() {
    return true;
  },

  isDraggable: function isDraggable(aTarget, aContent) {
    return { x: true, y: true };
  },

  dragStart: function dragStart(aX, aY, aTarget, aScroller) {
    if (this._shutdown)
      return false;
    this._dragging = true;
    this.marker.dragStart(aX, aY);
    return true;
  },

  dragStop: function dragStop(aDx, aDy, aScroller) {
    if (this._shutdown)
      return false;
    this._dragging = false;
    this.marker.dragStop(aDx, aDy);
    return true;
  },

  dragMove: function dragMove(aDx, aDy, aScroller, aIsKenetic, aClientX, aClientY) {
    
    
    if (this._shutdown || aIsKenetic)
      return false;
    this.marker.moveBy(aDx, aDy, aClientX, aClientY);
    
    
    return true;
  }
}

function Marker(aParent, aTag, aElementId, xPos, yPos) {
  this._xPos = xPos;
  this._yPos = yPos;
  this._selectionHelperUI = aParent;
  this._element = aParent.overlay.getMarker(aElementId);
  this._elementId = aElementId;
  
  this._element.customDragger = new MarkerDragger(this);
  this.tag = aTag;
}

Marker.prototype = {
  _element: null,
  _elementId: "",
  _selectionHelperUI: null,
  _xPos: 0,
  _yPos: 0,
  _xDrag: 0,
  _yDrag: 0,
  _tag: "",
  _hPlane: 0,
  _vPlane: 0,

  
  _monocleRadius: 8,
  _monocleXHitTextAdjust: -2, 
  _monocleYHitTextAdjust: -10, 

  get xPos() {
    return this._xPos;
  },

  get yPos() {
    return this._yPos;
  },

  get tag() {
    return this._tag;
  },

  set tag(aVal) {
    this._tag = aVal;
  },

  get dragging() {
    return this._element.customDragger.dragging;
  },

  shutdown: function shutdown() {
    this._element.hidden = true;
    this._element.customDragger.shutdown = true;
    delete this._element.customDragger;
    this._selectionHelperUI = null;
    this._element = null;
  },

  setTrackBounds: function setTrackBounds(aVerticalPlane, aHorizontalPlane) {
    
    this._hPlane = aHorizontalPlane;
    this._vPlane = aVerticalPlane;
  },

  show: function show() {
    this._element.hidden = false;
  },

  hide: function hide() {
    this._element.hidden = true;
  },

  get visible() {
    return this._element.hidden == false;
  },

  position: function position(aX, aY) {
    if (aX < 0) {
      Util.dumpLn("Marker: aX is negative");
      aX = 0;
    }
    if (aY < 0) {
      Util.dumpLn("Marker: aY is negative");
      aY = 0;
    }
    this._xPos = aX;
    this._yPos = aY;
    this._setPosition();
  },

  _setPosition: function _setPosition() {
    this._element.left = this._xPos + "px";
    this._element.top = this._yPos + "px";
  },

  dragStart: function dragStart(aX, aY) {
    this._xDrag = 0;
    this._yDrag = 0;
    this._selectionHelperUI.markerDragStart(this);
  },

  dragStop: function dragStop(aDx, aDy) {
    this._selectionHelperUI.markerDragStop(this);
  },

  moveBy: function moveBy(aDx, aDy, aClientX, aClientY) {
    this._xPos -= aDx;
    this._yPos -= aDy;
    this._xDrag -= aDx;
    this._yDrag -= aDy;
    
    
    let direction = "tbd";
    if (Math.abs(this._xDrag) > kDragHysteresisDistance ||
        Math.abs(this._yDrag) > kDragHysteresisDistance) {
      direction = (this._xDrag <= 0 && this._yDrag <= 0 ? "start" : "end");
    }
    
    
    if (this._selectionHelperUI.markerDragMove(this, direction)) {
      this._setPosition();
    }
  },

  hitTest: function hitTest(aX, aY) {
    
    
    aY += this._monocleYHitTextAdjust;
    aX += this._monocleXHitTextAdjust;
    if (aX >= (this._xPos - this._monocleRadius) &&
        aX <= (this._xPos + this._monocleRadius) &&
        aY >= (this._yPos - this._monocleRadius) &&
        aY <= (this._yPos + this._monocleRadius))
      return true;
    return false;
  },

  swapMonocle: function swapMonocle(aCaret) {
    let targetElement = aCaret._element;
    let targetElementId = aCaret._elementId;

    aCaret._element = this._element;
    aCaret._element.customDragger._marker = aCaret;
    aCaret._elementId = this._elementId;

    this._xPos = aCaret._xPos;
    this._yPos = aCaret._yPos;
    this._element = targetElement;
    this._element.customDragger._marker = this;
    this._elementId = targetElementId;
    this._element.visible = true;
  },

};





var SelectionHelperUI = {
  _debugEvents: false,
  _msgTarget: null,
  _startMark: null,
  _endMark: null,
  _caretMark: null,
  _target: null,
  _movement: { active: false, x:0, y: 0 },
  _activeSelectionRect: null,
  _selectionMarkIds: [],
  _targetIsEditable: false,

  



  get startMark() {
    if (this._startMark == null) {
      this._startMark = new Marker(this, "start", this._selectionMarkIds.pop(), 0, 0);
    }
    return this._startMark;
  },

  get endMark() {
    if (this._endMark == null) {
      this._endMark = new Marker(this, "end", this._selectionMarkIds.pop(), 0, 0);
    }
    return this._endMark;
  },

  get caretMark() {
    if (this._caretMark == null) {
      this._caretMark = new Marker(this, "caret", this._selectionMarkIds.pop(), 0, 0);
    }
    return this._caretMark;
  },

  get overlay() {
    return document.getElementById(this.layerMode == kChromeLayer ?
      "chrome-selection-overlay" : "content-selection-overlay");
  },

  get layerMode() {
    if (this._msgTarget && this._msgTarget instanceof SelectionPrototype)
      return kChromeLayer;
    return kContentLayer;
  },

  




  get isActive() {
    return this._msgTarget ? true : false;
  },

  





  get isSelectionUIVisible() {
    if (!this._msgTarget || !this._startMark)
      return false;
    return this._startMark.visible;
  },

  





  get isCaretUIVisible() {
    if (!this._msgTarget || !this._caretMark)
      return false;
    return this._caretMark.visible;
  },

  






  get hasActiveDrag() {
    if (!this._msgTarget)
      return false;
    if ((this._caretMark && this._caretMark.dragging) ||
        (this._startMark && this._startMark.dragging) ||
        (this._endMark && this._endMark.dragging))
      return true;
    return false;
  },


  



  observe: function (aSubject, aTopic, aData) {
  switch (aTopic) {
    case "attach_edit_session_to_content":
      let event = aSubject;
      SelectionHelperUI.attachEditSession(Browser.selectedTab.browser,
                                          event.clientX, event.clientY);
      break;
    }
  },

  



  








  pingSelectionHandler: function pingSelectionHandler() {
    if (!this.isActive)
      return null;

    if (this._pingCount == undefined) {
      this._pingCount = 0;
      this._pingArray = [];
    }

    this._pingCount++;

    let deferred = Promise.defer();
    this._pingArray.push({
      id: this._pingCount,
      deferred: deferred
    });

    this._sendAsyncMessage("Browser:SelectionHandlerPing", { id: this._pingCount });
    return deferred.promise;
  },

  










  openEditSession: function openEditSession(aMsgTarget, aX, aY, aSetFocus) {
    if (!aMsgTarget || this.isActive)
      return;
    this._init(aMsgTarget);
    this._setupDebugOptions();
    let setFocus = aSetFocus || false;
    
    
    
    this._sendAsyncMessage("Browser:SelectionStart", {
      setFocus: setFocus,
      xPos: aX,
      yPos: aY
    });
  },

  







  attachEditSession: function attachEditSession(aMsgTarget, aX, aY) {
    if (!aMsgTarget || this.isActive)
      return;
    this._init(aMsgTarget);
    this._setupDebugOptions();

    
    
    
    this._sendAsyncMessage("Browser:SelectionAttach", {
      xPos: aX,
      yPos: aY
    });
  },

  














  attachToCaret: function attachToCaret(aMsgTarget, aX, aY) {
    if (!this.isActive) {
      this._init(aMsgTarget);
      this._setupDebugOptions();
    } else {
      this._hideMonocles();
    }

    this._lastPoint = { xPos: aX, yPos: aY };

    this._sendAsyncMessage("Browser:CaretAttach", {
      xPos: aX,
      yPos: aY
    });
  },

  




  canHandleContextMenuMsg: function canHandleContextMenuMsg(aMessage) {
    if (aMessage.json.types.indexOf("content-text") != -1)
      return true;
    return false;
  },

  








  closeEditSession: function closeEditSession(aClearSelection) {
    if (!this.isActive) {
      return;
    }
    
    
    let clearSelection = aClearSelection || false;
    this._sendAsyncMessage("Browser:SelectionClose", {
      clearSelection: clearSelection
    });
  },

  



  init: function () {
    Services.obs.addObserver(this, "attach_edit_session_to_content", false);
  },

  _init: function _init(aMsgTarget) {
    
    this._msgTarget = aMsgTarget;

    
    this._setupMonocleIdArray();

    
    this._activeSelectionRect = Util.getCleanRect();
    this._targetElementRect = Util.getCleanRect();

    
    messageManager.addMessageListener("Content:SelectionRange", this);
    messageManager.addMessageListener("Content:SelectionCopied", this);
    messageManager.addMessageListener("Content:SelectionFail", this);
    messageManager.addMessageListener("Content:SelectionDebugRect", this);
    messageManager.addMessageListener("Content:HandlerShutdown", this);
    messageManager.addMessageListener("Content:SelectionHandlerPong", this);

    
    window.addEventListener("keypress", this, true);
    window.addEventListener("MozPrecisePointer", this, true);
    window.addEventListener("MozDeckOffsetChanging", this, true);
    window.addEventListener("MozDeckOffsetChanged", this, true);
    window.addEventListener("KeyboardChanged", this, true);

    
    window.addEventListener("click", this, false);
    window.addEventListener("touchstart", this, false);
    window.addEventListener("touchend", this, false);
    window.addEventListener("touchmove", this, false);

    Elements.browsers.addEventListener("URLChanged", this, true);
    Elements.browsers.addEventListener("SizeChanged", this, true);
    Elements.browsers.addEventListener("ZoomChanged", this, true);

    Elements.navbar.addEventListener("transitionend", this, true);
    Elements.navbar.addEventListener("MozAppbarDismissing", this, true);

    this.overlay.enabled = true;
  },

  _shutdown: function _shutdown() {
    messageManager.removeMessageListener("Content:SelectionRange", this);
    messageManager.removeMessageListener("Content:SelectionCopied", this);
    messageManager.removeMessageListener("Content:SelectionFail", this);
    messageManager.removeMessageListener("Content:SelectionDebugRect", this);
    messageManager.removeMessageListener("Content:HandlerShutdown", this);
    messageManager.removeMessageListener("Content:SelectionHandlerPong", this);

    window.removeEventListener("keypress", this, true);
    window.removeEventListener("click", this, false);
    window.removeEventListener("touchstart", this, false);
    window.removeEventListener("touchend", this, false);
    window.removeEventListener("touchmove", this, false);
    window.removeEventListener("MozPrecisePointer", this, true);
    window.removeEventListener("MozDeckOffsetChanging", this, true);
    window.removeEventListener("MozDeckOffsetChanged", this, true);
    window.removeEventListener("KeyboardChanged", this, true);

    Elements.browsers.removeEventListener("URLChanged", this, true);
    Elements.browsers.removeEventListener("SizeChanged", this, true);
    Elements.browsers.removeEventListener("ZoomChanged", this, true);

    Elements.navbar.removeEventListener("transitionend", this, true);
    Elements.navbar.removeEventListener("MozAppbarDismissing", this, true);

    this._shutdownAllMarkers();

    this._selectionMarkIds = [];
    this._msgTarget = null;
    this._activeSelectionRect = null;

    this.overlay.displayDebugLayer = false;
    this.overlay.enabled = false;
  },

  



  






  _swapCaretMarker: function _swapCaretMarker(aDirection) {
    let targetMark = null;
    if (aDirection == "start")
      targetMark = this.startMark;
    else
      targetMark = this.endMark;
    let caret = this.caretMark;
    targetMark.swapMonocle(caret);
    let id = caret._elementId;
    caret.shutdown();
    this._caretMark = null;
    this._selectionMarkIds.push(id);
  },

  




  _transitionFromCaretToSelection: function _transitionFromCaretToSelection(aDirection) {
    
    { let mark = this.startMark; mark = this.endMark; }

    
    
    this._swapCaretMarker(aDirection);

    let targetMark = null;
    if (aDirection == "start")
      targetMark = this.startMark;
    else
      targetMark = this.endMark;

    
    this.startMark.position(targetMark.xPos, targetMark.yPos);
    this.endMark.position(targetMark.xPos, targetMark.yPos);

    
    
    
    
    
    let xpos = this._cachedCaretPos ? this._cachedCaretPos.xPos :
      this._msgTarget.ctobx(targetMark.xPos, true);
    let ypos = this._cachedCaretPos ? this._cachedCaretPos.yPos :
      this._msgTarget.ctoby(targetMark.yPos, true);

    
    
    
    
    this._sendAsyncMessage("Browser:SelectionSwitchMode", {
      newMode: "selection",
      change: targetMark.tag,
      xPos: xpos,
      yPos: ypos,
    });
  },

  





  _setupDebugOptions: function _setupDebugOptions() {
    
    let debugOpts = { dumpRanges: false, displayRanges: false, dumpEvents: false };
    try {
      if (Services.prefs.getBoolPref(kDebugSelectionDumpPref))
        debugOpts.displayRanges = true;
    } catch (ex) {}
    try {
      if (Services.prefs.getBoolPref(kDebugSelectionDisplayPref))
        debugOpts.displayRanges = true;
    } catch (ex) {}
    try {
      if (Services.prefs.getBoolPref(kDebugSelectionDumpEvents)) {
        debugOpts.dumpEvents = true;
        this._debugEvents = true;
      }
    } catch (ex) {}

    if (debugOpts.displayRanges || debugOpts.dumpRanges || debugOpts.dumpEvents) {
      
      this.overlay.displayDebugLayer = true;
      
      this._sendAsyncMessage("Browser:SelectionDebug", debugOpts);
    }
  },

  




  _sendAsyncMessage: function _sendAsyncMessage(aMsg, aJson) {
    if (!this._msgTarget) {
      if (this._debugEvents)
        Util.dumpLn("SelectionHelperUI sendAsyncMessage could not send", aMsg);
      return;
    }
    if (this._msgTarget && this._msgTarget instanceof SelectionPrototype) {
      this._msgTarget.msgHandler(aMsg, aJson);
    } else {
      this._msgTarget.messageManager.sendAsyncMessage(aMsg, aJson);
    }
  },

  _checkForActiveDrag: function _checkForActiveDrag() {
    return (this.startMark.dragging || this.endMark.dragging ||
            this.caretMark.dragging);
  },

  _hitTestSelection: function _hitTestSelection(aEvent) {
    
    if (this._activeSelectionRect &&
        Util.pointWithinRect(aEvent.clientX, aEvent.clientY, this._activeSelectionRect)) {
      return true;
    }
    return false;
  },

  




  _setCaretPositionAtPoint: function _setCaretPositionAtPoint(aX, aY) {
    let json = this._getMarkerBaseMessage("caret");
    json.caret.xPos = aX;
    json.caret.yPos = aY;
    this._sendAsyncMessage("Browser:CaretUpdate", json);
  },

  





  _shutdownAllMarkers: function _shutdownAllMarkers() {
    if (this._startMark)
      this._startMark.shutdown();
    if (this._endMark)
      this._endMark.shutdown();
    if (this._caretMark)
      this._caretMark.shutdown();

    this._startMark = null;
    this._endMark = null;
    this._caretMark = null;
  },

  




  _setupMonocleIdArray: function _setupMonocleIdArray() {
    this._selectionMarkIds = ["selectionhandle-mark1",
                              "selectionhandle-mark2",
                              "selectionhandle-mark3"];
  },

  _hideMonocles: function _hideMonocles() {
    if (this._startMark) {
      this.startMark.hide();
    }
    if (this._endMark) {
      this.endMark.hide();
    }
    if (this._caretMark) {
      this.caretMark.hide();
    }
  },

  _showMonocles: function _showMonocles(aSelection) {
    if (!aSelection) {
      this.caretMark.show();
    } else {
      this.endMark.show();
      this.startMark.show();
    }
  },

  



  




  _onClick: function(aEvent) {
    if (this.layerMode == kChromeLayer && this._targetIsEditable) {
      this.attachToCaret(this._msgTarget, aEvent.clientX, aEvent.clientY);
    }
  },

  _onKeypress: function _onKeypress() {
    this.closeEditSession();
  },

  _onResize: function _onResize() {
    this._sendAsyncMessage("Browser:SelectionUpdate", {});
  },

  




  _onDeckOffsetChanging: function _onDeckOffsetChanging(aEvent) {
    
    this._hideMonocles();
  },

  




  _onDeckOffsetChanged: function _onDeckOffsetChanged(aEvent) {
    
    this.attachToCaret(null, this._lastPoint.xPos, this._lastPoint.yPos);
  },

  




  _onNavBarTransitionEvent: function _onNavBarTransitionEvent(aEvent) {
    if (this.layerMode == kContentLayer) {
      return;
    }

    if (aEvent.propertyName == "bottom" && Elements.navbar.isShowing) {
      this._sendAsyncMessage("Browser:SelectionUpdate", {});
      return;
    }
    
    if (aEvent.propertyName == "transform" && Elements.navbar.isShowing) {
      this._sendAsyncMessage("Browser:SelectionUpdate", {});
      this._showMonocles(ChromeSelectionHandler.hasSelection);
    }
  },

  _onNavBarDismissEvent: function _onNavBarDismissEvent() {
    if (!this.isActive || this.layerMode == kContentLayer) {
      return;
    }
    this._hideMonocles();
  },

  _onKeyboardChangedEvent: function _onKeyboardChangedEvent() {
    if (!this.isActive || this.layerMode == kContentLayer) {
      return;
    }
    this._sendAsyncMessage("Browser:SelectionUpdate", {});
  },

  



  _onDebugRectRequest: function _onDebugRectRequest(aMsg) {
    this.overlay.addDebugRect(aMsg.left, aMsg.top, aMsg.right, aMsg.bottom,
                              aMsg.color, aMsg.fill, aMsg.id);
  },

  _selectionHandlerShutdown: function _selectionHandlerShutdown() {
    this._shutdown();
  },

  



  _onSelectionCopied: function _onSelectionCopied(json) {
    this.closeEditSession(true);
  },

  _onSelectionRangeChange: function _onSelectionRangeChange(json) {
    let haveSelectionRect = true;

    if (json.updateStart) {
      this.startMark.position(this._msgTarget.btocx(json.start.xPos, true),
                              this._msgTarget.btocy(json.start.yPos, true));
    }
    if (json.updateEnd) {
      this.endMark.position(this._msgTarget.btocx(json.end.xPos, true),
                            this._msgTarget.btocy(json.end.yPos, true));
    }

    if (json.updateCaret) {
      
      
      
      haveSelectionRect = json.selectionRangeFound;
      if (json.selectionRangeFound) {
        this.caretMark.position(this._msgTarget.btocx(json.caret.xPos, true),
                                this._msgTarget.btocy(json.caret.yPos, true));
        this.caretMark.show();
      }
    }

    this._targetIsEditable = json.targetIsEditable;
    this._activeSelectionRect = haveSelectionRect ?
      this._msgTarget.rectBrowserToClient(json.selection, true) :
      this._activeSelectionRect = Util.getCleanRect();
    this._targetElementRect =
      this._msgTarget.rectBrowserToClient(json.element, true);

    
    
    if (json.src == "start" || json.src == "end") {
      this._showMonocles(true);
    }
  },

  _onSelectionFail: function _onSelectionFail() {
    Util.dumpLn("failed to get a selection.");
    this.closeEditSession();
  },

  





  _onPong: function _onPong(aId) {
    let ping = this._pingArray.pop();
    if (ping.id != aId) {
      ping.deferred.reject(
        new Error("Selection module's pong doesn't match our last ping."));
    }
    ping.deferred.resolve();
   },

  



  handleEvent: function handleEvent(aEvent) {
    if (this._debugEvents && aEvent.type != "touchmove") {
      Util.dumpLn("SelectionHelperUI:", aEvent.type);
    }
    switch (aEvent.type) {
      case "click":
        this._onClick(aEvent);
        break;

      case "touchstart": {
        if (aEvent.touches.length != 1)
          break;
        
        
        if (this._checkForActiveDrag()) {
          aEvent.preventDefault();
        }
        let touch = aEvent.touches[0];
        this._movement.x = touch.clientX;
        this._movement.y = touch.clientY;
        this._movement.active = true;
        break;
      }

      case "touchend":
        if (aEvent.touches.length == 0)
          this._movement.active = false;
        break;

      case "touchmove": {
        if (aEvent.touches.length != 1)
          break;
        let touch = aEvent.touches[0];
        
        if (!this._checkForActiveDrag() && this._movement.active) {
          if (Math.abs(touch.clientX - this._movement.x) > kDisableOnScrollDistance ||
              Math.abs(touch.clientY - this._movement.y) > kDisableOnScrollDistance) {
            this.closeEditSession(true);
          }
        }
        break;
      }

      case "keypress":
        this._onKeypress(aEvent);
        break;

      case "SizeChanged":
        this._onResize(aEvent);
        break;

      case "URLChanged":
        this._shutdown();
        break;

      case "ZoomChanged":
      case "MozPrecisePointer":
        this.closeEditSession(true);
        break;

      case "MozDeckOffsetChanging":
        this._onDeckOffsetChanging(aEvent);
        break;

      case "MozDeckOffsetChanged":
        this._onDeckOffsetChanged(aEvent);
        break;

      case "transitionend":
        this._onNavBarTransitionEvent(aEvent);
        break;

      case "MozAppbarDismissing":
        this._onNavBarDismissEvent();
        break;

      case "KeyboardChanged":
        this._onKeyboardChangedEvent();
        break;
    }
  },

  receiveMessage: function sh_receiveMessage(aMessage) {
    if (this._debugEvents) Util.dumpLn("SelectionHelperUI:", aMessage.name);
    let json = aMessage.json;
    switch (aMessage.name) {
      case "Content:SelectionFail":
        this._onSelectionFail();
        break;
      case "Content:SelectionRange":
        this._onSelectionRangeChange(json);
        break;
      case "Content:SelectionCopied":
        this._onSelectionCopied(json);
        break;
      case "Content:SelectionDebugRect":
        this._onDebugRectRequest(json);
        break;
      case "Content:HandlerShutdown":
        this._selectionHandlerShutdown();
        break;
      case "Content:SelectionHandlerPong":
        this._onPong(json.id);
        break;
    }
  },

  



  _getMarkerBaseMessage: function _getMarkerBaseMessage(aMarkerTag) {
    return {
      change: aMarkerTag,
      start: {
        xPos: this._msgTarget.ctobx(this.startMark.xPos, true),
        yPos: this._msgTarget.ctoby(this.startMark.yPos, true)
      },
      end: {
        xPos: this._msgTarget.ctobx(this.endMark.xPos, true),
        yPos: this._msgTarget.ctoby(this.endMark.yPos, true)
      },
      caret: {
        xPos: this._msgTarget.ctobx(this.caretMark.xPos, true),
        yPos: this._msgTarget.ctoby(this.caretMark.yPos, true)
      },
    };
  },

  markerDragStart: function markerDragStart(aMarker) {
    let json = this._getMarkerBaseMessage(aMarker.tag);
    if (aMarker.tag == "caret") {
      this._cachedCaretPos = null;
      this._sendAsyncMessage("Browser:CaretMove", json);
      return;
    }
    this._sendAsyncMessage("Browser:SelectionMoveStart", json);
  },

  markerDragStop: function markerDragStop(aMarker) {
    let json = this._getMarkerBaseMessage(aMarker.tag);
    if (aMarker.tag == "caret") {
      this._sendAsyncMessage("Browser:CaretUpdate", json);
      return;
    }
    this._sendAsyncMessage("Browser:SelectionMoveEnd", json);
  },

  markerDragMove: function markerDragMove(aMarker, aDirection) {
    if (aMarker.tag == "caret") {
      
      
      if (aDirection != "tbd") {
        
        
        
        this._transitionFromCaretToSelection(aDirection);
        return false;
      }
      
      if (!this._cachedCaretPos) {
        this._cachedCaretPos = this._getMarkerBaseMessage(aMarker.tag).caret;
      }
      return true;
    }
    this._cachedCaretPos = null;

    
    this._hideMonocles();

    let json = this._getMarkerBaseMessage(aMarker.tag);
    this._sendAsyncMessage("Browser:SelectionMove", json);
    return true;
  },
};
