


"use strict";







function findGraphNodeParent (el) {
  
  if (!el.classList)
    return null;

  while (!el.classList.contains("nodes")) {
    if (el.classList.contains("audionode"))
      return el;
    else
      el = el.parentNode;
  }
  return null;
}















let ToggleMixin = {

  bindToggle: function () {
    this._onToggle = this._onToggle.bind(this);
    this.button.addEventListener("mousedown", this._onToggle, false);
  },

  unbindToggle: function () {
    this.button.removeEventListener("mousedown", this._onToggle);
  },

  show: function () {
    this._viewController({ visible: true });
  },

  hide: function () {
    this._viewController({ visible: false });
  },

  hideImmediately: function () {
    this._viewController({ visible: false, delayed: false, animated: false });
  },

  



  isVisible: function () {
    return !this.el.hasAttribute("pane-collapsed");
  },

  








  _viewController: function ({ visible, animated, delayed }) {
    let flags = {
      visible: visible,
      animated: animated != null ? animated : !!this._animated,
      delayed: delayed != null ? delayed : !!this._delayed,
      callback: () => window.emit(this._toggleEvent, visible)
    };

    ViewHelpers.togglePane(flags, this.el);

    if (flags.visible) {
      this.button.removeAttribute("pane-collapsed");
      this.button.setAttribute("tooltiptext", this._collapseString);
    }
    else {
      this.button.setAttribute("pane-collapsed", "");
      this.button.setAttribute("tooltiptext", this._expandString);
    }
  },

  _onToggle: function () {
    this._viewController({ visible: !this.isVisible() });
  }
}
