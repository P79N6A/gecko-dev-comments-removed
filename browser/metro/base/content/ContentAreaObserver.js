



 


























var ContentAreaObserver = {
  styles: {},
  _keyboardState: false,

  



  get width() {
    return window.innerWidth || 1366;
  },

  get height() {
    return window.innerHeight || 768;
  },

  get contentHeight() {
    return this._getContentHeightForWindow(this.height);
  },

  get contentTop () {
    return Elements.toolbar.getBoundingClientRect().bottom;
  },

  get viewableHeight() {
    return this._getViewableHeightForContent(this.contentHeight);
  },

  get isKeyboardOpened() {
    return MetroUtils.keyboardVisible;
  },

  



  init: function init() {
    window.addEventListener("resize", this, false);

    
    Services.obs.addObserver(this, "metro_softkeyboard_shown", false);
    Services.obs.addObserver(this, "metro_softkeyboard_hidden", false);

    
    this._initStyles();

    
    this.update();
  },

  shutdown: function shutdown() {
    Services.obs.removeObserver(this, "metro_softkeyboard_shown");
    Services.obs.removeObserver(this, "metro_softkeyboard_hidden");
  },

  update: function cao_update (width, height) {
    let oldWidth = parseInt(this.styles["window-width"].width);
    let oldHeight = parseInt(this.styles["window-height"].height);

    let newWidth = width || this.width;
    let newHeight = height || this.height;

    if (newHeight == oldHeight && newWidth == oldWidth)
      return;

    this.styles["window-width"].width = newWidth + "px";
    this.styles["window-width"].maxWidth = newWidth + "px";
    this.styles["window-height"].height = newHeight + "px";
    this.styles["window-height"].maxHeight = newHeight + "px";

    this.updateContentArea(newWidth, this._getContentHeightForWindow(newHeight));
    this._disatchBrowserEvent("SizeChanged");
  },

  updateContentArea: function cao_updateContentArea (width, height) {
    let oldHeight = parseInt(this.styles["content-height"].height);
    let oldWidth = parseInt(this.styles["content-width"].width);

    let newWidth = width || this.width;
    let newHeight = height || this.contentHeight;

    if (newHeight == oldHeight && newWidth == oldWidth)
      return;

    this.styles["content-height"].height = newHeight + "px";
    this.styles["content-height"].maxHeight = newHeight + "px";
    this.styles["content-width"].width = newWidth + "px";
    this.styles["content-width"].maxWidth = newWidth + "px";

    this.updateViewableArea(newWidth, this._getViewableHeightForContent(newHeight));
    this._disatchBrowserEvent("ContentSizeChanged");
  },

  updateViewableArea: function cao_updateViewableArea (width, height) {
    let oldHeight = parseInt(this.styles["viewable-height"].height);
    let oldWidth = parseInt(this.styles["viewable-width"].width);

    let newWidth = width || this.width;
    let newHeight = height || this.viewableHeight;

    if (newHeight == oldHeight && newWidth == oldWidth)
      return;

    this.styles["viewable-height"].height = newHeight + "px";
    this.styles["viewable-height"].maxHeight = newHeight + "px";
    this.styles["viewable-width"].width = newWidth + "px";
    this.styles["viewable-width"].maxWidth = newWidth + "px";

    this._disatchBrowserEvent("ViewableSizeChanged");
  },

  onBrowserCreated: function onBrowserCreated(aBrowser) {
    aBrowser.classList.add("content-width");
    aBrowser.classList.add("content-height");
  },

  



  _onKeyboardDisplayChanging: function _onKeyboardDisplayChanging(aNewState) {
    this._keyboardState = aNewState;

    this._dispatchWindowEvent("KeyboardChanged", aNewState);

    this.updateViewableArea();
  },

  observe: function cao_observe(aSubject, aTopic, aData) {
    
    
    
    switch (aTopic) {
      case "metro_softkeyboard_hidden":
      case "metro_softkeyboard_shown":
        
        
        
        let self = this;
        let state = aTopic == "metro_softkeyboard_shown";
        Util.executeSoon(function () {
          self._onKeyboardDisplayChanging(state);
        });
        break;
    }
  },

  handleEvent: function cao_handleEvent(aEvent) {
    switch (aEvent.type) {
      case 'resize':
        if (aEvent.target != window)
          return;
        ContentAreaObserver.update();
        break;
    }
  },

  



  _dispatchWindowEvent: function _dispatchWindowEvent(aEventName, aDetail) {
    let event = document.createEvent("UIEvents");
    event.initUIEvent(aEventName, true, false, window, aDetail);
    window.dispatchEvent(event);
  },

  _disatchBrowserEvent: function (aName, aDetail) {
    setTimeout(function() {
      let event = document.createEvent("Events");
      event.initEvent(aName, true, false);
      Elements.browsers.dispatchEvent(event);
    }, 0);
  },

  _initStyles: function _initStyles() {
    let stylesheet = document.styleSheets[0];
    for (let style of ["window-width", "window-height",
                       "content-height", "content-width",
                       "viewable-height", "viewable-width"]) {
      let index = stylesheet.insertRule("." + style + " {}", stylesheet.cssRules.length);
      this.styles[style] = stylesheet.cssRules[index].style;
    }
  },

  _getContentHeightForWindow: function (windowHeight) {
    let contextUIHeight = BrowserUI.isTabsOnly ? Elements.toolbar.getBoundingClientRect().bottom : 0;
    return windowHeight - contextUIHeight;
  },

  _getViewableHeightForContent: function (contentHeight) {
    let keyboardHeight = MetroUtils.keyboardHeight;
    return contentHeight - keyboardHeight;
  },

  _debugDumpDims: function _debugDumpDims() {
    let width = parseInt(this.styles["window-width"].width);
    let height = parseInt(this.styles["window-height"].height);
    Util.dumpLn("window:", width, height);
    width = parseInt(this.styles["content-width"].width);
    height = parseInt(this.styles["content-height"].height);
    Util.dumpLn("content:", width, height);
    width = parseInt(this.styles["viewable-width"].width);
    height = parseInt(this.styles["viewable-height"].height);
    Util.dumpLn("viewable:", width, height);
  }
};
