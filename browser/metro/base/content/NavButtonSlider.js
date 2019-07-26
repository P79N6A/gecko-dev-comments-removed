







var NavButtonSlider = {
  _back: document.getElementById("overlay-back"),
  _plus: document.getElementById("overlay-plus"),
  _dragging: false,
  _yPos: -1,

  get dragging() {
    return this._dragging;
  },

  



  freeDrag: function freeDrag() {
    return true;
  },

  isDraggable: function isDraggable(aTarget, aContent) {
    return { x: false, y: true };
  },

  dragStart: function dragStart(aX, aY, aTarget, aScroller) {
    this._dragging = true;
    return true;
  },

  dragStop: function dragStop(aDx, aDy, aScroller) {
    this._dragging = false;
    return true;
  },

  dragMove: function dragMove(aDx, aDy, aScroller, aIsKenetic, aClientX, aClientY) {
    
    
    if (aIsKenetic) {
      return false;
    }
    
    this._update(aClientY);

    
    
    return true;
  },

  



  init: function init() {
    
    this._back.customDragger = this;
    this._plus.customDragger = this;
    Elements.browsers.addEventListener("ContentSizeChanged", this, true);
    this._updateStops();
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

  _setPosition: function _setPosition() {
    this._back.style.top = this._yPos + "px";
    this._plus.style.top = this._yPos + "px";
  },

  _update: function (aClientY) {
    if (this._topStop > aClientY || this._bottomStop < aClientY)
      return;
    this._yPos = aClientY;
    this._setPosition();
  },

  



  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "ContentSizeChanged":
        this._updateStops();
        break;
    }
  },
};


