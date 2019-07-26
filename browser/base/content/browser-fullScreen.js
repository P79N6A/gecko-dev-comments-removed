# -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

var FullScreen = {
  _XULNS: "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
  get _fullScrToggler() {
    delete this._fullScrToggler;
    return this._fullScrToggler = document.getElementById("fullscr-toggler");
  },
  toggle: function (event) {
    var enterFS = window.fullScreen;

    
    if (event && event.type == "fullscreen")
      enterFS = !enterFS;

    
    
    document.getElementById("View:FullScreen").setAttribute("checked", enterFS);

#ifdef XP_MACOSX
    
    document.getElementById("enterFullScreenItem").hidden = enterFS;
    document.getElementById("exitFullScreenItem").hidden = !enterFS;
#endif

    
    
    
    
    if (enterFS && this.useLionFullScreen) {
      if (document.mozFullScreen) {
        this.showXULChrome("toolbar", false);
      }
      else {
        gNavToolbox.setAttribute("inFullscreen", true);
        document.documentElement.setAttribute("inFullscreen", true);
      }
      return;
    }

    
    this.showXULChrome("toolbar", !enterFS);

    if (enterFS) {
      
      
      
      
      if (!document.mozFullScreen) {
        this._fullScrToggler.addEventListener("mouseover", this._expandCallback, false);
        this._fullScrToggler.addEventListener("dragenter", this._expandCallback, false);
      }
      if (gPrefService.getBoolPref("browser.fullscreen.autohide"))
        gBrowser.mPanelContainer.addEventListener("mousemove",
                                                  this._collapseCallback, false);

      document.addEventListener("keypress", this._keyToggleCallback, false);
      document.addEventListener("popupshown", this._setPopupOpen, false);
      document.addEventListener("popuphidden", this._setPopupOpen, false);
      
      
      
      this._shouldAnimate = !document.mozFullScreen;
      this.mouseoverToggle(false);

      
      gPrefService.addObserver("browser.fullscreen", this, false);
    }
    else {
      
      this._cancelAnimation();
      gNavToolbox.style.marginTop = "";
      if (this._isChromeCollapsed)
        this.mouseoverToggle(true);
      
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
          this.showWarning(this.fullscreenDoc);
        }
        break;
      case "transitionend":
        if (event.propertyName == "opacity")
          this.cancelWarning();
        break;
    }
  },

  enterDomFullscreen : function(event) {
    if (!document.mozFullScreen)
      return;

    
    
    
    
    
    if (event.target.defaultView.top != gBrowser.contentWindow) {
      document.mozCancelFullScreen();
      return;
    }

    let focusManager = Cc["@mozilla.org/focus-manager;1"].getService(Ci.nsIFocusManager);
    if (focusManager.activeWindow != window) {
      
      
      document.mozCancelFullScreen();
      return;
    }

    
    if (!document.getElementById("sidebar-box").hidden)
      toggleSidebar();

    if (gFindBarInitialized)
      gFindBar.close();

    this.showWarning(event.target);

    
    gBrowser.tabContainer.addEventListener("TabOpen", this.exitDomFullScreen);
    gBrowser.tabContainer.addEventListener("TabClose", this.exitDomFullScreen);
    gBrowser.tabContainer.addEventListener("TabSelect", this.exitDomFullScreen);

    
    
    
    if (!this.useLionFullScreen) {
      window.addEventListener("activate", this);
    }

    
    
    this._cancelAnimation();
    this.mouseoverToggle(false);

    
    
    this._fullScrToggler.removeEventListener("mouseover", this._expandCallback, false);
    this._fullScrToggler.removeEventListener("dragenter", this._expandCallback, false);
  },

  cleanup: function () {
    if (window.fullScreen) {
      gBrowser.mPanelContainer.removeEventListener("mousemove",
                                                   this._collapseCallback, false);
      document.removeEventListener("keypress", this._keyToggleCallback, false);
      document.removeEventListener("popupshown", this._setPopupOpen, false);
      document.removeEventListener("popuphidden", this._setPopupOpen, false);
      gPrefService.removeObserver("browser.fullscreen", this);

      this._fullScrToggler.removeEventListener("mouseover", this._expandCallback, false);
      this._fullScrToggler.removeEventListener("dragenter", this._expandCallback, false);
      this.cancelWarning();
      gBrowser.tabContainer.removeEventListener("TabOpen", this.exitDomFullScreen);
      gBrowser.tabContainer.removeEventListener("TabClose", this.exitDomFullScreen);
      gBrowser.tabContainer.removeEventListener("TabSelect", this.exitDomFullScreen);
      if (!this.useLionFullScreen)
        window.removeEventListener("activate", this);
      this.fullscreenDoc = null;
    }
  },

  observe: function(aSubject, aTopic, aData)
  {
    if (aData == "browser.fullscreen.autohide") {
      if (gPrefService.getBoolPref("browser.fullscreen.autohide")) {
        gBrowser.mPanelContainer.addEventListener("mousemove",
                                                  this._collapseCallback, false);
      }
      else {
        gBrowser.mPanelContainer.removeEventListener("mousemove",
                                                     this._collapseCallback, false);
      }
    }
  },

  
  _expandCallback: function()
  {
    FullScreen.mouseoverToggle(true);
  },
  _collapseCallback: function()
  {
    FullScreen.mouseoverToggle(false);
  },
  _keyToggleCallback: function(aEvent)
  {
    
    
    if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE) {
      FullScreen._shouldAnimate = false;
      FullScreen.mouseoverToggle(false, true);
    }
    
    else if (aEvent.keyCode == aEvent.DOM_VK_F6)
      FullScreen.mouseoverToggle(true);
  },

  
  _isPopupOpen: false,
  _isChromeCollapsed: false,
  _safeToCollapse: function(forceHide)
  {
    if (!gPrefService.getBoolPref("browser.fullscreen.autohide"))
      return false;

    
    if (!forceHide && this._isPopupOpen)
      return false;

    
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
  _isAnimating: false,
  _animationTimeout: 0,
  _animationHandle: 0,
  _animateUp: function() {
    
    if (!window.fullScreen || !this._safeToCollapse(false)) {
      this._isAnimating = false;
      this._shouldAnimate = true;
      return;
    }

    this._animateStartTime = window.mozAnimationStartTime;
    if (!this._animationHandle)
      this._animationHandle = window.mozRequestAnimationFrame(this);
  },

  sample: function (timeStamp) {
    const duration = 1500;
    const timePassed = timeStamp - this._animateStartTime;
    const pos = timePassed >= duration ? 1 :
                1 - Math.pow(1 - timePassed / duration, 4);

    if (pos >= 1) {
      
      this._cancelAnimation();
      gNavToolbox.style.marginTop = "";
      this.mouseoverToggle(false);
      return;
    }

    gNavToolbox.style.marginTop = (gNavToolbox.boxObject.height * pos * -1) + "px";
    this._animationHandle = window.mozRequestAnimationFrame(this);
  },

  _cancelAnimation: function() {
    window.mozCancelAnimationFrame(this._animationHandle);
    this._animationHandle = 0;
    clearTimeout(this._animationTimeout);
    this._isAnimating = false;
    this._shouldAnimate = false;
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
    let uri = this.fullscreenDoc.nodePrincipal.URI;
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
    
    
    
    if (isApproved)
      Services.obs.notifyObservers(this.fullscreenDoc, "fullscreen-approved", "");
    else
      document.mozCancelFullScreen();
  },

  warningBox: null,
  warningFadeOutTimeout: null,
  fullscreenDoc: null,

  
  
  
  showWarning: function(targetDoc) {
    if (!document.mozFullScreen ||
        !gPrefService.getBoolPref("full-screen-api.approval-required"))
      return;

    
    this.fullscreenDoc = targetDoc;
    let uri = this.fullscreenDoc.nodePrincipal.URI;
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

  mouseoverToggle: function(aShow, forceHide)
  {
    
    
    
    
    if (aShow != this._isChromeCollapsed || (!aShow && this._isAnimating) ||
        (!aShow && !this._safeToCollapse(forceHide)))
      return;

    
    
    
    
    if (gPrefService.getIntPref("browser.fullscreen.animateUp") == 0)
      this._shouldAnimate = false;

    if (!aShow && this._shouldAnimate) {
      this._isAnimating = true;
      this._shouldAnimate = false;
      this._animationTimeout = setTimeout(this._animateUp.bind(this), 800);
      return;
    }

    
    if (aShow) {
      gBrowser.mPanelContainer.addEventListener("mousemove",
                                                this._collapseCallback, false);
    }
    else {
      gBrowser.mPanelContainer.removeEventListener("mousemove",
                                                   this._collapseCallback, false);
    }

    
    
    gNavToolbox.style.marginTop =
      aShow ? "" : -gNavToolbox.getBoundingClientRect().height + "px";

    this._fullScrToggler.collapsed = aShow;
    this._isChromeCollapsed = !aShow;
    if (gPrefService.getIntPref("browser.fullscreen.animateUp") == 2)
      this._shouldAnimate = true;
  },

  showXULChrome: function(aTag, aShow)
  {
    var els = document.getElementsByTagNameNS(this._XULNS, aTag);

    for (let el of els) {
      
      if (el.getAttribute("fullscreentoolbar") == "true") {
        if (!aShow) {

          var toolbarMode = el.getAttribute("mode");
          if (toolbarMode != "text") {
            el.setAttribute("saved-mode", toolbarMode);
            el.setAttribute("saved-iconsize", el.getAttribute("iconsize"));
            el.setAttribute("mode", "icons");
            el.setAttribute("iconsize", "small");
          }

          
          
          el.setAttribute("saved-context", el.getAttribute("context"));
          if (el.id == "nav-bar" || el.id == "TabsToolbar")
            el.setAttribute("context", "autohide-context");
          else
            el.removeAttribute("context");

          
          
          el.setAttribute("inFullscreen", true);
        }
        else {
          var restoreAttr = function restoreAttr(attrName) {
            var savedAttr = "saved-" + attrName;
            if (el.hasAttribute(savedAttr)) {
              el.setAttribute(attrName, el.getAttribute(savedAttr));
              el.removeAttribute(savedAttr);
            }
          }

          restoreAttr("mode");
          restoreAttr("iconsize");
          restoreAttr("context");

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
    var ctlsOnTabbar = window.toolbar.visible &&
                       (navbar.collapsed ||
                          (TabsOnTop.enabled &&
                           !gPrefService.getBoolPref("browser.tabs.autoHide")));
    if (fullscreenctls.parentNode == navbar && ctlsOnTabbar) {
      fullscreenctls.removeAttribute("flex");
      document.getElementById("TabsToolbar").appendChild(fullscreenctls);
    }
    else if (fullscreenctls.parentNode.id == "TabsToolbar" && !ctlsOnTabbar) {
      fullscreenctls.setAttribute("flex", "1");
      navbar.appendChild(fullscreenctls);
    }
    fullscreenctls.hidden = aShow;
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
