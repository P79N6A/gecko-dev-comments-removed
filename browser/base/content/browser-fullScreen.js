# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

var FullScreen = {
  _MESSAGES: [
    "DOMFullscreen:Request",
    "DOMFullscreen:NewOrigin",
    "DOMFullscreen:Exit",
    "DOMFullscreen:Painted",
  ],

  init: function() {
    
    window.addEventListener("fullscreen", this, true);
    window.addEventListener("MozDOMFullscreen:Entered", this,
                             true,
                             false);
    window.addEventListener("MozDOMFullscreen:Exited", this,
                             true,
                             false);
    for (let type of this._MESSAGES) {
      window.messageManager.addMessageListener(type, this);
    }

    if (window.fullScreen)
      this.toggle();
  },

  uninit: function() {
    for (let type of this._MESSAGES) {
      window.messageManager.removeMessageListener(type, this);
    }
    this.cleanup();
  },

  toggle: function () {
    var enterFS = window.fullScreen;

    
    
    let fullscreenCommand = document.getElementById("View:FullScreen");
    if (enterFS) {
      fullscreenCommand.setAttribute("checked", enterFS);
    } else {
      fullscreenCommand.removeAttribute("checked");
    }

#ifdef XP_MACOSX
    
    document.getElementById("enterFullScreenItem").hidden = enterFS;
    document.getElementById("exitFullScreenItem").hidden = !enterFS;
#endif

    if (!this._fullScrToggler) {
      this._fullScrToggler = document.getElementById("fullscr-toggler");
      this._fullScrToggler.addEventListener("mouseover", this._expandCallback, false);
      this._fullScrToggler.addEventListener("dragenter", this._expandCallback, false);
    }

    if (enterFS) {
      gNavToolbox.setAttribute("inFullscreen", true);
      document.documentElement.setAttribute("inFullscreen", true);
      if (!document.mozFullScreen && this.useLionFullScreen)
        document.documentElement.setAttribute("OSXLionFullscreen", true);
    } else {
      gNavToolbox.removeAttribute("inFullscreen");
      document.documentElement.removeAttribute("inFullscreen");
      document.documentElement.removeAttribute("OSXLionFullscreen");
    }

    if (!document.mozFullScreen)
      this._updateToolbars(enterFS);

    if (enterFS) {
      document.addEventListener("keypress", this._keyToggleCallback, false);
      document.addEventListener("popupshown", this._setPopupOpen, false);
      document.addEventListener("popuphidden", this._setPopupOpen, false);
      
      if (!document.mozFullScreen)
        this.hideNavToolbox(true);
    }
    else {
      this.showNavToolbox(false);
      
      this._isPopupOpen = false;
      this.cleanup();
      
      
      
      
      
      TabsInTitlebar.updateAppearance(true);
    }
  },

  exitDomFullScreen : function() {
    document.mozCancelFullScreen();
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "activate":
        if (document.mozFullScreen) {
          this.showWarning(this.fullscreenOrigin);
        }
        break;
      case "fullscreen":
        this.toggle();
        break;
      case "transitionend":
        if (event.propertyName == "opacity")
          this.cancelWarning();
        break;
      case "MozDOMFullscreen:Entered": {
        
        
        
        
        
        
        
        let browser;
        if (event.target == gBrowser) {
          browser = event.originalTarget;
        } else {
          let topWin = event.target.ownerDocument.defaultView.top;
          browser = gBrowser.getBrowserForContentWindow(topWin);
        }
        if (!browser || !this.enterDomFullscreen(browser)) {
          if (document.mozFullScreen) {
            
            
            
            setTimeout(() => document.mozCancelFullScreen(), 0);
          }
          break;
        }
        
        
        
        
        if (this._isRemoteBrowser(browser)) {
          browser.messageManager.sendAsyncMessage("DOMFullscreen:Entered");
        }
        break;
      }
      case "MozDOMFullscreen:Exited":
        this.cleanupDomFullscreen();
        break;
    }
  },

  receiveMessage: function(aMessage) {
    let browser = aMessage.target;
    switch (aMessage.name) {
      case "DOMFullscreen:Request": {
        this._windowUtils.remoteFrameFullscreenChanged(browser);
        break;
      }
      case "DOMFullscreen:NewOrigin": {
        this.showWarning(aMessage.data.originNoSuffix);
        break;
      }
      case "DOMFullscreen:Exit": {
        this._windowUtils.remoteFrameFullscreenReverted();
        break;
      }
      case "DOMFullscreen:Painted": {
        Services.obs.notifyObservers(window, "fullscreen-painted", "");
        break;
      }
    }
  },

  enterDomFullscreen : function(aBrowser) {
    if (!document.mozFullScreen)
      return false;

    
    
    
    
    if (gBrowser.selectedBrowser != aBrowser) {
      return false;
    }

    let focusManager = Services.focus;
    if (focusManager.activeWindow != window) {
      
      
      return false;
    }

    document.documentElement.setAttribute("inDOMFullscreen", true);

    if (gFindBarInitialized)
      gFindBar.close();

    
    gBrowser.tabContainer.addEventListener("TabOpen", this.exitDomFullScreen);
    gBrowser.tabContainer.addEventListener("TabClose", this.exitDomFullScreen);
    gBrowser.tabContainer.addEventListener("TabSelect", this.exitDomFullScreen);

    
    
    
    window.addEventListener("activate", this);

    return true;
  },

  cleanup: function () {
    if (!window.fullScreen) {
      MousePosTracker.removeListener(this);
      document.removeEventListener("keypress", this._keyToggleCallback, false);
      document.removeEventListener("popupshown", this._setPopupOpen, false);
      document.removeEventListener("popuphidden", this._setPopupOpen, false);
    }
  },

  cleanupDomFullscreen: function () {
    this.cancelWarning();
    gBrowser.tabContainer.removeEventListener("TabOpen", this.exitDomFullScreen);
    gBrowser.tabContainer.removeEventListener("TabClose", this.exitDomFullScreen);
    gBrowser.tabContainer.removeEventListener("TabSelect", this.exitDomFullScreen);
    window.removeEventListener("activate", this);

    document.documentElement.removeAttribute("inDOMFullscreen");

    window.messageManager
          .broadcastAsyncMessage("DOMFullscreen:CleanUp");
  },

  _isRemoteBrowser: function (aBrowser) {
    return gMultiProcessBrowser && aBrowser.getAttribute("remote") == "true";
  },

  get _windowUtils() {
    return window.QueryInterface(Ci.nsIInterfaceRequestor)
                 .getInterface(Ci.nsIDOMWindowUtils);
  },

  getMouseTargetRect: function()
  {
    return this._mouseTargetRect;
  },

  
  _expandCallback: function()
  {
    FullScreen.showNavToolbox();
  },
  onMouseEnter: function()
  {
    FullScreen.hideNavToolbox();
  },
  _keyToggleCallback: function(aEvent)
  {
    
    
    if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE) {
      FullScreen.hideNavToolbox();
    }
    
    else if (aEvent.keyCode == aEvent.DOM_VK_F6)
      FullScreen.showNavToolbox();
  },

  
  _isPopupOpen: false,
  _isChromeCollapsed: false,
  _safeToCollapse: function () {
    if (!gPrefService.getBoolPref("browser.fullscreen.autohide"))
      return false;

    
    if (this._isPopupOpen)
      return false;

    
    if (this.useLionFullScreen)
      return false;

    
    if (document.commandDispatcher.focusedElement &&
        document.commandDispatcher.focusedElement.ownerDocument == document &&
        document.commandDispatcher.focusedElement.localName == "input") {
      return false;
    }

    return true;
  },

  _setPopupOpen: function(aEvent)
  {
    
    
    
    
    if (aEvent.type == "popupshown" && !FullScreen._isChromeCollapsed &&
        aEvent.target.localName != "tooltip" && aEvent.target.localName != "window")
      FullScreen._isPopupOpen = true;
    else if (aEvent.type == "popuphidden" && aEvent.target.localName != "tooltip" &&
             aEvent.target.localName != "window")
      FullScreen._isPopupOpen = false;
  },

  
  getAutohide: function(aItem)
  {
    aItem.setAttribute("checked", gPrefService.getBoolPref("browser.fullscreen.autohide"));
  },
  setAutohide: function()
  {
    gPrefService.setBoolPref("browser.fullscreen.autohide", !gPrefService.getBoolPref("browser.fullscreen.autohide"));
  },

  cancelWarning: function(event) {
    if (!this.warningBox)
      return;
    this.warningBox.removeEventListener("transitionend", this);
    if (this.warningFadeOutTimeout) {
      clearTimeout(this.warningFadeOutTimeout);
      this.warningFadeOutTimeout = null;
    }

    
    
    
    
    gBrowser.selectedBrowser.focus();

    this.warningBox.setAttribute("hidden", true);
    this.warningBox.removeAttribute("fade-warning-out");
    this.warningBox.removeAttribute("obscure-browser");
    this.warningBox = null;
  },

  setFullscreenAllowed: function(isApproved) {
    
    
    
    let rememberCheckbox = document.getElementById("full-screen-remember-decision");
    let uri = BrowserUtils.makeURI(this.fullscreenOrigin);
    if (!rememberCheckbox.hidden) {
      if (rememberCheckbox.checked)
        Services.perms.add(uri,
                           "fullscreen",
                           isApproved ? Services.perms.ALLOW_ACTION : Services.perms.DENY_ACTION,
                           Services.perms.EXPIRE_NEVER);
      else if (isApproved) {
        
        
        
        
        
        Services.perms.add(uri,
                           "fullscreen",
                           Services.perms.ALLOW_ACTION,
                           Services.perms.EXPIRE_SESSION);
        var onFullscreenchange = function onFullscreenchange(event) {
          if (event.target == document && document.mozFullScreenElement == null) {
            
            Services.perms.remove(uri, "fullscreen");
            document.removeEventListener("mozfullscreenchange", onFullscreenchange);
          }
        }
        document.addEventListener("mozfullscreenchange", onFullscreenchange);
      }
    }
    if (this.warningBox)
      this.warningBox.setAttribute("fade-warning-out", "true");
    
    
    
    if (isApproved) {
      gBrowser.selectedBrowser
              .messageManager
              .sendAsyncMessage("DOMFullscreen:Approved");
    } else {
      document.mozCancelFullScreen();
    }
  },

  warningBox: null,
  warningFadeOutTimeout: null,

  
  
  
  showWarning: function(aOrigin) {
    if (!document.mozFullScreen ||
        !gPrefService.getBoolPref("full-screen-api.approval-required"))
      return;

    
    this.fullscreenOrigin = aOrigin;
    let uri = BrowserUtils.makeURI(aOrigin);
    let host = null;
    try {
      host = uri.host;
    } catch (e) { }
    let hostLabel = document.getElementById("full-screen-domain-text");
    let rememberCheckbox = document.getElementById("full-screen-remember-decision");
    let isApproved = false;
    if (host) {
      
      
      let utils = {};
      Cu.import("resource://gre/modules/DownloadUtils.jsm", utils);
      let displayHost = utils.DownloadUtils.getURIHost(uri.spec)[0];
      let bundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");

      hostLabel.textContent = bundle.formatStringFromName("fullscreen.entered", [displayHost], 1);
      hostLabel.removeAttribute("hidden");

      rememberCheckbox.label = bundle.formatStringFromName("fullscreen.rememberDecision", [displayHost], 1);
      rememberCheckbox.checked = false;
      rememberCheckbox.removeAttribute("hidden");

      
      
      isApproved = Services.perms.testPermission(uri, "fullscreen") == Services.perms.ALLOW_ACTION;
    } else {
      hostLabel.setAttribute("hidden", "true");
      rememberCheckbox.setAttribute("hidden", "true");
    }

    
    
    if (!this.warningBox) {
      this.warningBox = document.getElementById("full-screen-warning-container");
      
      this.warningBox.addEventListener("transitionend", this);
      this.warningBox.removeAttribute("hidden");
    } else {
      if (this.warningFadeOutTimeout) {
        clearTimeout(this.warningFadeOutTimeout);
        this.warningFadeOutTimeout = null;
      }
      this.warningBox.removeAttribute("fade-warning-out");
    }

    
    
    
    
    
    
    let authUI = document.getElementById("full-screen-approval-pane");
    if (isApproved) {
      authUI.setAttribute("hidden", "true");
      this.warningBox.removeAttribute("obscure-browser");
    } else {
      
      this.warningBox.setAttribute("obscure-browser", "true");
      authUI.removeAttribute("hidden");
    }

    
    
    if (isApproved)
      this.warningFadeOutTimeout =
        setTimeout(
          function() {
            if (this.warningBox)
              this.warningBox.setAttribute("fade-warning-out", "true");
          }.bind(this),
          3000);
  },

  showNavToolbox: function(trackMouse = true) {
    this._fullScrToggler.hidden = true;
    gNavToolbox.removeAttribute("fullscreenShouldAnimate");
    gNavToolbox.style.marginTop = "";

    if (!this._isChromeCollapsed) {
      return;
    }

    
    if (trackMouse && !this.useLionFullScreen) {
      let rect = gBrowser.mPanelContainer.getBoundingClientRect();
      this._mouseTargetRect = {
        top: rect.top + 50,
        bottom: rect.bottom,
        left: rect.left,
        right: rect.right
      };
      MousePosTracker.addListener(this);
    }

    this._isChromeCollapsed = false;
  },

  hideNavToolbox: function (aAnimate = false) {
    if (this._isChromeCollapsed || !this._safeToCollapse())
      return;

    this._fullScrToggler.hidden = false;

    if (aAnimate && gPrefService.getBoolPref("browser.fullscreen.animate")) {
      gNavToolbox.setAttribute("fullscreenShouldAnimate", true);
      
      let listener = () => {
        gNavToolbox.removeEventListener("transitionend", listener, true);
        if (this._isChromeCollapsed)
          this._fullScrToggler.hidden = false;
      };
      gNavToolbox.addEventListener("transitionend", listener, true);
      this._fullScrToggler.hidden = true;
    }

    gNavToolbox.style.marginTop =
      -gNavToolbox.getBoundingClientRect().height + "px";
    this._isChromeCollapsed = true;
    MousePosTracker.removeListener(this);
  },

  _updateToolbars: function (aEnterFS) {
    for (let el of document.querySelectorAll("toolbar[fullscreentoolbar=true]")) {
      if (aEnterFS) {
        
        
        el.setAttribute("saved-context", el.getAttribute("context"));
        if (el.id == "nav-bar" || el.id == "TabsToolbar")
          el.setAttribute("context", "autohide-context");
        else
          el.removeAttribute("context");

        
        
        el.setAttribute("inFullscreen", true);
      } else {
        if (el.hasAttribute("saved-context")) {
          el.setAttribute("context", el.getAttribute("saved-context"));
          el.removeAttribute("saved-context");
        }
        el.removeAttribute("inFullscreen");
      }
    }

    ToolbarIconColor.inferFromText();

    
    
    
    
    if (this.useLionFullScreen) {
      return;
    }

    var fullscreenctls = document.getElementById("window-controls");
    var navbar = document.getElementById("nav-bar");
    var ctlsOnTabbar = window.toolbar.visible;
    if (fullscreenctls.parentNode == navbar && ctlsOnTabbar) {
      fullscreenctls.removeAttribute("flex");
      document.getElementById("TabsToolbar").appendChild(fullscreenctls);
    }
    else if (fullscreenctls.parentNode.id == "TabsToolbar" && !ctlsOnTabbar) {
      fullscreenctls.setAttribute("flex", "1");
      navbar.appendChild(fullscreenctls);
    }
    fullscreenctls.hidden = !aEnterFS;
  }
};
XPCOMUtils.defineLazyGetter(FullScreen, "useLionFullScreen", function() {
  
  
  
  
#ifdef XP_MACOSX
  return parseFloat(Services.sysinfo.getProperty("version")) >= 11 &&
         document.documentElement.getAttribute("fullscreenbutton") == "true";
#else
  return false;
#endif
});
