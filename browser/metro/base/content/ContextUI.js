




const kContextUITabsShowEvent = "MozContextUITabsShow";






var ContextUI = {
  _expandable: true,
  _hidingId: 0,

  



  init: function init() {
    Elements.browsers.addEventListener('URLChanged', this, true);
    Elements.browsers.addEventListener("AlertActive", this, true);
    Elements.browsers.addEventListener("AlertClose", this, true);
    Elements.tabList.addEventListener('TabSelect', this, true);
    Elements.panelUI.addEventListener('ToolPanelShown', this, false);
    Elements.panelUI.addEventListener('ToolPanelHidden', this, false);

    window.addEventListener("touchstart", this, true);
    window.addEventListener("mousedown", this, true);
    window.addEventListener("MozEdgeUIStarted", this, true);
    window.addEventListener("MozEdgeUICanceled", this, true);
    window.addEventListener("MozEdgeUICompleted", this, true);
    window.addEventListener("keypress", this, true);
    window.addEventListener("KeyboardChanged", this, false);
    window.addEventListener("MozFlyoutPanelShowing", this, false);

    Elements.tray.addEventListener("transitionend", this, true);

    Appbar.init();
  },

  



  
  get isVisible() {
    return this.navbarVisible || this.tabbarVisible || this.contextAppbarVisible;
  },

  
  get navbarVisible() {
    return (Elements.navbar.hasAttribute("visible") ||
            Elements.navbar.hasAttribute("startpage"));
  },

  
  get tabbarVisible() {
    return Elements.tray.hasAttribute("expanded");
  },

  
  get contextAppbarVisible() {
    return Elements.contextappbar.isShowing;
  },

  
  get isExpandable() { return this._expandable; },
  set isExpandable(aFlag) {
    this._expandable = aFlag;
    if (!this._expandable)
      this.dismiss();
  },

  



  


  toggleNavUI: function () {
    
    
    
    if (this.tabbarVisible) {
      this.dismiss();
    } else {
      this.displayNavUI();
    }
  },

  



  displayNavUI: function () {
    let shown = false;

    if (!this.navbarVisible) {
      BrowserUI.updateURI();
      this.displayNavbar();
      shown = true;
    }

    if (!this.tabbarVisible) {
      this.displayTabs();
      shown = true;
    }

    if (shown) {
      ContentAreaObserver.update(window.innerWidth, window.innerHeight);
    }

    return shown;
  },

  



  dismiss: function () {
    let dismissed = false;

    this._clearDelayedTimeout();

    
    
    if (this.navbarVisible) {
      BrowserUI.blurNavBar();
      this.dismissNavbar();
      dismissed = true;
    }
    if (this.tabbarVisible) {
      this.dismissTabs();
      dismissed = true;
    }
    if (Elements.contextappbar.isShowing) {
      this.dismissContextAppbar();
      dismissed = true;
    }

    if (dismissed) {
      ContentAreaObserver.update(window.innerWidth, window.innerHeight);
    }

    return dismissed;
  },

  


  peekTabs: function peekTabs(aDelay) {
    if (!this.tabbarVisible)
      this.displayTabs();

    ContextUI.dismissTabsWithDelay(aDelay);
  },

  


  dismissTabsWithDelay: function (aDelay) {
    aDelay = aDelay || kNewTabAnimationDelayMsec;
    this._clearDelayedTimeout();
    this._hidingId = setTimeout(function () {
        ContextUI.dismissTabs();
      }, aDelay);
  },

  
  displayNavbar: function () {
    Elements.navbar.show();
    ContentAreaObserver.updateContentArea();
  },

  
  displayTabs: function () {
    this._clearDelayedTimeout();
    this._setIsExpanded(true);
  },

  
  dismissNavbar: function dismissNavbar() {
    if (!BrowserUI.isStartTabVisible) {
      Elements.autocomplete.closePopup();
      Elements.navbar.dismiss();
      ContentAreaObserver.updateContentArea();
    }
  },

  
  dismissTabs: function dimissTabs() {
    this._clearDelayedTimeout();
    this._setIsExpanded(false);
  },

  
  dismissContextAppbar: function dismissContextAppbar() {
    Elements.contextappbar.dismiss();
  },

  



  
  _setIsExpanded: function _setIsExpanded(aFlag, setSilently) {
    
    if (!this.isExpandable || this.tabbarVisible == aFlag)
      return;

    if (aFlag)
      Elements.tray.setAttribute("expanded", "true");
    else
      Elements.tray.removeAttribute("expanded");

    if (!setSilently)
      this._fire(kContextUITabsShowEvent);
  },

  _clearDelayedTimeout: function _clearDelayedTimeout() {
    if (this._hidingId) {
      clearTimeout(this._hidingId);
      this._hidingId = 0;
    }
  },

  



  _onEdgeUIStarted: function(aEvent) {
    this._hasEdgeSwipeStarted = true;
    this._clearDelayedTimeout();
    this.toggleNavUI();
  },

  _onEdgeUICanceled: function(aEvent) {
    this._hasEdgeSwipeStarted = false;
    this.dismiss();
  },

  _onEdgeUICompleted: function(aEvent) {
    if (this._hasEdgeSwipeStarted) {
      this._hasEdgeSwipeStarted = false;
      return;
    }

    this._clearDelayedTimeout();
    this.toggleNavUI();
  },

  onDownInput: function onDownInput(aEvent) {
    if (!this.isVisible) {
      return;
    }

    
    let whitelist = [
      
      
      Elements.tabs,
      
      
      
      Browser.getNotificationBox()
    ];

    if (whitelist.some(elem => elem.contains(aEvent.target))) {
      return;
    }

    
    if (BrowserUI.isStartTabVisible) {
      ContextUI.dismissTabs();
      return;
    }

    
    if (aEvent.target.ownerDocument.defaultView.top == getBrowser().contentWindow) {
      this.dismiss();
      return;
    }

    
    this.dismissTabs();
    this.dismissContextAppbar();
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "URLChanged":
        this.displayNavbar();
        break;
      case "MozEdgeUIStarted":
        this._onEdgeUIStarted(aEvent);
        break;
      case "MozEdgeUICanceled":
        this._onEdgeUICanceled(aEvent);
        break;
      case "MozEdgeUICompleted":
        this._onEdgeUICompleted(aEvent);
        break;
      case "keypress":
        if (String.fromCharCode(aEvent.which) == "z" &&
            aEvent.getModifierState("Win"))
          this.toggleNavUI();
        break;
      case "transitionend":
        setTimeout(function () {
          ContentAreaObserver.updateContentArea();
        }, 0);
        break;
      case "KeyboardChanged":
        this.dismissTabs();
        break;
      case "mousedown":
        if (aEvent.button != 0) {
          break;
        }
        this.onDownInput(aEvent);
        break;
      case "touchstart":
        this.onDownInput(aEvent);
        break;
      case "ToolPanelShown":
      case "ToolPanelHidden":
        this.dismiss();
        break;
      case "AlertActive":
      case "AlertClose":
        ContentAreaObserver.updateContentArea();
        break;
      case "touchstart":
        if (!BrowserUI.isStartTabVisible) {
          this.dismiss();
        }
        break;
      case "MozFlyoutPanelShowing":
        if (BrowserUI.isStartTabVisible) {
          this.dismissTabs();
          this.dismissContextAppbar();
        } else {
          this.dismiss();
        }
        break;
    }
  },

  _fire: function (name) {
    let event = document.createEvent("Events");
    event.initEvent(name, true, true);
    window.dispatchEvent(event);
  }
};
