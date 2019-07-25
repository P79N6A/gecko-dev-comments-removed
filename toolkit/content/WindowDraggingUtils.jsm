



































let EXPORTED_SYMBOLS = [ "WindowDraggingElement" ];

function WindowDraggingElement(elem, window) {
  this._elem = elem;
  this._window = window;
#ifdef XP_WIN
  this._elem.addEventListener("MozMouseHittest", this, false);
#else
  this._elem.addEventListener("mousedown", this, false);
#endif
}

WindowDraggingElement.prototype = {
  mouseDownCheck: function(e) { return true; },
  dragTags: ["box", "hbox", "vbox", "spacer", "label", "statusbarpanel", "stack",
             "toolbaritem", "toolbarseparator", "toolbarspring", "toolbarspacer",
             "radiogroup", "deck", "scrollbox", "arrowscrollbox", "tabs"],
  shouldDrag: function(aEvent) {
    if (aEvent.button != 0 ||
        this._window.fullScreen ||
        !this.mouseDownCheck.call(this._elem, aEvent) ||
        aEvent.getPreventDefault())
      return false;

    
    if (!this._elem._alive)
      return false;

    let target = aEvent.originalTarget, parent = aEvent.originalTarget;
    while (parent != this._elem) {
      let mousethrough = parent.getAttribute("mousethrough");
      if (mousethrough == "always")
        target = parent.parentNode;
      else if (mousethrough == "never")
        break;
      parent = parent.parentNode;
    }
    while (target != this._elem) {
      if (this.dragTags.indexOf(target.localName) == -1)
        return false;
      target = target.parentNode;
    }
    return true;
  },
  handleEvent: function(aEvent) {
#ifdef XP_WIN
    if (this.shouldDrag(aEvent))
      aEvent.preventDefault();
#else
    switch (aEvent.type) {
      case "mousedown":
        if (!this.shouldDrag(aEvent))
          return;

#ifdef MOZ_WIDGET_GTK2
        
        
        this._window.beginWindowMove(aEvent);
#else
        this._deltaX = aEvent.screenX - this._window.screenX;
        this._deltaY = aEvent.screenY - this._window.screenY;
        this._draggingWindow = true;
        this._window.addEventListener("mousemove", this, false);
        this._window.addEventListener("mouseup", this, false);
#endif
        break;
      case "mousemove":
        if (this._draggingWindow)
          this._window.moveTo(aEvent.screenX - this._deltaX, aEvent.screenY - this._deltaY);
        break;
      case "mouseup":
        if (this._draggingWindow) {
          this._draggingWindow = false;
          this._window.removeEventListener("mousemove", this, false);
          this._window.removeEventListener("mouseup", this, false);
        }
        break;
    }
#endif
  }
}
