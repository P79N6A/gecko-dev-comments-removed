




let SocialUI,
    SocialFlyout,
    SocialMarks,
    SocialShare,
    SocialSidebar,
    SocialStatus,
    SocialActivationListener;

(function() {

XPCOMUtils.defineLazyModuleGetter(this, "PanelFrame",
  "resource:///modules/PanelFrame.jsm");

XPCOMUtils.defineLazyGetter(this, "OpenGraphBuilder", function() {
  let tmp = {};
  Cu.import("resource:///modules/Social.jsm", tmp);
  return tmp.OpenGraphBuilder;
});

XPCOMUtils.defineLazyGetter(this, "DynamicResizeWatcher", function() {
  let tmp = {};
  Cu.import("resource:///modules/Social.jsm", tmp);
  return tmp.DynamicResizeWatcher;
});

XPCOMUtils.defineLazyGetter(this, "sizeSocialPanelToContent", function() {
  let tmp = {};
  Cu.import("resource:///modules/Social.jsm", tmp);
  return tmp.sizeSocialPanelToContent;
});

XPCOMUtils.defineLazyGetter(this, "CreateSocialStatusWidget", function() {
  let tmp = {};
  Cu.import("resource:///modules/Social.jsm", tmp);
  return tmp.CreateSocialStatusWidget;
});

XPCOMUtils.defineLazyGetter(this, "CreateSocialMarkWidget", function() {
  let tmp = {};
  Cu.import("resource:///modules/Social.jsm", tmp);
  return tmp.CreateSocialMarkWidget;
});

XPCOMUtils.defineLazyGetter(this, "hookWindowCloseForPanelClose", function() {
  let tmp = {};
  Cu.import("resource://gre/modules/MozSocialAPI.jsm", tmp);
  return tmp.hookWindowCloseForPanelClose;
});

SocialUI = {
  _initialized: false,

  
  init: function SocialUI_init() {
    if (this._initialized) {
      return;
    }

    Services.obs.addObserver(this, "social:ambient-notification-changed", false);
    Services.obs.addObserver(this, "social:profile-changed", false);
    Services.obs.addObserver(this, "social:frameworker-error", false);
    Services.obs.addObserver(this, "social:providers-changed", false);
    Services.obs.addObserver(this, "social:provider-reload", false);
    Services.obs.addObserver(this, "social:provider-enabled", false);
    Services.obs.addObserver(this, "social:provider-disabled", false);

    Services.prefs.addObserver("social.toast-notifications.enabled", this, false);

    CustomizableUI.addListener(this);
    SocialActivationListener.init();

    
    
    document.getElementById("viewSidebarMenu").addEventListener("popupshowing", SocialSidebar.populateSidebarMenu, true);
    document.getElementById("social-statusarea-popup").addEventListener("popupshowing", SocialSidebar.populateSidebarMenu, true);

    Social.init().then((update) => {
      if (update)
        this._providersChanged();
      
      SocialSidebar.restoreWindowState();
    });

    this._initialized = true;
  },

  
  uninit: function SocialUI_uninit() {
    if (!this._initialized) {
      return;
    }
    SocialSidebar.saveWindowState();

    Services.obs.removeObserver(this, "social:ambient-notification-changed");
    Services.obs.removeObserver(this, "social:profile-changed");
    Services.obs.removeObserver(this, "social:frameworker-error");
    Services.obs.removeObserver(this, "social:providers-changed");
    Services.obs.removeObserver(this, "social:provider-reload");
    Services.obs.removeObserver(this, "social:provider-enabled");
    Services.obs.removeObserver(this, "social:provider-disabled");

    Services.prefs.removeObserver("social.toast-notifications.enabled", this);
    CustomizableUI.removeListener(this);
    SocialActivationListener.uninit();

    document.getElementById("viewSidebarMenu").removeEventListener("popupshowing", SocialSidebar.populateSidebarMenu, true);
    document.getElementById("social-statusarea-popup").removeEventListener("popupshowing", SocialSidebar.populateSidebarMenu, true);

    this._initialized = false;
  },

  observe: function SocialUI_observe(subject, topic, data) {
    switch (topic) {
      case "social:provider-enabled":
        SocialMarks.populateToolbarPalette();
        SocialStatus.populateToolbarPalette();
        break;
      case "social:provider-disabled":
        SocialMarks.removeProvider(data);
        SocialStatus.removeProvider(data);
        SocialSidebar.disableProvider(data);
        break;
      case "social:provider-reload":
        SocialStatus.reloadProvider(data);
        
        
        if (!SocialSidebar.provider || SocialSidebar.provider.origin != data)
          return;
        
        
        
        
        SocialSidebar.unloadSidebar();
        SocialFlyout.unload();
        
        
      case "social:providers-changed":
        this._providersChanged();
        break;
      
      case "social:ambient-notification-changed":
        SocialStatus.updateButton(data);
        break;
      case "social:profile-changed":
        
        
        
        SocialStatus.updateButton(data);
        break;
      case "social:frameworker-error":
        if (this.enabled && SocialSidebar.provider && SocialSidebar.provider.origin == data) {
          SocialSidebar.setSidebarErrorMessage();
        }
        break;
      case "nsPref:changed":
        if (data == "social.toast-notifications.enabled") {
          SocialSidebar.updateToggleNotifications();
        }
        break;
    }
  },

  _providersChanged: function() {
    SocialSidebar.clearProviderMenus();
    SocialSidebar.update();
    SocialShare.populateProviderMenu();
    SocialStatus.populateToolbarPalette();
    SocialMarks.populateToolbarPalette();
  },

  showLearnMore: function() {
    let url = Services.urlFormatter.formatURLPref("app.support.baseURL") + "social-api";
    openUILinkIn(url, "tab");
  },

  closeSocialPanelForLinkTraversal: function (target, linkNode) {
    
    if (target == "" || target == "_self")
      return;

    
    let win = linkNode.ownerDocument.defaultView;
    let container = win.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIWebNavigation)
                                  .QueryInterface(Ci.nsIDocShell)
                                  .chromeEventHandler;
    let containerParent = container.parentNode;
    if (containerParent.classList.contains("social-panel") &&
        containerParent instanceof Ci.nsIDOMXULPopupElement) {
      
      setTimeout(() => {
        containerParent.hidePopup();
      }, 0);
    }
  },

  get _chromeless() {
    
    let docElem = document.documentElement;
    
    
    let chromeless = docElem.getAttribute("chromehidden").includes("extrachrome") ||
                     docElem.getAttribute('chromehidden').includes("toolbar");
    
    
    delete this._chromeless;
    this._chromeless = chromeless;
    return chromeless;
  },

  get enabled() {
    
    if (this._chromeless || PrivateBrowsingUtils.isWindowPrivate(window))
      return false;
    return Social.providers.length > 0;
  },

  canShareOrMarkPage: function(aURI) {
    
    
    if (PrivateBrowsingUtils.isWindowPrivate(window))
      return false;

    return (aURI && (aURI.schemeIs('http') || aURI.schemeIs('https')));
  },

  onCustomizeEnd: function(aWindow) {
    if (aWindow != window)
      return;
    
    
    let canShare = this.canShareOrMarkPage(gBrowser.currentURI);
    let shareButton = SocialShare.shareButton;
    if (shareButton) {
      if (canShare) {
        shareButton.removeAttribute("disabled")
      } else {
        shareButton.setAttribute("disabled", "true")
      }
    }
    
    for (let node of SocialMarks.nodes) {
      if (canShare) {
        node.removeAttribute("disabled")
      } else {
        node.setAttribute("disabled", "true")
      }
    }
  },

  
  
  updateState: function() {
    if (location == "about:customizing")
      return;
    goSetCommandEnabled("Social:PageShareOrMark", this.canShareOrMarkPage(gBrowser.currentURI));
    if (!SocialUI.enabled)
      return;
    
    SocialMarks.update();
  }
}


SocialActivationListener = {
  init: function() {
    messageManager.addMessageListener("Social:Activation", this);
  },
  uninit: function() {
    messageManager.removeMessageListener("Social:Activation", this);
  },
  receiveMessage: function(aMessage) {
    let data = aMessage.json;
    let browser = aMessage.target;
    data.window = window;
    
    
    
    let options;
    if (browser == SocialShare.iframe && Services.prefs.getBoolPref("social.share.activationPanelEnabled")) {
      options = { bypassContentCheck: true, bypassInstallPanel: true };
    }

    
    
    if (PrivateBrowsingUtils.isWindowPrivate(window))
      return;
    Social.installProvider(data, function(manifest) {
      Social.activateFromOrigin(manifest.origin, function(provider) {
        if (provider.sidebarURL) {
          SocialSidebar.show(provider.origin);
        }
        if (provider.shareURL) {
          
          
          
          
          let widget = CustomizableUI.getWidget("social-share-button");
          if (!widget.areaType) {
            CustomizableUI.addWidgetToArea("social-share-button", CustomizableUI.AREA_NAVBAR);
            
            SocialUI.onCustomizeEnd(window);
          }

          
          
          SocialShare._createFrame();
          SocialShare.iframe.setAttribute('src', 'data:text/plain;charset=utf8,');
          SocialShare.iframe.setAttribute('origin', provider.origin);
          
          SocialShare.populateProviderMenu();
          if (SocialShare.panel.state == "open") {
            SocialShare.sharePage(provider.origin);
          }
        }
        if (provider.postActivationURL) {
          
          
          gBrowser.loadOneTab(provider.postActivationURL, {inBackground: SocialShare.panel.state == "open"});
        }
      });
    }, options);
  }
}

SocialFlyout = {
  get panel() {
    return document.getElementById("social-flyout-panel");
  },

  get iframe() {
    if (!this.panel.firstChild)
      this._createFrame();
    return this.panel.firstChild;
  },

  dispatchPanelEvent: function(name) {
    let doc = this.iframe.contentDocument;
    let evt = doc.createEvent("CustomEvent");
    evt.initCustomEvent(name, true, true, {});
    doc.documentElement.dispatchEvent(evt);
  },

  _createFrame: function() {
    let panel = this.panel;
    if (!SocialUI.enabled || panel.firstChild)
      return;
    
    let iframe = document.createElement("iframe");
    iframe.setAttribute("type", "content");
    iframe.setAttribute("class", "social-panel-frame");
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("tooltip", "aHTMLTooltip");
    iframe.setAttribute("origin", SocialSidebar.provider.origin);
    panel.appendChild(iframe);
  },

  setFlyoutErrorMessage: function SF_setFlyoutErrorMessage() {
    this.iframe.removeAttribute("src");
    this.iframe.webNavigation.loadURI("about:socialerror?mode=compactInfo&origin=" +
                                 encodeURIComponent(this.iframe.getAttribute("origin")),
                                 null, null, null, null);
    sizeSocialPanelToContent(this.panel, this.iframe);
  },

  unload: function() {
    let panel = this.panel;
    panel.hidePopup();
    if (!panel.firstChild)
      return
    let iframe = panel.firstChild;
    if (iframe.socialErrorListener)
      iframe.socialErrorListener.remove();
    panel.removeChild(iframe);
  },

  onShown: function(aEvent) {
    let panel = this.panel;
    let iframe = this.iframe;
    this._dynamicResizer = new DynamicResizeWatcher();
    iframe.docShell.isActive = true;
    iframe.docShell.isAppTab = true;
    if (iframe.contentDocument.readyState == "complete") {
      this._dynamicResizer.start(panel, iframe);
      this.dispatchPanelEvent("socialFrameShow");
    } else {
      
      iframe.addEventListener("load", function panelBrowserOnload(e) {
        iframe.removeEventListener("load", panelBrowserOnload, true);
        setTimeout(function() {
          if (SocialFlyout._dynamicResizer) { 
            SocialFlyout._dynamicResizer.start(panel, iframe);
            SocialFlyout.dispatchPanelEvent("socialFrameShow");
          }
        }, 0);
      }, true);
    }
  },

  onHidden: function(aEvent) {
    this._dynamicResizer.stop();
    this._dynamicResizer = null;
    this.iframe.docShell.isActive = false;
    this.dispatchPanelEvent("socialFrameHide");
  },

  load: function(aURL, cb) {
    if (!SocialSidebar.provider)
      return;

    this.panel.hidden = false;
    let iframe = this.iframe;
    
    
    let src = iframe.contentDocument && iframe.contentDocument.documentURIObject;
    if (!src || !src.equalsExceptRef(Services.io.newURI(aURL, null, null))) {
      iframe.addEventListener("load", function documentLoaded() {
        iframe.removeEventListener("load", documentLoaded, true);
        cb();
      }, true);
      Social.setErrorListener(iframe, SocialFlyout.setFlyoutErrorMessage.bind(SocialFlyout))
      iframe.setAttribute("src", aURL);
    } else {
      
      
      iframe.setAttribute("src", aURL);
      cb();
    }
  },

  open: function(aURL, yOffset, aCallback) {
    
    document.getElementById("social-notification-panel").hidePopup();

    if (!SocialUI.enabled)
      return;
    let panel = this.panel;
    let iframe = this.iframe;

    this.load(aURL, function() {
      sizeSocialPanelToContent(panel, iframe);
      let anchor = document.getElementById("social-sidebar-browser");
      if (panel.state == "open") {
        panel.moveToAnchor(anchor, "start_before", 0, yOffset, false);
      } else {
        panel.openPopup(anchor, "start_before", 0, yOffset, false, false);
      }
      if (aCallback) {
        try {
          aCallback(iframe.contentWindow);
        } catch(e) {
          Cu.reportError(e);
        }
      }
    });
  }
}

SocialShare = {
  get _dynamicResizer() {
    delete this._dynamicResizer;
    this._dynamicResizer = new DynamicResizeWatcher();
    return this._dynamicResizer;
  },

  
  
  get anchor() {
    let widget = CustomizableUI.getWidget("social-share-button");
    return widget.forWindow(window).anchor;
  },
  get panel() {
    return document.getElementById("social-share-panel");
  },

  get iframe() {
    
    
    return this.panel.lastChild.firstChild;
  },

  uninit: function () {
    if (this.iframe) {
      this.iframe.remove();
    }
  },

  _createFrame: function() {
    let panel = this.panel;
    if (this.iframe)
      return;
    this.panel.hidden = false;
    
    let iframe = document.createElement("browser");
    iframe.setAttribute("type", "content");
    iframe.setAttribute("class", "social-share-frame");
    iframe.setAttribute("context", "contentAreaContextMenu");
    iframe.setAttribute("tooltip", "aHTMLTooltip");
    iframe.setAttribute("disableglobalhistory", "true");
    iframe.setAttribute("flex", "1");
    panel.lastChild.appendChild(iframe);
    iframe.addEventListener("load", function _firstload() {
      iframe.removeEventListener("load", _firstload, true);
      iframe.messageManager.loadFrameScript("chrome://browser/content/content.js", true);
    }, true);
    this.populateProviderMenu();
  },

  getSelectedProvider: function() {
    let provider;
    let lastProviderOrigin = this.iframe && this.iframe.getAttribute("origin");
    if (lastProviderOrigin) {
      provider = Social._getProviderFromOrigin(lastProviderOrigin);
    }
    return provider;
  },

  createTooltip: function(event) {
    let tt = event.target;
    let provider = Social._getProviderFromOrigin(tt.triggerNode.getAttribute("origin"));
    tt.firstChild.setAttribute("value", provider.name);
    tt.lastChild.setAttribute("value", provider.origin);
  },

  populateProviderMenu: function() {
    if (!this.iframe)
      return;
    let providers = [p for (p of Social.providers) if (p.shareURL)];
    let hbox = document.getElementById("social-share-provider-buttons");
    
    
    let addButton = document.getElementById("add-share-provider");
    while (hbox.lastChild != addButton) {
      hbox.removeChild(hbox.lastChild);
    }
    let selectedProvider = this.getSelectedProvider();
    for (let provider of providers) {
      let button = document.createElement("toolbarbutton");
      button.setAttribute("class", "toolbarbutton-1 share-provider-button");
      button.setAttribute("type", "radio");
      button.setAttribute("group", "share-providers");
      button.setAttribute("image", provider.iconURL);
      button.setAttribute("tooltip", "share-button-tooltip");
      button.setAttribute("origin", provider.origin);
      button.setAttribute("label", provider.name);
      button.setAttribute("oncommand", "SocialShare.sharePage(this.getAttribute('origin'));");
      if (provider == selectedProvider) {
        this.defaultButton = button;
      }
      hbox.appendChild(button);
    }
    if (!this.defaultButton) {
      this.defaultButton = addButton;
    }
    this.defaultButton.setAttribute("checked", "true");
  },

  get shareButton() {
    
    
    
    if (!window.CustomizableUI)
      return null;
    let widget = CustomizableUI.getWidget("social-share-button");
    if (!widget || !widget.areaType)
      return null;
    return widget.forWindow(window).node;
  },

  _onclick: function() {
    Services.telemetry.getHistogramById("SOCIAL_PANEL_CLICKS").add(0);
  },

  onShowing: function() {
    this.anchor.setAttribute("open", "true");
    this.iframe.addEventListener("click", this._onclick, true);
  },

  onHidden: function() {
    this.anchor.removeAttribute("open");
    this.iframe.removeEventListener("click", this._onclick, true);
    this.iframe.setAttribute("src", "data:text/plain;charset=utf8,");
    
    this.iframe.docShell.createAboutBlankContentViewer(null);
    this.currentShare = null;
    
    if (this.iframe.sessionHistory) {
      let purge = this.iframe.sessionHistory.count;
      if (purge > 0)
        this.iframe.sessionHistory.PurgeHistory(purge);
    }
  },

  setErrorMessage: function() {
    let iframe = this.iframe;
    if (!iframe)
      return;

    let url;
    let origin = iframe.getAttribute("origin");
    if (!origin) {
      
      url = "about:socialerror?mode=tryAgainOnly&directory=1&url=" + encodeURIComponent(iframe.getAttribute("src"));
    } else {
      url = "about:socialerror?mode=compactInfo&origin=" + encodeURIComponent(origin);
    }
    iframe.webNavigation.loadURI(url, null, null, null, null);
    sizeSocialPanelToContent(this.panel, iframe);
  },

  sharePage: function(providerOrigin, graphData, target) {
    
    
    
    this._createFrame();
    let iframe = this.iframe;

    
    
    
    
    
    let pageData = graphData ? graphData : this.currentShare;
    let sharedURI = pageData ? Services.io.newURI(pageData.url, null, null) :
                                gBrowser.currentURI;
    if (!SocialUI.canShareOrMarkPage(sharedURI))
      return;

    
    
    
    
    let _dataFn;
    if (!pageData || sharedURI == gBrowser.currentURI) {
      messageManager.addMessageListener("PageMetadata:PageDataResult", _dataFn = (msg) => {
        messageManager.removeMessageListener("PageMetadata:PageDataResult", _dataFn);
        let pageData = msg.json;
        if (graphData) {
          
          for (let p in graphData) {
            pageData[p] = graphData[p];
          }
        }
        this.sharePage(providerOrigin, pageData, target);
      });
      gBrowser.selectedBrowser.messageManager.sendAsyncMessage("PageMetadata:GetPageData");
      return;
    }
    
    if (!pageData.microdata && target) {
      messageManager.addMessageListener("PageMetadata:MicrodataResult", _dataFn = (msg) => {
        messageManager.removeMessageListener("PageMetadata:MicrodataResult", _dataFn);
        pageData.microdata = msg.data;
        this.sharePage(providerOrigin, pageData, target);
      });
      gBrowser.selectedBrowser.messageManager.sendAsyncMessage("PageMetadata:GetMicrodata", null, { target });
      return;
    }
    this.currentShare = pageData;

    let provider;
    if (providerOrigin)
      provider = Social._getProviderFromOrigin(providerOrigin);
    else
      provider = this.getSelectedProvider();
    if (!provider || !provider.shareURL) {
      this.showDirectory();
      return;
    }
    
    let hbox = document.getElementById("social-share-provider-buttons");
    let btn = hbox.querySelector("[origin='" + provider.origin + "']");
    if (btn)
      btn.checked = true;

    let shareEndpoint = OpenGraphBuilder.generateEndpointURL(provider.shareURL, pageData);

    this._dynamicResizer.stop();
    let size = provider.getPageSize("share");
    if (size) {
      
      
      
      delete size.width;
    }

    
    
    let endpointMatch = shareEndpoint == iframe.getAttribute("src");
    if (endpointMatch) {
      this._dynamicResizer.start(iframe.parentNode, iframe, size);
      iframe.docShell.isActive = true;
      iframe.docShell.isAppTab = true;
      let evt = iframe.contentDocument.createEvent("CustomEvent");
      evt.initCustomEvent("OpenGraphData", true, true, JSON.stringify(pageData));
      iframe.contentDocument.documentElement.dispatchEvent(evt);
    } else {
      iframe.parentNode.setAttribute("loading", "true");
      
      iframe.addEventListener("load", function panelBrowserOnload(e) {
        iframe.removeEventListener("load", panelBrowserOnload, true);
        iframe.docShell.isActive = true;
        iframe.docShell.isAppTab = true;
        iframe.parentNode.removeAttribute("loading");
        
        
        
        iframe.contentWindow.opener = iframe.contentWindow;

        SocialShare._dynamicResizer.start(iframe.parentNode, iframe, size);

        let evt = iframe.contentDocument.createEvent("CustomEvent");
        evt.initCustomEvent("OpenGraphData", true, true, JSON.stringify(pageData));
        iframe.contentDocument.documentElement.dispatchEvent(evt);
      }, true);
    }
    
    
    if (iframe.sessionHistory) {
      let purge = iframe.sessionHistory.count;
      if (purge > 0)
        iframe.sessionHistory.PurgeHistory(purge);
    }

    
    let uri = Services.io.newURI(shareEndpoint, null, null);
    iframe.setAttribute("origin", provider.origin);
    iframe.setAttribute("src", shareEndpoint);
    this._openPanel();
  },

  showDirectory: function() {
    this._createFrame();
    let iframe = this.iframe;
    if (iframe.getAttribute("src") == "about:providerdirectory")
      return;
    iframe.removeAttribute("origin");
    iframe.parentNode.setAttribute("loading", "true");
    iframe.addEventListener("DOMContentLoaded", function _dcl(e) {
      iframe.removeEventListener("DOMContentLoaded", _dcl, true);
      iframe.parentNode.removeAttribute("loading");
    }, true);

    iframe.addEventListener("load", function panelBrowserOnload(e) {
      iframe.removeEventListener("load", panelBrowserOnload, true);

      hookWindowCloseForPanelClose(iframe.contentWindow);
      SocialShare._dynamicResizer.start(iframe.parentNode, iframe);

      iframe.addEventListener("unload", function panelBrowserOnload(e) {
        iframe.removeEventListener("unload", panelBrowserOnload, true);
        SocialShare._dynamicResizer.stop();
      }, true);
    }, true);
    iframe.setAttribute("src", "about:providerdirectory");
    this._openPanel();
  },

  _openPanel: function() {
    let anchor = document.getAnonymousElementByAttribute(this.anchor, "class", "toolbarbutton-icon");
    this.panel.openPopup(anchor, "bottomcenter topright", 0, 0, false, false);
    Social.setErrorListener(this.iframe, this.setErrorMessage.bind(this));
    Services.telemetry.getHistogramById("SOCIAL_TOOLBAR_BUTTONS").add(0);
  }
};

SocialSidebar = {
  _openStartTime: 0,

  
  get canShow() {
    if (!SocialUI.enabled || document.mozFullScreen)
      return false;
    return Social.providers.some(p => p.sidebarURL);
  },

  
  get opened() {
    let broadcaster = document.getElementById("socialSidebarBroadcaster");
    return !broadcaster.hidden;
  },

  restoreWindowState: function() {
    
    
    
    
    
    this._initialized = true;
    if (!this.canShow)
      return;

    if (Services.prefs.prefHasUserValue("social.provider.current")) {
      
      
      
      let origin = Services.prefs.getCharPref("social.provider.current");
      Services.prefs.clearUserPref("social.provider.current");
      
      
      let opened = origin && true;
      if (Services.prefs.prefHasUserValue("social.sidebar.open")) {
        opened = origin && Services.prefs.getBoolPref("social.sidebar.open");
        Services.prefs.clearUserPref("social.sidebar.open");
      }
      let data = {
        "hidden": !opened,
        "origin": origin
      };
      SessionStore.setWindowValue(window, "socialSidebar", JSON.stringify(data));
    }

    let data = SessionStore.getWindowValue(window, "socialSidebar");
    
    if (!data && window.opener && !window.opener.closed) {
      try {
        data = SessionStore.getWindowValue(window.opener, "socialSidebar");
      } catch(e) {
        
        
        
      }
    }
    if (data) {
      data = JSON.parse(data);
      document.getElementById("social-sidebar-browser").setAttribute("origin", data.origin);
      if (!data.hidden)
        this.show(data.origin);
    } else if (Services.prefs.prefHasUserValue("social.sidebar.provider")) {
      
      this.show(Services.prefs.getCharPref("social.sidebar.provider"));
    }
  },

  saveWindowState: function() {
    let broadcaster = document.getElementById("socialSidebarBroadcaster");
    let sidebarOrigin = document.getElementById("social-sidebar-browser").getAttribute("origin");
    let data = {
      "hidden": broadcaster.hidden,
      "origin": sidebarOrigin
    };
    if (broadcaster.hidden) {
      Services.telemetry.getHistogramById("SOCIAL_SIDEBAR_OPEN_DURATION").add(Date.now()  / 1000 - this._openStartTime);
    } else {
      this._openStartTime = Date.now() / 1000;
    }

    
    if (broadcaster.hidden)
      Services.prefs.clearUserPref("social.sidebar.provider");
    else
      Services.prefs.setCharPref("social.sidebar.provider", sidebarOrigin);

    try {
      SessionStore.setWindowValue(window, "socialSidebar", JSON.stringify(data));
    } catch(e) {
      
    }
  },

  setSidebarVisibilityState: function(aEnabled) {
    let sbrowser = document.getElementById("social-sidebar-browser");
    
    
    if (aEnabled == sbrowser.docShellIsActive)
      return;
    sbrowser.docShellIsActive = aEnabled;
    let evt = sbrowser.contentDocument.createEvent("CustomEvent");
    evt.initCustomEvent(aEnabled ? "socialFrameShow" : "socialFrameHide", true, true, {});
    sbrowser.contentDocument.documentElement.dispatchEvent(evt);
  },

  updateToggleNotifications: function() {
    let command = document.getElementById("Social:ToggleNotifications");
    command.setAttribute("checked", Services.prefs.getBoolPref("social.toast-notifications.enabled"));
    command.setAttribute("hidden", !SocialUI.enabled);
  },

  update: function SocialSidebar_update() {
    
    if (!this._initialized)
      return;
    this.ensureProvider();
    this.updateToggleNotifications();
    this._updateHeader();
    clearTimeout(this._unloadTimeoutId);
    
    let command = document.getElementById("Social:ToggleSidebar");
    command.setAttribute("hidden", this.canShow ? "false" : "true");

    
    
    let hideSidebar = !this.canShow || !this.opened;
    let broadcaster = document.getElementById("socialSidebarBroadcaster");
    broadcaster.hidden = hideSidebar;
    command.setAttribute("checked", !hideSidebar);

    let sbrowser = document.getElementById("social-sidebar-browser");

    if (hideSidebar) {
      sbrowser.removeEventListener("load", SocialSidebar._loadListener, true);
      this.setSidebarVisibilityState(false);
      
      
      
      if (!this.canShow) {
        this.unloadSidebar();
      } else {
        this._unloadTimeoutId = setTimeout(
          this.unloadSidebar,
          Services.prefs.getIntPref("social.sidebar.unload_timeout_ms")
        );
      }
    } else {
      sbrowser.setAttribute("origin", this.provider.origin);
      if (this.provider.errorState == "frameworker-error") {
        SocialSidebar.setSidebarErrorMessage();
        return;
      }

      
      if (sbrowser.getAttribute("src") != this.provider.sidebarURL) {
        
        
        sbrowser.docShell.createAboutBlankContentViewer(null);
        Social.setErrorListener(sbrowser, this.setSidebarErrorMessage.bind(this));
        
        sbrowser.docShell.isAppTab = true;
        sbrowser.setAttribute("src", this.provider.sidebarURL);
        PopupNotifications.locationChange(sbrowser);
      }

      
      if (sbrowser.contentDocument.readyState != "complete") {
        document.getElementById("social-sidebar-button").setAttribute("loading", "true");
        sbrowser.addEventListener("load", SocialSidebar._loadListener, true);
      } else {
        this.setSidebarVisibilityState(true);
      }
    }
    this._updateCheckedMenuItems(this.opened && this.provider ? this.provider.origin : null);
  },

  _onclick: function() {
    Services.telemetry.getHistogramById("SOCIAL_PANEL_CLICKS").add(3);
  },

  _loadListener: function SocialSidebar_loadListener() {
    let sbrowser = document.getElementById("social-sidebar-browser");
    sbrowser.removeEventListener("load", SocialSidebar._loadListener, true);
    document.getElementById("social-sidebar-button").removeAttribute("loading");
    SocialSidebar.setSidebarVisibilityState(true);
    sbrowser.addEventListener("click", SocialSidebar._onclick, true);
  },

  unloadSidebar: function SocialSidebar_unloadSidebar() {
    let sbrowser = document.getElementById("social-sidebar-browser");
    if (!sbrowser.hasAttribute("origin"))
      return;

    sbrowser.removeEventListener("click", SocialSidebar._onclick, true);
    sbrowser.stop();
    sbrowser.removeAttribute("origin");
    sbrowser.setAttribute("src", "about:blank");
    
    
    
    sbrowser.docShell.createAboutBlankContentViewer(null);
    SocialFlyout.unload();
  },

  _unloadTimeoutId: 0,

  setSidebarErrorMessage: function() {
    let sbrowser = document.getElementById("social-sidebar-browser");
    
    let origin = sbrowser.getAttribute("origin");
    if (origin) {
      origin = "&origin=" + encodeURIComponent(origin);
    }
    if (this.provider.errorState == "frameworker-error") {
      sbrowser.setAttribute("src", "about:socialerror?mode=workerFailure" + origin);
    } else {
      let url = encodeURIComponent(this.provider.sidebarURL);
      sbrowser.loadURI("about:socialerror?mode=tryAgain&url=" + url + origin, null, null);
    }
  },

  _provider: null,
  ensureProvider: function() {
    if (this._provider)
      return;
    
    
    let sbrowser = document.getElementById("social-sidebar-browser");
    let origin = sbrowser.getAttribute("origin");
    let providers = [p for (p of Social.providers) if (p.sidebarURL)];
    let provider;
    if (origin)
      provider = Social._getProviderFromOrigin(origin);
    if (!provider && providers.length > 0)
      provider = providers[0];
    if (provider)
      this.provider = provider;
  },

  get provider() {
    return this._provider;
  },

  set provider(provider) {
    if (!provider || provider.sidebarURL) {
      this._provider = provider;
      this._updateHeader();
      this._updateCheckedMenuItems(provider && provider.origin);
      this.update();
    }
  },

  disableProvider: function(origin) {
    if (this._provider && this._provider.origin == origin) {
      this._provider = null;
      
      this.ensureProvider();
    }
  },

  _updateHeader: function() {
    let provider = this.provider;
    let image, title;
    if (provider) {
      image = "url(" + (provider.icon32URL || provider.iconURL) + ")";
      title = provider.name;
    }
    document.getElementById("social-sidebar-favico").style.listStyleImage = image;
    document.getElementById("social-sidebar-title").value = title;
  },

  _updateCheckedMenuItems: function(origin) {
    
    let menuitems = document.getElementsByClassName("social-provider-menuitem");
    for (let mi of menuitems) {
      if (origin && mi.getAttribute("origin") == origin) {
        mi.setAttribute("checked", "true");
        mi.setAttribute("oncommand", "SocialSidebar.hide();");
      } else if (mi.getAttribute("checked")) {
        mi.removeAttribute("checked");
        mi.setAttribute("oncommand", "SocialSidebar.show(this.getAttribute('origin'));");
      }
    }
  },

  show: function(origin) {
    
    let broadcaster = document.getElementById("socialSidebarBroadcaster");
    broadcaster.hidden = false;
    if (origin)
      this.provider = Social._getProviderFromOrigin(origin);
    else
      SocialSidebar.update();
    this.saveWindowState();
    Services.telemetry.getHistogramById("SOCIAL_SIDEBAR_STATE").add(true);
  },

  hide: function() {
    let broadcaster = document.getElementById("socialSidebarBroadcaster");
    broadcaster.hidden = true;
    this._updateCheckedMenuItems();
    this.clearProviderMenus();
    SocialSidebar.update();
    this.saveWindowState();
    Services.telemetry.getHistogramById("SOCIAL_SIDEBAR_STATE").add(false);
  },

  toggleSidebar: function SocialSidebar_toggle() {
    let broadcaster = document.getElementById("socialSidebarBroadcaster");
    if (broadcaster.hidden)
      this.show();
    else
      this.hide();
  },

  populateSidebarMenu: function(event) {
    
    
    
    
    let popup = event.target;
    let providerMenuSeps = popup.getElementsByClassName("social-provider-menu");
    if (providerMenuSeps[0].previousSibling.nodeName == "menuseparator")
      SocialSidebar.populateProviderMenu(providerMenuSeps[0]);
  },

  clearProviderMenus: function() {
    
    
    let providerMenuSeps = document.getElementsByClassName("social-provider-menu");
    for (let providerMenuSep of providerMenuSeps) {
      while (providerMenuSep.previousSibling.nodeName == "menuitem") {
        let menu = providerMenuSep.parentNode;
        menu.removeChild(providerMenuSep.previousSibling);
      }
    }
  },

  populateProviderMenu: function(providerMenuSep) {
    let menu = providerMenuSep.parentNode;
    
    
    while (providerMenuSep.previousSibling.nodeName == "menuitem") {
      menu.removeChild(providerMenuSep.previousSibling);
    }
    
    let providers = [p for (p of Social.providers) if (p.sidebarURL)];
    if (providers.length < 2 && menu.id != "viewSidebarMenu") {
      providerMenuSep.hidden = true;
      return;
    }
    let topSep = providerMenuSep.previousSibling;
    for (let provider of providers) {
      let menuitem = document.createElement("menuitem");
      menuitem.className = "menuitem-iconic social-provider-menuitem";
      menuitem.setAttribute("image", provider.iconURL);
      menuitem.setAttribute("label", provider.name);
      menuitem.setAttribute("origin", provider.origin);
      if (this.opened && provider == this.provider) {
        menuitem.setAttribute("checked", "true");
        menuitem.setAttribute("oncommand", "SocialSidebar.hide();");
      } else {
        menuitem.setAttribute("oncommand", "SocialSidebar.show(this.getAttribute('origin'));");
      }
      menu.insertBefore(menuitem, providerMenuSep);
    }
    topSep.hidden = topSep.nextSibling == providerMenuSep;
    providerMenuSep.hidden = !providerMenuSep.nextSibling;
  }
}










function ToolbarHelper(type, createButtonFn, listener) {
  this._createButton = createButtonFn;
  this._type = type;

  if (listener) {
    CustomizableUI.addListener(listener);
    
    window.addEventListener("unload", () => {
      CustomizableUI.removeListener(listener);
    });
  }
}

ToolbarHelper.prototype = {
  idFromOrigin: function(origin) {
    
    
    return this._type + "-" + Services.io.newURI(origin, null, null).hostPort.replace(/[\.:]/g,'-');
  },

  
  removeProviderButton: function(origin) {
    CustomizableUI.destroyWidget(this.idFromOrigin(origin));
  },

  clearPalette: function() {
    [this.removeProviderButton(p.origin) for (p of Social.providers)];
  },

  
  populatePalette: function() {
    if (!Social.enabled) {
      this.clearPalette();
      return;
    }

    
    
    for (let provider of Social.providers) {
      let id = this.idFromOrigin(provider.origin);
      this._createButton(id, provider);
    }
  }
}

let SocialStatusWidgetListener = {
  _getNodeOrigin: function(aWidgetId) {
    
    let node = document.getElementById(aWidgetId);
    if (!node)
      return null
    if (!node.classList.contains("social-status-button"))
      return null
    return node.getAttribute("origin");
  },
  onWidgetAdded: function(aWidgetId, aArea, aPosition) {
    let origin = this._getNodeOrigin(aWidgetId);
    if (origin)
      SocialStatus.updateButton(origin);
  },
  onWidgetRemoved: function(aWidgetId, aPrevArea) {
    let origin = this._getNodeOrigin(aWidgetId);
    if (!origin)
      return;
    
    
    SocialStatus.updateButton(origin);
    SocialStatus._removeFrame(origin);
  }
}

SocialStatus = {
  populateToolbarPalette: function() {
    this._toolbarHelper.populatePalette();

    for (let provider of Social.providers)
      this.updateButton(provider.origin);
  },

  removeProvider: function(origin) {
    this._removeFrame(origin);
    this._toolbarHelper.removeProviderButton(origin);
  },

  reloadProvider: function(origin) {
    let button = document.getElementById(this._toolbarHelper.idFromOrigin(origin));
    if (button && button.getAttribute("open") == "true")
      document.getElementById("social-notification-panel").hidePopup();
    this._removeFrame(origin);
  },

  _removeFrame: function(origin) {
    let notificationFrameId = "social-status-" + origin;
    let frame = document.getElementById(notificationFrameId);
    if (frame) {
      frame.parentNode.removeChild(frame);
    }
  },

  get _toolbarHelper() {
    delete this._toolbarHelper;
    this._toolbarHelper = new ToolbarHelper("social-status-button",
                                            CreateSocialStatusWidget,
                                            SocialStatusWidgetListener);
    return this._toolbarHelper;
  },

  updateButton: function(origin) {
    let id = this._toolbarHelper.idFromOrigin(origin);
    let widget = CustomizableUI.getWidget(id);
    if (!widget)
      return;
    let button = widget.forWindow(window).node;
    if (button) {
      
      let provider = Social._getProviderFromOrigin(origin);
      let icons = provider.ambientNotificationIcons;
      let iconNames = Object.keys(icons);
      let notif = icons[iconNames[0]];

      
      
      let iconURL = provider.icon32URL || provider.iconURL;
      let tooltiptext;
      if (!notif || !widget.areaType) {
        button.style.listStyleImage = "url(" + iconURL + ")";
        button.setAttribute("badge", "");
        button.setAttribute("aria-label", "");
        button.setAttribute("tooltiptext", provider.name);
        return;
      }
      button.style.listStyleImage = "url(" + (notif.iconURL || iconURL) + ")";
      button.setAttribute("tooltiptext", notif.label || provider.name);

      let badge = notif.counter || "";
      button.setAttribute("badge", badge);
      let ariaLabel = notif.label;
      
      if (badge)
        ariaLabel = gNavigatorBundle.getFormattedString("social.aria.toolbarButtonBadgeText",
                                                        [ariaLabel, badge]);
      button.setAttribute("aria-label", ariaLabel);
    }
  },

  _onclose: function(frame) {
    frame.removeEventListener("close", this._onclose, true);
    frame.removeEventListener("click", this._onclick, true);
    if (frame.socialErrorListener)
      frame.socialErrorListener.remove();
  },

  _onclick: function() {
    Services.telemetry.getHistogramById("SOCIAL_PANEL_CLICKS").add(1);
  },

  showPopup: function(aToolbarButton) {
    
    let origin = aToolbarButton.getAttribute("origin");
    let provider = Social._getProviderFromOrigin(origin);

    PanelFrame.showPopup(window, aToolbarButton, "social", origin,
                         provider.statusURL, provider.getPageSize("status"),
                         (frame) => {
                          frame.addEventListener("close", () => { SocialStatus._onclose(frame) }, true);
                          frame.addEventListener("click", this._onclick, true);
                          Social.setErrorListener(frame, this.setPanelErrorMessage.bind(this));
                        });
    Services.telemetry.getHistogramById("SOCIAL_TOOLBAR_BUTTONS").add(1);
  },

  setPanelErrorMessage: function(aNotificationFrame) {
    if (!aNotificationFrame)
      return;

    let src = aNotificationFrame.getAttribute("src");
    aNotificationFrame.removeAttribute("src");
    let origin = aNotificationFrame.getAttribute("origin");
    aNotificationFrame.webNavigation.loadURI("about:socialerror?mode=tryAgainOnly&url=" +
                                            encodeURIComponent(src) + "&origin=" +
                                            encodeURIComponent(origin),
                                            null, null, null, null);
    let panel = aNotificationFrame.parentNode;
    sizeSocialPanelToContent(panel, aNotificationFrame);
  },

};







SocialMarks = {
  get nodes() {
    let providers = [p for (p of Social.providers) if (p.markURL)];
    for (let p of providers) {
      let widgetId = SocialMarks._toolbarHelper.idFromOrigin(p.origin);
      let widget = CustomizableUI.getWidget(widgetId);
      if (!widget)
        continue;
      let node = widget.forWindow(window).node;
      if (node)
        yield node;
    }
  },
  update: function() {
    
    
    for (let node of this.nodes) {
      
      
      if (node.update) {
        node.update();
      }
    }
  },

  getProviders: function() {
    
    
    
    
    let tbh = this._toolbarHelper;
    return [p for (p of Social.providers) if (p.markURL &&
                                              document.getElementById(tbh.idFromOrigin(p.origin)))];
  },

  populateContextMenu: function() {
    
    let providers = this.getProviders();

    
    let menus = [m for (m of document.getElementsByClassName("context-socialmarks"))];
    [m.parentNode.removeChild(m) for (m of menus)];

    let contextMenus = [
      {
        type: "link",
        id: "context-marklinkMenu",
        label: "social.marklinkMenu.label"
      },
      {
        type: "page",
        id: "context-markpageMenu",
        label: "social.markpageMenu.label"
      }
    ];
    for (let cfg of contextMenus) {
      this._populateContextPopup(cfg, providers);
    }
    this.update();
  },

  MENU_LIMIT: 3, 
  _populateContextPopup: function(menuInfo, providers) {
    let menu = document.getElementById(menuInfo.id);
    let popup = menu.firstChild;
    for (let provider of providers) {
      
      
      
      let mi = document.createElement("menuitem");
      mi.setAttribute("oncommand", "gContextMenu.markLink(this.getAttribute('origin'));");
      mi.setAttribute("origin", provider.origin);
      mi.setAttribute("image", provider.iconURL);
      if (providers.length <= this.MENU_LIMIT) {
        
        mi.setAttribute("class", "menuitem-iconic context-socialmarks context-mark"+menuInfo.type);
        let menuLabel = gNavigatorBundle.getFormattedString(menuInfo.label, [provider.name]);
        mi.setAttribute("label", menuLabel);
        menu.parentNode.insertBefore(mi, menu);
      } else {
        mi.setAttribute("class", "menuitem-iconic context-socialmarks");
        mi.setAttribute("label", provider.name);
        popup.appendChild(mi);
      }
    }
  },

  populateToolbarPalette: function() {
    this._toolbarHelper.populatePalette();
    this.populateContextMenu();
  },

  removeProvider: function(origin) {
    this._toolbarHelper.removeProviderButton(origin);
  },

  get _toolbarHelper() {
    delete this._toolbarHelper;
    this._toolbarHelper = new ToolbarHelper("social-mark-button", CreateSocialMarkWidget);
    return this._toolbarHelper;
  },

  markLink: function(aOrigin, aUrl, aTarget) {
    
    let id = this._toolbarHelper.idFromOrigin(aOrigin);
    document.getElementById(id).markLink(aUrl, aTarget);
  }
};

})();
