# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

var FullScreen = {
  _XULNS: "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",

  init: function() {
    
    window.addEventListener("fullscreen", this, true);
    window.messageManager.addMessageListener("MozEnteredDomFullscreen", this);
    window.messageManager.addMessageListener("MozExitedDomFullscreen", this);

    if (window.fullScreen)
      this.toggle();
  },

  uninit: function() {
    window.messageManager.removeMessageListener("MozEnteredDomFullscreen", this);
    window.messageManager.removeMessageListener("MozExitedDomFullscreen", this);
    this.cleanup();
  },

  toggle: function (event) {
    var enterFS = window.fullScreen;

    
    if (event && event.type == "fullscreen")
      enterFS = !enterFS;

    
    
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

    
    this.showXULChrome("toolbar", !enterFS);

    if (enterFS) {
      document.addEventListener("keypress", this._keyToggleCallback, false);
      document.addEventListener("popupshown", this._setPopupOpen, false);
      document.addEventListener("popuphidden", this._setPopupOpen, false);
      this._shouldAnimate = true;
      
      
      
      this.hideNavToolbox(document.mozFullScreen);
    }
    else {
      this.showNavToolbox(false);
      
      this._isPopupOpen = false;
      this.cleanup();
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
        this.toggle(event);
        break;
      case "transitionend":
        if (event.propertyName == "opacity")
          this.cancelWarning();
        break;
    }
  },

  receiveMessage: function(aMessage) {
    if (aMessage.name == "MozEnteredDomFullscreen") {
      
      
      
      
      
      let data = aMessage.data;
      let browser = aMessage.target;
      if (gMultiProcessBrowser && browser.getAttribute("remote") == "true") {
        let windowUtils = window.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDOMWindowUtils);
        windowUtils.remoteFrameFullscreenChanged(browser, data.origin);
      }
      this.enterDomFullscreen(browser, data.origin);
    } else if (aMessage.name == "MozExitedDomFullscreen") {
      document.documentElement.removeAttribute("inDOMFullscreen");
      this.cleanupDomFullscreen();
      this.showNavToolbox();
      
      
      if (window.fullScreen) {
        this._shouldAnimate = true;
        this.hideNavToolbox();
      }
    }
  },

  enterDomFullscreen : function(aBrowser, aOrigin) {
    if (!document.mozFullScreen)
      return;

    
    
    
    
    if (gBrowser.selectedBrowser != aBrowser) {
      document.mozCancelFullScreen();
      return;
    }

    let focusManager = Services.focus;
    if (focusManager.activeWindow != window) {
      
      
      document.mozCancelFullScreen();
      return;
    }

    document.documentElement.setAttribute("inDOMFullscreen", true);

    if (gFindBarInitialized)
      gFindBar.close();

    this.showWarning(aOrigin);

    
    gBrowser.tabContainer.addEventListener("TabOpen", this.exitDomFullScreen);
    gBrowser.tabContainer.addEventListener("TabClose", this.exitDomFullScreen);
    gBrowser.tabContainer.addEventListener("TabSelect", this.exitDomFullScreen);

    
    
    
    if (!this.useLionFullScreen) {
      window.addEventListener("activate", this);
    }

    
    
    this.hideNavToolbox(true);
    this._fullScrToggler.hidden = true;
  },

  cleanup: function () {
    if (window.fullScreen) {
      MousePosTracker.removeListener(this);
      document.removeEventListener("keypress", this._keyToggleCallback, false);
      document.removeEventListener("popupshown", this._setPopupOpen, false);
      document.removeEventListener("popuphidden", this._setPopupOpen, false);

      this.cleanupDomFullscreen();
    }
  },

  cleanupDomFullscreen: function () {
    this.cancelWarning();
    gBrowser.tabContainer.removeEventListener("TabOpen", this.exitDomFullScreen);
    gBrowser.tabContainer.removeEventListener("TabClose", this.exitDomFullScreen);
    gBrowser.tabContainer.removeEventListener("TabSelect", this.exitDomFullScreen);
    if (!this.useLionFullScreen)
      window.removeEventListener("activate", this);

    window.messageManager
          .broadcastAsyncMessage("DOMFullscreen:Cleanup");
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
      FullScreen.hideNavToolbox(true);
    }
    
    else if (aEvent.keyCode == aEvent.DOM_VK_F6)
      FullScreen.showNavToolbox();
  },

  
  _isPopupOpen: false,
  _isChromeCollapsed: false,
  _safeToCollapse: function(forceHide)
  {
    if (!gPrefService.getBoolPref("browser.fullscreen.autohide"))
      return false;

    if (!forceHide) {
      
      if (this._isPopupOpen)
        return false;
      
      if (this.useLionFullScreen)
        return false;
    }

    
    if (document.commandDispatcher.focusedElement &&
        document.commandDispatcher.focusedElement.ownerDocument == document &&
        document.commandDispatcher.focusedElement.localName == "input") {
      if (forceHide)
        
        document.commandDispatcher.focusedElement.blur();
      else
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

  
  _shouldAnimate: true,

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
        let host = uri.host;
        var onFullscreenchange = function onFullscreenchange(event) {
          if (event.target == document && document.mozFullScreenElement == null) {
            
            Services.perms.remove(host, "fullscreen");
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

    
    this._isChromeCollapsed = false;
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
  },

  hideNavToolbox: function(forceHide = false) {
    this._fullScrToggler.hidden = document.mozFullScreen;
    if (this._isChromeCollapsed) {
      if (forceHide) {
        gNavToolbox.removeAttribute("fullscreenShouldAnimate");
      }
      return;
    }
    if (!this._safeToCollapse(forceHide)) {
      this._fullScrToggler.hidden = true;
      return;
    }

    
    
    
    
    let animateUp = gPrefService.getIntPref("browser.fullscreen.animateUp");
    if (animateUp == 0) {
      this._shouldAnimate = false;
    } else if (animateUp == 2) {
      this._shouldAnimate = true;
    }
    if (this._shouldAnimate && !forceHide) {
      gNavToolbox.setAttribute("fullscreenShouldAnimate", true);
      this._shouldAnimate = false;
      
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

  showXULChrome: function(aTag, aShow)
  {
    var els = document.getElementsByTagNameNS(this._XULNS, aTag);

    for (let el of els) {
      
      if (el.getAttribute("fullscreentoolbar") == "true") {
        if (!aShow) {
          
          
          el.setAttribute("saved-context", el.getAttribute("context"));
          if (el.id == "nav-bar" || el.id == "TabsToolbar")
            el.setAttribute("context", "autohide-context");
          else
            el.removeAttribute("context");

          
          
          el.setAttribute("inFullscreen", true);
        }
        else {
          if (el.hasAttribute("saved-context")) {
            el.setAttribute("context", el.getAttribute("saved-context"));
            el.removeAttribute("saved-context");
          }
          el.removeAttribute("inFullscreen");
        }
      } else {
        
        
        if (aShow)
          el.removeAttribute("moz-collapsed");
        else
          el.setAttribute("moz-collapsed", "true");
      }
    }

    if (aShow) {
      gNavToolbox.removeAttribute("inFullscreen");
      document.documentElement.removeAttribute("inFullscreen");
    } else {
      gNavToolbox.setAttribute("inFullscreen", true);
      document.documentElement.setAttribute("inFullscreen", true);
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
    fullscreenctls.hidden = aShow;

    ToolbarIconColor.inferFromText();
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
