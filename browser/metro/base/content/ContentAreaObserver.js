



 


























var ContentAreaObserver = {
  styles: {},
  
  
  
  get width() { return window.innerWidth || 1366; },
  get height() { return window.innerHeight || 768; },

  get contentHeight() {
    return this._getContentHeightForWindow(this.height);
  },

  get contentTop () {
    return Elements.toolbar.getBoundingClientRect().bottom;
  },

  get viewableHeight() {
    return this._getViewableHeightForContent(this.contentHeight);
  },

  get isKeyboardOpened() { return MetroUtils.keyboardVisible; },
  get hasVirtualKeyboard() { return true; },

  init: function cao_init() {
    window.addEventListener("resize", this, false);

    let os = Services.obs;
    os.addObserver(this, "metro_softkeyboard_shown", false);
    os.addObserver(this, "metro_softkeyboard_hidden", false);

    
    
    
    
    
    
    
    
    
    
    
    let stylesheet = document.styleSheets[0];
    for (let style of ["window-width", "window-height",
                       "content-height", "content-width",
                       "viewable-height", "viewable-width"]) {
      let index = stylesheet.insertRule("." + style + " {}", stylesheet.cssRules.length);
      this.styles[style] = stylesheet.cssRules[index].style;
    }
    this.update();
  },

  uninit: function cao_uninit() {
    let os = Services.obs;
    os.removeObserver(this, "metro_softkeyboard_shown");
    os.removeObserver(this, "metro_softkeyboard_hidden");
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

    let isStartup = !oldHeight && !oldWidth;
    for (let i = Browser.tabs.length - 1; i >=0; i--) {
      let tab = Browser.tabs[i];
      let oldContentWindowWidth = tab.browser.contentWindowWidth;
      tab.updateViewportSize(newWidth, newHeight); 
      
      
      if (!isStartup) {
        
        
        if (tab.browser.contentWindowWidth == oldContentWindowWidth)
          tab.restoreViewportPosition(oldWidth, newWidth);

        tab.updateDefaultZoomLevel();
      }
    }

    
    if (!isStartup) {
      let currentElement = document.activeElement;
      let [scrollbox, scrollInterface] = ScrollUtils.getScrollboxFromElement(currentElement);
      if (scrollbox && scrollInterface && currentElement && currentElement != scrollbox) {
        
        while (currentElement && currentElement.parentNode != scrollbox)
          currentElement = currentElement.parentNode;
  
        setTimeout(function() { scrollInterface.ensureElementIsVisible(currentElement) }, 0);
      }
    }

    this.updateContentArea(newWidth, this._getContentHeightForWindow(newHeight));
    this._fire("SizeChanged");
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
    this._fire("ContentSizeChanged");
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

    this._fire("ViewableSizeChanged");
  },

  observe: function cao_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "metro_softkeyboard_shown":
      case "metro_softkeyboard_hidden": {
        let event = document.createEvent("UIEvents");
        let eventDetails = {
          opened: aTopic == "metro_softkeyboard_shown",
          details: aData
        };

        event.initUIEvent("KeyboardChanged", true, false, window, eventDetails);
        window.dispatchEvent(event);

        this.updateViewableArea();
        break;
      }
    };
  },

  handleEvent: function cao_handleEvent(anEvent) {
    switch (anEvent.type) {
      case 'resize':
        if (anEvent.target != window)
          return;

        ContentAreaObserver.update();
        break;
    }
  },

  _fire: function (aName) {
    
    setTimeout(function() {
      let event = document.createEvent("Events");
      event.initEvent(aName, true, false);
      Elements.browsers.dispatchEvent(event);
    }, 0);
  },

  _getContentHeightForWindow: function (windowHeight) {
    let contextUIHeight = BrowserUI.isTabsOnly ? Elements.toolbar.getBoundingClientRect().bottom : 0;
    return windowHeight - contextUIHeight;
  },

  _getViewableHeightForContent: function (contentHeight) {
    let keyboardHeight = MetroUtils.keyboardHeight;
    return contentHeight - keyboardHeight;
  }
};
