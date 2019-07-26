








const kOnClickMargin = 3;

const kNavButtonPref = "browser.display.overlaynavbuttons";

var NavButtonSlider = {
  _back: document.getElementById("overlay-back"),
  _plus: document.getElementById("overlay-plus"),
  _mouseMoveStarted: false,
  _mouseDown: false,
  _yPos: -1,

  



  freeDrag: function freeDrag() {
    return true;
  },

  isDraggable: function isDraggable(aTarget, aContent) {
    return { x: false, y: true };
  },

  dragStart: function dragStart(aX, aY, aTarget, aScroller) {
    return true;
  },

  dragStop: function dragStop(aDx, aDy, aScroller) {
    return true;
  },

  dragMove: function dragMove(aDx, aDy, aScroller, aIsKenetic, aClientX, aClientY) {
    
    
    if (aIsKenetic) {
      return false;
    }
    
    this._updatePosition(aClientY);

    
    
    return true;
  },

  



  init: function init() {
    
    this._back.customDragger = this;
    this._plus.customDragger = this;
    Elements.browsers.addEventListener("ContentSizeChanged", this, true);
    let events = ["mousedown", "mouseup", "mousemove", "click"];
    events.forEach(function (value) {
      this._back.addEventListener(value, this, true);
      this._plus.addEventListener(value, this, true);
    }, this);

    this._updateStops();
    this._updateVisibility();
    Services.prefs.addObserver(kNavButtonPref, this, false);
  },

  observe: function BrowserUI_observe(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed" && aData == kNavButtonPref) {
      this._updateVisibility();
    }
  },

  _updateVisibility: function () {
    if (Services.prefs.getBoolPref(kNavButtonPref)) {
      this._back.removeAttribute("hidden");
      this._plus.removeAttribute("hidden");
    } else {
      this._back.setAttribute("hidden", true);
      this._plus.setAttribute("hidden", true);
    }
  },

  _updateStops: function () {
    this._contentHeight = ContentAreaObserver.contentHeight;
    this._imageHeight = 118;
    this._topStop = this._imageHeight * .7;
    this._bottomStop = this._contentHeight - (this._imageHeight * .7);

    
    if (this._yPos != -1 &&
        (this._topStop > this._yPos || this._bottomStop < this._yPos)) {
      this._back.style.top = "50%";
      this._plus.style.top = "50%";
    }
  },

  _getPosition: function _getPosition() {
    this._yPos = parseInt(getComputedStyle(this._back).top);
  },

  _setPosition: function _setPosition() {
    this._back.style.top = this._yPos + "px";
    this._plus.style.top = this._yPos + "px";
  },

  _updatePosition: function (aClientY) {
    if (this._topStop > aClientY || this._bottomStop < aClientY)
      return;
    this._yPos = aClientY;
    this._setPosition();
  },

  _updateOffset: function (aOffset) {
    let newPos = this._yPos + aOffset;
    if (this._topStop > newPos || this._bottomStop < newPos)
      return;
    this._yPos = newPos;
    this._setPosition();
  },

  



  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "ContentSizeChanged":
        this._updateStops();
        break;
      case "mousedown":
        this._getPosition();
        this._mouseDown = true;
        this._mouseMoveStarted = false;
        this._mouseY = aEvent.clientY;
        aEvent.originalTarget.setCapture();
        this._back.setAttribute("mousedrag", true);
        this._plus.setAttribute("mousedrag", true);
        break;
      case "mouseup":
        this._mouseDown = false;
        this._back.removeAttribute("mousedrag");
        this._plus.removeAttribute("mousedrag");
        break;
      case "mousemove":
        
        if (!this._mouseDown) {
          return;
        }
        
        let dy = aEvent.clientY - this._mouseY;
        if (!this._mouseMoveStarted && Math.abs(dy) < kOnClickMargin) {
          return;
        }
        
        this._mouseMoveStarted = true;
        this._mouseY = aEvent.clientY;
        this._updateOffset(dy);
        break;
      case "click":
        
        if (this._mouseMoveStarted) {
          return;
        }
        if (aEvent.originalTarget == this._back) {
           CommandUpdater.doCommand('cmd_back');
        } else {
           CommandUpdater.doCommand('cmd_newTab');
        }
        break;
    }
  },
};


