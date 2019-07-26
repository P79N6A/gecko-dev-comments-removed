




const kContextUIShowEvent = "MozContextUIShow";

const kContextUIDismissEvent = "MozContextUIDismiss";

const kContextUITabsShowEvent = "MozContextUITabsShow";



const kHideContextAndTrayDelayMsec = 3000;


const kNewTabAnimationDelayMsec = 1000;





var ContextUI = {
  _expandable: true,
  _hidingId: 0,

  



  init: function init() {
    Elements.browsers.addEventListener("mousedown", this, true);
    Elements.browsers.addEventListener("touchstart", this, true);
    Elements.browsers.addEventListener("AlertActive", this, true);

    Elements.browsers.addEventListener('URLChanged', this, true);
    Elements.tabList.addEventListener('TabSelect', this, true);
    Elements.panelUI.addEventListener('ToolPanelShown', this, false);
    Elements.panelUI.addEventListener('ToolPanelHidden', this, false);

    window.addEventListener("MozEdgeUIStarted", this, true);
    window.addEventListener("MozEdgeUICanceled", this, true);
    window.addEventListener("MozEdgeUICompleted", this, true);
    window.addEventListener("keypress", this, true);
    window.addEventListener("KeyboardChanged", this, false);

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
      this.displayNavbar();
      shown = true;
    }

    if (!this.tabbarVisible) {
      this.displayTabs();
      shown = true;
    }

    if (shown) {
      ContentAreaObserver.update(window.innerWidth, window.innerHeight);
      this._fire(kContextUIShowEvent);
    }

    return shown;
  },

  



  dismiss: function () {
    let dismissed = false;

    this._clearDelayedTimeout();

    
    
    if (this.navbarVisible) {
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
      this._fire(kContextUIDismissEvent);
    }

    return dismissed;
  },

  


  peekTabs: function peekTabs() {
    if (this.tabbarVisible) {
      setTimeout(function () {
        ContextUI.dismissTabsWithDelay(kNewTabAnimationDelayMsec);
      }, 0);
    } else {
      BrowserUI.setOnTabAnimationEnd(function () {
        ContextUI.dismissTabsWithDelay(kNewTabAnimationDelayMsec);
      });
      this.displayTabs();
    }
  },

  


  dismissTabsWithDelay: function (aDelay) {
    aDelay = aDelay || kHideContextAndTrayDelayMsec;
    this._clearDelayedTimeout();
    this._hidingId = setTimeout(function () {
        ContextUI.dismissTabs();
      }, aDelay);
  },

  
  cancelDismiss: function cancelDismiss() {
    this._clearDelayedTimeout();
  },

  
  displayNavbar: function () {
    this._clearDelayedTimeout();
    Elements.navbar.show();
  },

  
  displayTabs: function () {
    this._clearDelayedTimeout();
    this._setIsExpanded(true);
  },

  
  displayContextAppbar: function () {
    this._clearDelayedTimeout();
    Elements.contextappbar.show();
  },

  
  dismissNavbar: function dismissNavbar() {
    Elements.navbar.dismiss();
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

    if (StartUI.hide()) {
      this.dismiss();
      return;
    }
    this.toggleNavUI();
  },

  _onEdgeUICanceled: function(aEvent) {
    this._hasEdgeSwipeStarted = false;
    StartUI.hide();
    this.dismiss();
  },

  _onEdgeUICompleted: function(aEvent) {
    if (this._hasEdgeSwipeStarted) {
      this._hasEdgeSwipeStarted = false;
      return;
    }

    this._clearDelayedTimeout();
    if (StartUI.hide()) {
      this.dismiss();
      return;
    }
    this.toggleNavUI();
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
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
        if (aEvent.button == 0 && this.isVisible)
          this.dismiss();
        break;
      case 'URLChanged':
        this.dismissTabs();
        break;
      case 'TabSelect':
        this.dismissTabs();
        break;

      case 'ToolPanelShown':
      case 'ToolPanelHidden':
      case "touchstart":
      case "AlertActive":
        this.dismiss();
        break;
    }
  },

  _fire: function (name) {
    let event = document.createEvent("Events");
    event.initEvent(name, true, true);
    window.dispatchEvent(event);
  }
};
