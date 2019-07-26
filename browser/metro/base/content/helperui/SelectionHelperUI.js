



 



 
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

function Marker() {}

Marker.prototype = {
  _element: null,
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

  init: function init(aParent, aElementId, xPos, yPos) {
    this._selectionHelperUI = aParent;
    this._element = document.getElementById(aElementId);
    
    this._element.customDragger = new MarkerDragger(this);
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
    this._selectionHelperUI.markerDragMove(this);
    this._setPosition();
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
};





var SelectionHelperUI = {
  _debugEvents: false,
  _popupState: null,
  _startMark: null,
  _endMark: null,
  _target: null,
  _movement: { active: false, x:0, y: 0 },
  _activeSelectionRect: null,

  get startMark() {
    if (this._startMark == null) {
      this._startMark = new Marker();
      this._startMark.tag = "start";
      this._startMark.init(this, "selectionhandle-start");
    }
    return this._startMark;
  },

  get endMark() {
    if (this._endMark == null) {
      this._endMark = new Marker();
      this._endMark.tag = "end";
      this._endMark.init(this, "selectionhandle-end");
    }
    return this._endMark;
  },

  get overlay() {
    return document.getElementById("selection-overlay");
  },

  





  openEditSession: function openEditSession(aMessage) {
     














    this._popupState = aMessage.json;
    this._popupState._target = aMessage.target;

    this._init();

    Util.dumpLn("enableSelection target:", this._popupState._target);

    
    this.startMark.setTrackBounds(this._popupState.xPos, this._popupState.yPos);
    this.endMark.setTrackBounds(this._popupState.xPos, this._popupState.yPos);

    
    
    this._popupState._target.messageManager.sendAsyncMessage(
      "Browser:SelectionStart",
      { xPos: this._popupState.xPos,
        yPos: this._popupState.yPos });

    this._setupDebugOptions();
  },

  




  attachEditSession: function attachEditSession(aMessage) {
    this._popupState = aMessage.json;
    this._popupState._target = aMessage.target;

    this._init();

    Util.dumpLn("enableSelection target:", this._popupState._target);

    
    this.startMark.setTrackBounds(this._popupState.xPos, this._popupState.yPos);
    this.endMark.setTrackBounds(this._popupState.xPos, this._popupState.yPos);

    
    
    this._popupState._target.messageManager.sendAsyncMessage(
      "Browser:SelectionAttach",
      { xPos: this._popupState.xPos,
        yPos: this._popupState.yPos });

    this._setupDebugOptions();
  },

  




  canHandle: function canHandle(aMessage) {
    if (aMessage.json.types.indexOf("content-text") != -1)
      return true;
    return false;
  },

  




  isActive: function isActive() {
    return (this._popupState && this._popupState._target);
  },

  





  closeEditSession: function closeEditSession() {
    if (this._popupState._target)
      this._popupState._target.messageManager.sendAsyncMessage("Browser:SelectionClose");
    this._shutdown();
  },

  





  closeEditSessionAndClear: function closeEditSessionAndClear() {
    if (this._popupState._target)
      this._popupState._target.messageManager.sendAsyncMessage("Browser:SelectionClear");
    this.closeEditSession();
  },

  



  _init: function _init() {
    
    messageManager.addMessageListener("Content:SelectionRange", this);
    messageManager.addMessageListener("Content:SelectionCopied", this);
    messageManager.addMessageListener("Content:SelectionFail", this);
    messageManager.addMessageListener("Content:SelectionDebugRect", this);

    
    window.addEventListener("click", this, true);
    window.addEventListener("dblclick", this, true);
    window.addEventListener("MozPressTapGesture", this, true);

    
    window.addEventListener("touchstart", this, true);
    window.addEventListener("touchend", this, true);
    window.addEventListener("touchmove", this, true);

    
    window.addEventListener("keypress", this, true);
    Elements.browsers.addEventListener("URLChanged", this, true);
    Elements.browsers.addEventListener("SizeChanged", this, true);
    Elements.browsers.addEventListener("ZoomChanged", this, true);

    window.addEventListener("MozPrecisePointer", this, true);

    this.overlay.enabled = true;
  },

  _shutdown: function _shutdown() {
    messageManager.removeMessageListener("Content:SelectionRange", this);
    messageManager.removeMessageListener("Content:SelectionCopied", this);
    messageManager.removeMessageListener("Content:SelectionFail", this);
    messageManager.removeMessageListener("Content:SelectionDebugRect", this);

    window.removeEventListener("click", this, true);
    window.removeEventListener("dblclick", this, true);
    window.removeEventListener("MozPressTapGesture", this, true);

    window.removeEventListener("touchstart", this, true);
    window.removeEventListener("touchend", this, true);
    window.removeEventListener("touchmove", this, true);

    window.removeEventListener("keypress", this, true);
    Elements.browsers.removeEventListener("URLChanged", this, true);
    Elements.browsers.removeEventListener("SizeChanged", this, true);
    Elements.browsers.removeEventListener("ZoomChanged", this, true);

    window.removeEventListener("MozPrecisePointer", this, true);

    if (this.startMark != null)
      this.startMark.shutdown();
    if (this.endMark != null)
      this.endMark.shutdown();

    delete this._startMark;
    delete this._endMark;

    this._popupState = null;
    this._activeSelectionRect = null;

    this.overlay.displayDebugLayer = false;
    this.overlay.enabled = false;
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
      
      this._popupState
          ._target
          .messageManager
          .sendAsyncMessage("Browser:SelectionDebug", debugOpts);
    }
  },

  



  _checkForActiveDrag: function _checkForActiveDrag() {
    return (this.startMark.dragging || this.endMark.dragging);
  },

  _hitTestSelection: function _hitTestSelection(aEvent) {
    let clickPos =
      this._popupState._target.transformClientToBrowser(aEvent.clientX,
                                                        aEvent.clientY);
    
    if (this._activeSelectionRect &&
        Util.pointWithinRect(clickPos.x, clickPos.y, this._activeSelectionRect)) {
      return true;
    }
    return false;
  },

  _onTap: function _onTap(aEvent) {
    
    aEvent.stopPropagation();
    aEvent.preventDefault();

    if (this.startMark.hitTest(aEvent.clientX, aEvent.clientY) ||
        this.endMark.hitTest(aEvent.clientX, aEvent.clientY)) {
      
      this._popupState._target.messageManager.sendAsyncMessage("Browser:ChangeMode", {});
    }
  },

  _onLongTap: function _onLongTap(aEvent) {
    
    
    if (this.startMark.hitTest(aEvent.clientX, aEvent.clientY) ||
        this.endMark.hitTest(aEvent.clientX, aEvent.clientY)) {
      aEvent.stopPropagation();
      aEvent.preventDefault();
      return;
    }
  },

  



  _onDblTap: function _onDblTap(aEvent) {
    if (!this._hitTestSelection(aEvent)) {
      
      this.closeEditSessionAndClear();
      return;
    }

    
    let clickPos =
      this._popupState._target.transformClientToBrowser(aEvent.clientX,
                                                        aEvent.clientY);
    let json = {
      xPos: clickPos.x,
      yPos: clickPos.y,
    };

    this._popupState._target.messageManager.sendAsyncMessage("Browser:SelectionCopy", json);

    aEvent.stopPropagation();
    aEvent.preventDefault();
  },

  _onSelectionCopied: function _onSelectionCopied(json) {
    if (json.succeeded) {
      this.showToast(Strings.browser.GetStringFromName("selectionHelper.textCopied"));
    }
    this.closeEditSessionAndClear();
  },

  _onSelectionRangeChange: function _onSelectionRangeChange(json) {
    if (json.updateStart) {
      let pos =
        this._popupState._target.transformBrowserToClient(json.start.xPos, json.start.yPos);
      this.startMark.position(pos.x, pos.y);
      this.startMark.show();
    }
    if (json.updateEnd) {
      let pos =
        this._popupState._target.transformBrowserToClient(json.end.xPos, json.end.yPos);
      this.endMark.position(pos.x, pos.y);
      this.endMark.show();
    }
    this._activeSelectionRect = json.rect;
  },

  _onSelectionFail: function _onSelectionFail() {
    Util.dumpLn("failed to get a selection.");
    this.closeEditSession();
  },

  _onKeypress: function _onKeypress() {
    this.closeEditSession();
  },

  _onResize: function _onResize() {
    this._popupState._target
                    .messageManager
                    .sendAsyncMessage("Browser:SelectionUpdate", {});
  },

  _onDebugRectRequest: function _onDebugRectRequest(aMsg) {
    this.overlay.addDebugRect(aMsg.left, aMsg.top, aMsg.right, aMsg.bottom,
                              aMsg.color, aMsg.fill, aMsg.id);
  },

  



  _initMouseEventFromEvent: function _initMouseEventFromEvent(aDestEvent, aSrcEvent, aType) {
    event.initNSMouseEvent(aType, true, true, content, 0,
                           aSrcEvent.screenX, aSrcEvent.screenY, aSrcEvent.clientX, aSrcEvent.clientY,
                           false, false, false, false,
                           aSrcEvent.button, aSrcEvent.relatedTarget, 1.0,
                           Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH);
  },

  handleEvent: function handleEvent(aEvent) {
    if (this._debugEvents && aEvent.type != "touchmove") {
      Util.dumpLn("SelectionHelperUI:", aEvent.type);
    }
    switch (aEvent.type) {
      case "click":
        this._onTap(aEvent);
        break;

      case "dblclick":
        this._onDblTap(aEvent);
        break;

      case "MozPressTapGesture":
        this._onLongTap(aEvent);
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
            this.closeEditSessionAndClear();
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
        this.closeEditSessionAndClear();
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
    }
  },

  



  _getMarkerBaseMessage: function _getMarkerBaseMessage() {
  













    return {
      start: { xPos: this.startMark.xPos, yPos: this.startMark.yPos },
      end: { xPos: this.endMark.xPos, yPos: this.endMark.yPos },
    };
  },

  markerDragStart: function markerDragStart(aMarker) {
    let json = this._getMarkerBaseMessage();
    json.change = aMarker.tag;
    this._popupState._target.messageManager.sendAsyncMessage("Browser:SelectionMoveStart", json);
  },

  markerDragStop: function markerDragStop(aMarker) {
    
    let json = this._getMarkerBaseMessage();
    json.change = aMarker.tag;
    this._popupState._target.messageManager.sendAsyncMessage("Browser:SelectionMoveEnd", json);
  },

  markerDragMove: function markerDragMove(aMarker) {
    let json = this._getMarkerBaseMessage();
    json.change = aMarker.tag;
    this._popupState._target.messageManager.sendAsyncMessage("Browser:SelectionMove", json);
  },

  showToast: function showToast(aString) {
    let toaster =
      Cc["@mozilla.org/toaster-alerts-service;1"]
        .getService(Ci.nsIAlertsService);
    toaster.showAlertNotification(null, aString, "", false, "", null);
  },
};
