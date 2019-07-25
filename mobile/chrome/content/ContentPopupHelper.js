



































var ContentPopupHelper = {
  _popup: null,
  get popup() {
    return this._popup;
  },

  set popup(aPopup) {
    
    if (!this._popup && !aPopup)
      return;

    if (aPopup) {
      
      
      
      
      
      
      
      
      
      
      
      
      aPopup.left = 0;
      aPopup.firstChild.style.maxWidth = "0px";
      aPopup.style.visibility = "hidden";
      aPopup.hidden = false;

      window.addEventListener("TabSelect", this, false);
      window.addEventListener("TabClose", this, false);
      window.addEventListener("AnimatedZoomBegin", this, false);
      window.addEventListener("AnimatedZoomEnd", this, false);
      window.addEventListener("MozBeforeResize", this, true);
      window.addEventListener("resize", this, false);
      Elements.browsers.addEventListener("PanBegin", this, false);
      Elements.browsers.addEventListener("PanFinished", this, false);
    } else {
      this._popup.hidden = true;
      this._anchorRect = null;

      window.removeEventListener("TabSelect", this, false);
      window.removeEventListener("TabClose", this, false);
      window.removeEventListener("AnimatedZoomBegin", this, false);
      window.removeEventListener("AnimatedZoomEnd", this, false);
      window.removeEventListener("MozBeforeResize", this, true);
      window.removeEventListener("resize", this, false);
      Elements.browsers.removeEventListener("PanBegin", this, false);
      Elements.browsers.removeEventListener("PanFinished", this, false);

      let event = document.createEvent("Events");
      event.initEvent("contentpopuphidden", true, false);
      this._popup.dispatchEvent(event);
    }

    this._popup = aPopup;
  },

  





  anchorTo: function(aAnchorRect) {
    let popup = this._popup;
    if (!popup)
      return;

    
    this._anchorRect = aAnchorRect ? aAnchorRect : this._anchorRect;

    
    
    let [leftVis, rightVis, leftW, rightW] = Browser.computeSidebarVisibility();
    let leftOffset = leftVis * leftW;
    let rightOffset = rightVis * rightW;
    let visibleAreaWidth = window.innerWidth - leftOffset - rightOffset;
    popup.firstChild.style.maxWidth = (visibleAreaWidth * 0.75) + "px";

    let browser = getBrowser();
    let rect = this._anchorRect.clone().scale(browser.scale, browser.scale);
    let scroll = browser.getRootView().getPosition();

    
    
    let topOffset = (BrowserUI.toolbarH - Browser.getScrollboxPosition(Browser.pageScrollboxScroller).y);

    
    let notification = Browser.getNotificationBox().currentNotification;
    if (notification)
      topOffset += notification.getBoundingClientRect().height;

    let virtualContentRect = {
      width: rect.width,
      height: rect.height,
      left: Math.ceil(rect.left - scroll.x + leftOffset - rightOffset),
      right: Math.floor(rect.left + rect.width - scroll.x + leftOffset - rightOffset),
      top: Math.ceil(rect.top - scroll.y + topOffset),
      bottom: Math.floor(rect.top + rect.height - scroll.y + topOffset)
    };

    
    
    if (virtualContentRect.left + virtualContentRect.width > visibleAreaWidth) {
      let offsetX = visibleAreaWidth - (virtualContentRect.left + virtualContentRect.width);
      virtualContentRect.width += offsetX;
      virtualContentRect.right -= offsetX;
    }

    if (virtualContentRect.left < leftOffset) {
      let offsetX = (virtualContentRect.right - virtualContentRect.width);
      virtualContentRect.width += offsetX;
      virtualContentRect.left -= offsetX;
    }

    
    let browserRect = Rect.fromRect(browser.getBoundingClientRect());
    if (BrowserUI.isToolbarLocked()) {
      
      
      let toolbarH = BrowserUI.toolbarH;
      browserRect = new Rect(leftOffset - rightOffset, Math.max(0, browserRect.top - toolbarH) + toolbarH,
                             browserRect.width + leftOffset - rightOffset, browserRect.height - toolbarH);
    }

    if (browserRect.intersect(Rect.fromRect(virtualContentRect)).isEmpty()) {
      popup.style.visibility = "hidden";
      return;
    }

    
    let left = rect.left - scroll.x + leftOffset - rightOffset;
    let top = rect.top - scroll.y + topOffset + (rect.height);

    
    let arrowboxRect = Rect.fromRect(popup.getBoundingClientRect());
    if (left + arrowboxRect.width > window.innerWidth)
      left -= (left + arrowboxRect.width - window.innerWidth);
    else if (left < leftOffset)
      left += (leftOffset - left);
    popup.left = left;

    
    let buttonsHeight = Elements.contentNavigator.getBoundingClientRect().height;
    if (top + arrowboxRect.height >= window.innerHeight - buttonsHeight)
      top -= (rect.height + arrowboxRect.height);
    popup.top = top;

    
    let virtualContentElement = {
      getBoundingClientRect: function() {
        return virtualContentRect;
      }
    };
    popup.anchorTo(virtualContentElement);
    popup.style.visibility = "visible";

    let event = document.createEvent("Events");
    event.initEvent("contentpopupshown", true, false);
    popup.dispatchEvent(event);
  },

  handleEvent: function(aEvent) {
    let popup = this._popup;
    if (!popup || !this._anchorRect)
      return;

    switch(aEvent.type) {
      case "TabSelect":
      case "TabClose":
        this.popup = null;
        break;

      case "PanBegin":
      case "AnimatedZoomBegin":
        popup.left = 0;
        popup.style.visibility = "hidden";
        break;

      case "PanFinished":
      case "AnimatedZoomEnd":
        popup.style.visibility = "visible";
        this.anchorTo();
        break;

      case "MozBeforeResize":
        popup.left = 0;
        break;

      case "resize":
        this.anchorTo();
        break;
    }
  }
};

