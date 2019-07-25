





































var TabletSidebar = {
  _grabbed: false, 
  _offset: 0, 
  _slideMultiplier: 1, 

  get visible() {
    return Elements.urlbarState.getAttribute("tablet_sidebar") == "true";
  },

  toggle: function toggle() {
    if (this.visible)
      this.hide();
    else
      this.show();
  },

  show: function show() {
    Elements.urlbarState.setAttribute("tablet_sidebar", "true");
    ViewableAreaObserver.update();
  },

  hide: function hide() {
    Elements.urlbarState.setAttribute("tablet_sidebar", "false");
    ViewableAreaObserver.update();
  },

  



  grab: function grab() {
    this._grabbed = true;
    ViewableAreaObserver.update();

    let ltr = (Util.localeDir == Util.LOCALE_DIR_LTR);

    if (this.visible) {
      this._setOffset(ltr ? 0 : ViewableAreaObserver.sidebarWidth);
      this._slideMultiplier = 3;
    } else {
      
      this.show();
      this._setOffset(ltr ? ViewableAreaObserver.sidebarWidth : 0);
      this._slideMultiplier = 6;
    }
  },

  
  slideBy: function slideBy(aX) {
    this._setOffset(this._offset + (aX * this._slideMultiplier));
  },

  
  ungrab: function ungrab() {
    if (!this._grabbed)
      return;
    this._grabbed = false;

    let finalOffset = this._offset;
    this._setOffset(0);

    let rtl = (Util.localeDir == Util.LOCALE_DIR_RTL);
    if (finalOffset > (ViewableAreaObserver.sidebarWidth / 2) ^ rtl)
      this.hide();
    else
      
      ViewableAreaObserver.update();
  },

  
  _setOffset: function _setOffset(aOffset) {
    this._offset = aOffset;
    let scrollX = Util.clamp(aOffset, 0, ViewableAreaObserver.sidebarWidth);
    Browser.controlsScrollboxScroller.scrollTo(scrollX, 0);
  }
}
