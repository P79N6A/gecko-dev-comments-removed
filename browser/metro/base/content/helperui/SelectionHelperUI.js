















const kDisableOnScrollDistance = 25;





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
  this._element = document.getElementById(aElementId);
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
    this._selectionHelperUI.markerDragStart(this);
  },

  dragStop: function dragStop(aDx, aDy) {
    this._selectionHelperUI.markerDragStop(this);
  },

  moveBy: function moveBy(aDx, aDy, aClientX, aClientY) {
    this._xPos -= aDx;
    this._yPos -= aDy;
    let direction = (aDx >= 0 && aDy >= 0 ? "start" : "end");
    
    
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
  _selectionHandlerActive: false,
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
    return document.getElementById("selection-overlay");
  },

  




  get isActive() {
    return (this._msgTarget &&
            this._selectionHandlerActive);
  },

  



  








  openEditSession: function openEditSession(aBrowser, aX, aY) {
    if (!aBrowser || this.isActive)
      return;
    this._init(aBrowser);
    this._setupDebugOptions();

    
    
    
    this._selectionHandlerActive = false;
    this._sendAsyncMessage("Browser:SelectionStart", {
      xPos: aX,
      yPos: aY
    });
  },

  







  attachEditSession: function attachEditSession(aBrowser, aX, aY) {
    if (!aBrowser || this.isActive)
      return;
    this._init(aBrowser);
    this._setupDebugOptions();

    
    
    
    this._selectionHandlerActive = false;
    this._sendAsyncMessage("Browser:SelectionAttach", {
      xPos: aX,
      yPos: aY
    });
  },

  














  attachToCaret: function attachToCaret(aBrowser, aX, aY) {
    if (!this.isActive) {
      this._init(aBrowser);
      this._setupDebugOptions();
    }

    this._lastPoint = { xPos: aX, yPos: aY };

    this._selectionHandlerActive = false;
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
    let clearSelection = aClearSelection || false;
    this._sendAsyncMessage("Browser:SelectionClose", {
      clearSelection: clearSelection
    });
    this._shutdown();
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

    window.addEventListener("keypress", this, true);
    window.addEventListener("click", this, false);
    window.addEventListener("touchstart", this, true);
    window.addEventListener("touchend", this, true);
    window.addEventListener("touchmove", this, true);
    window.addEventListener("MozContextUIShow", this, true);
    window.addEventListener("MozContextUIDismiss", this, true);
    window.addEventListener("MozPrecisePointer", this, true);
    window.addEventListener("MozDeckOffsetChanging", this, true);
    window.addEventListener("MozDeckOffsetChanged", this, true);

    Elements.browsers.addEventListener("URLChanged", this, true);
    Elements.browsers.addEventListener("SizeChanged", this, true);
    Elements.browsers.addEventListener("ZoomChanged", this, true);

    this.overlay.enabled = true;
  },

  _shutdown: function _shutdown() {
    messageManager.removeMessageListener("Content:SelectionRange", this);
    messageManager.removeMessageListener("Content:SelectionCopied", this);
    messageManager.removeMessageListener("Content:SelectionFail", this);
    messageManager.removeMessageListener("Content:SelectionDebugRect", this);

    window.removeEventListener("keypress", this, true);
    window.removeEventListener("click", this, false);
    window.removeEventListener("touchstart", this, true);
    window.removeEventListener("touchend", this, true);
    window.removeEventListener("touchmove", this, true);
    window.removeEventListener("MozContextUIShow", this, true);
    window.removeEventListener("MozContextUIDismiss", this, true);
    window.removeEventListener("MozPrecisePointer", this, true);
    window.removeEventListener("MozDeckOffsetChanging", this, true);
    window.removeEventListener("MozDeckOffsetChanged", this, true);

    Elements.browsers.removeEventListener("URLChanged", this, true);
    Elements.browsers.removeEventListener("SizeChanged", this, true);
    Elements.browsers.removeEventListener("ZoomChanged", this, true);

    this._shutdownAllMarkers();

    this._selectionMarkIds = [];
    this._msgTarget = null;
    this._activeSelectionRect = null;
    this._selectionHandlerActive = false;

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

    
    
    
    
    this._sendAsyncMessage("Browser:SelectionSwitchMode", {
      newMode: "selection",
      change: targetMark.tag,
      xPos: this._msgTarget.ctobx(targetMark.xPos, true),
      yPos: this._msgTarget.ctoby(targetMark.yPos, true),
    });
  },

  







  _transitionFromSelectionToCaret: function _transitionFromSelectionToCaret(aClientX, aClientY) {
    
    this.closeEditSession(true);

    
    this._selectionHandlerActive = false;
    this._activeSelectionRect = null;

    
    this._shutdownAllMarkers();
    this._setupMonocleIdArray();

    
    let coords =
      this._msgTarget.ptClientToBrowser(aClientX, aClientY, true);

    
    
    
    this._sendAsyncMessage("Browser:CaretAttach", {
      xPos: coords.x,
      yPos: coords.y
    });

    
    this._setCaretPositionAtPoint(coords.x, coords.y);
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
    this._msgTarget.messageManager.sendAsyncMessage(aMsg, aJson);
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
    let json = this._getMarkerBaseMessage();
    json.change = "caret";
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

  



  







  _onTap: function _onTap(aEvent) {
    let clientCoords =
      this._msgTarget.ptBrowserToClient(aEvent.clientX, aEvent.clientY, true);

    
    if (this.startMark.hitTest(clientCoords.x, clientCoords.y) ||
        this.endMark.hitTest(clientCoords.x, clientCoords.y)) {
      aEvent.stopPropagation();
      aEvent.preventDefault();
      return;
    }

    
    
    
    let pointInTargetElement =
      Util.pointWithinRect(clientCoords.x, clientCoords.y,
                           this._targetElementRect);

    
    
    if (this.caretMark.visible && pointInTargetElement) {
      
      this._setCaretPositionAtPoint(aEvent.clientX, aEvent.clientY);
      return;
    }

    
    
    
    if (this._targetIsEditable && !pointInTargetElement) {
      
      
      
      
      this.closeEditSession(false);
      return;
    }

    let selectionTap = this._hitTestSelection(aEvent);

    
    
    
    if (selectionTap) {
      aEvent.stopPropagation();
      aEvent.preventDefault();
      return;
    }

    
    
    if (this.startMark.visible && pointInTargetElement &&
        this._targetIsEditable) {
      this._transitionFromSelectionToCaret(clientCoords.x, clientCoords.y);
    }

    
    
    aEvent.stopPropagation();
    aEvent.preventDefault();
  },

  _onKeypress: function _onKeypress() {
    this.closeEditSession();
  },

  _onResize: function _onResize() {
    this._sendAsyncMessage("Browser:SelectionUpdate", {});
  },

  _onContextUIVisibilityEvent: function _onContextUIVisibilityEvent(aType) {
    
    if (!this.isActive)
      return;
    this.overlay.hidden = (aType == "MozContextUIShow");
  },

  




  _onDeckOffsetChanging: function _onDeckOffsetChanging(aEvent) {
    
    this._hideMonocles();
  },

  




  _onDeckOffsetChanged: function _onDeckOffsetChanged(aEvent) {
    
    this.attachToCaret(null, this._lastPoint.xPos, this._lastPoint.yPos);
  },

  



  _onDebugRectRequest: function _onDebugRectRequest(aMsg) {
    this.overlay.addDebugRect(aMsg.left, aMsg.top, aMsg.right, aMsg.bottom,
                              aMsg.color, aMsg.fill, aMsg.id);
  },

  _onSelectionCopied: function _onSelectionCopied(json) {
    this.closeEditSession(true);
  },

  _onSelectionRangeChange: function _onSelectionRangeChange(json) {
    let haveSelectionRect = true;

    
    if (json.updateStart) {
      this.startMark.position(this._msgTarget.btocx(json.start.xPos, true),
                              this._msgTarget.btocy(json.start.yPos, true));
      this.startMark.show();
    }
    if (json.updateEnd) {
      this.endMark.position(this._msgTarget.btocx(json.end.xPos, true),
                            this._msgTarget.btocy(json.end.yPos, true));
      this.endMark.show();
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
  },

  _onSelectionFail: function _onSelectionFail() {
    Util.dumpLn("failed to get a selection.");
    this.closeEditSession();
  },

  



  handleEvent: function handleEvent(aEvent) {
    if (this._debugEvents && aEvent.type != "touchmove") {
      Util.dumpLn("SelectionHelperUI:", aEvent.type);
    }
    switch (aEvent.type) {
      case "click":
        this._onTap(aEvent);
        break;

      case "touchstart": {
        if (aEvent.touches.length != 1)
          break;
        let touch = aEvent.touches[0];
        this._movement.x = this._movement.y = 0;
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
          let distanceY = touch.clientY - this._movement.y;
          if (Math.abs(distanceY) > kDisableOnScrollDistance) {
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

      case "ZoomChanged":
      case "URLChanged":
      case "MozPrecisePointer":
        this.closeEditSession(true);
        break;

      case "MozContextUIShow":
      case "MozContextUIDismiss":
        this._onContextUIVisibilityEvent(aEvent.type);
        break;

      case "MozDeckOffsetChanging":
        this._onDeckOffsetChanging(aEvent);
        break;

      case "MozDeckOffsetChanged":
        this._onDeckOffsetChanged(aEvent);
        break;
    }
  },

  receiveMessage: function sh_receiveMessage(aMessage) {
    if (this._debugEvents) Util.dumpLn("SelectionHelperUI:", aMessage.name);
    let json = aMessage.json;
    switch (aMessage.name) {
      case "Content:SelectionFail":
        this._selectionHandlerActive = false;
        this._onSelectionFail();
        break;
      case "Content:SelectionRange":
        this._selectionHandlerActive = true;
        this._onSelectionRangeChange(json);
        break;
      case "Content:SelectionCopied":
        this._selectionHandlerActive = true;
        this._onSelectionCopied(json);
        break;
      case "Content:SelectionDebugRect":
        this._onDebugRectRequest(json);
        break;
    }
  },

  



  _getMarkerBaseMessage: function _getMarkerBaseMessage() {
    return {
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
    let json = this._getMarkerBaseMessage();
    json.change = aMarker.tag;
    if (aMarker.tag == "caret") {
      this._sendAsyncMessage("Browser:CaretMove", json);
      return;
    }
    this._sendAsyncMessage("Browser:SelectionMoveStart", json);
  },

  markerDragStop: function markerDragStop(aMarker) {
    let json = this._getMarkerBaseMessage();
    json.change = aMarker.tag;
    if (aMarker.tag == "caret") {
      this._sendAsyncMessage("Browser:CaretUpdate", json);
      return;
    }
    this._sendAsyncMessage("Browser:SelectionMoveEnd", json);
  },

  markerDragMove: function markerDragMove(aMarker, aDirection) {
    if (aMarker.tag == "caret") {
      
      
      
      this._transitionFromCaretToSelection(aDirection);
      return false;
    }
    let json = this._getMarkerBaseMessage();
    json.change = aMarker.tag;
    this._sendAsyncMessage("Browser:SelectionMove", json);
    return true;
  },
};
