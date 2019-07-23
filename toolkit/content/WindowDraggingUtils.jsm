



































let EXPORTED_SYMBOLS = [ "WindowDraggingElement" ];

function WindowDraggingElement(elem, window) {
  this._elem = elem;
  this._window = window;
  this._elem.addEventListener("mousedown", this, false);
}

WindowDraggingElement.prototype = {
  mouseDownCheck: function(e) { return true; },
  dragTags: ["box", "hbox", "vbox", "spacer", "label", "statusbarpanel", "stack",
             "toolbaritem", "toolbarseparator", "toolbarspring", "toolbarspacer",
             "radiogroup", "deck", "scrollbox"],
  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "mousedown":
        if (aEvent.button != 0 || !this.mouseDownCheck.call(this._elem, aEvent))
          return;

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
            return;
          target = target.parentNode;
        }
        this._deltaX = aEvent.screenX - this._window.screenX;
        this._deltaY = aEvent.screenY - this._window.screenY;
        this._draggingWindow = true;
        this._window.addEventListener("mousemove", this, false);
        this._window.addEventListener("mouseup", this, false);
        break;
      case "mousemove":
        if (this._draggingWindow)
          this._window.moveTo(aEvent.screenX - this._deltaX, aEvent.screenY - this._deltaY);
        break;
      case "mouseup":
        this._draggingWindow = false;
        this._window.removeEventListener("mousemove", this, false);
        this._window.removeEventListener("mouseup", this, false);
        break;
    }
  }
}
