




let SocialUI,
    SocialChatBar,
    SocialFlyout,
    SocialMarks,
    SocialShare,
    SocialMenu,
    SocialToolbar,
    SocialSidebar,
    SocialStatus;

(function() {


const PANEL_MIN_HEIGHT = 100;
const PANEL_MIN_WIDTH = 330;

XPCOMUtils.defineLazyModuleGetter(this, "SharedFrame",
  "resource:///modules/SharedFrame.jsm");

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

SocialUI = {
  
  init: function SocialUI_init() {
    Services.obs.addObserver(this, "social:ambient-notification-changed", false);
    Services.obs.addObserver(this, "social:profile-changed", false);
    Services.obs.addObserver(this, "social:frameworker-error", false);
    Services.obs.addObserver(this, "social:provider-set", false);
    Services.obs.addObserver(this, "social:providers-changed", false);
    Services.obs.addObserver(this, "social:provider-reload", false);
    Services.obs.addObserver(this, "social:provider-installed", false);
    Services.obs.addObserver(this, "social:provider-uninstalled", false);
    Services.obs.addObserver(this, "social:provider-enabled", false);
    Services.obs.addObserver(this, "social:provider-disabled", false);

    Services.prefs.addObserver("social.sidebar.open", this, false);
    Services.prefs.addObserver("social.toast-notifications.enabled", this, false);

    gBrowser.addEventListener("ActivateSocialFeature", this._activationEventHandler.bind(this), true, true);
    window.addEventListener("aftercustomization", function() {
      if (SocialUI.enabled)
        SocialMarks.populateContextMenu(SocialMarks);
    }, false);

    if (!Social.initialized) {
      Social.init();
    } else if (Social.providers.length > 0) {
      
      
      this.observe(null, "social:providers-changed", null);
      this.observe(null, "social:provider-set", Social.provider ? Social.provider.origin : null);
    }
  },

  
  uninit: function SocialUI_uninit() {
    Services.obs.removeObserver(this, "social:ambient-notification-changed");
    Services.obs.removeObserver(this, "social:profile-changed");
    Services.obs.removeObserver(this, "social:frameworker-error");
    Services.obs.removeObserver(this, "social:provider-set");
    Services.obs.removeObserver(this, "social:providers-changed");
    Services.obs.removeObserver(this, "social:provider-reload");
    Services.obs.removeObserver(this, "social:provider-installed");
    Services.obs.removeObserver(this, "social:provider-uninstalled");
    Services.obs.removeObserver(this, "social:provider-enabled");
    Services.obs.removeObserver(this, "social:provider-disabled");

    Services.prefs.removeObserver("social.sidebar.open", this);
    Services.prefs.removeObserver("social.toast-notifications.enabled", this);
  },

  _matchesCurrentProvider: function (origin) {
    return Social.provider && Social.provider.origin == origin;
  },

  observe: function SocialUI_observe(subject, topic, data) {
    
    
    try {
      switch (topic) {
        case "social:provider-installed":
          SocialMarks.setPosition(data);
          SocialStatus.setPosition(data);
          break;
        case "social:provider-uninstalled":
          SocialMarks.removePosition(data);
          SocialStatus.removePosition(data);
          break;
        case "social:provider-enabled":
          SocialMarks.populateToolbarPalette();
          SocialStatus.populateToolbarPalette();
          break;
        case "social:provider-disabled":
          SocialMarks.removeProvider(data);
          SocialStatus.removeProvider(data);
          break;
        case "social:provider-reload":
          
          
          if (!Social.provider || Social.provider.origin != data)
            return;
          
          
          
          SocialSidebar.unloadSidebar();
          
        case "social:provider-set":
          
          
          this._updateActiveUI();
          this._updateMenuItems();

          SocialFlyout.unload();
          SocialChatBar.update();
          SocialShare.update();
          SocialSidebar.update();
          SocialToolbar.update();
          SocialStatus.populateToolbarPalette();
          SocialMarks.populateToolbarPalette();
          SocialMenu.populate();
          break;
        case "social:providers-changed":
          
          this._updateActiveUI();
          
          SocialToolbar.populateProviderMenus();
          SocialShare.populateProviderMenu();
          SocialStatus.populateToolbarPalette();
          SocialMarks.populateToolbarPalette();
          break;

        
        case "social:ambient-notification-changed":
          SocialStatus.updateNotification(data);
          if (this._matchesCurrentProvider(data)) {
            SocialToolbar.updateButton();
            SocialMenu.populate();
          }
          break;
        case "social:profile-changed":
          if (this._matchesCurrentProvider(data)) {
            SocialToolbar.updateProvider();
            SocialMarks.update();
            SocialChatBar.update();
          }
          break;
        case "social:frameworker-error":
          if (this.enabled && Social.provider.origin == data) {
            SocialSidebar.setSidebarErrorMessage();
          }
          break;

        case "nsPref:changed":
          if (data == "social.sidebar.open") {
            SocialSidebar.update();
          } else if (data == "social.toast-notifications.enabled") {
            SocialToolbar.updateButton();
          }
          break;
      }
    } catch (e) {
      Components.utils.reportError(e + "\n" + e.stack);
      throw e;
    }
  },

  nonBrowserWindowInit: function SocialUI_nonBrowserInit() {
    
    document.getElementById("menu_socialAmbientMenu").hidden = true;
  },

  
  showProfile: function SocialUI_showProfile() {
    if (Social.provider.haveLoggedInUser())
      openUILinkIn(Social.provider.profile.profileURL, "tab");
    else {
      
      openUILinkIn(Social.provider.origin, "tab");
    }
  },

  _updateActiveUI: function SocialUI_updateActiveUI() {
    
    
    let enabled = Social.providers.length > 0 && !this._chromeless &&
                  !PrivateBrowsingUtils.isWindowPrivate(window);
    let broadcaster = document.getElementById("socialActiveBroadcaster");
    broadcaster.hidden = !enabled;

    let toggleCommand = document.getElementById("Social:Toggle");
    toggleCommand.setAttribute("hidden", enabled ? "false" : "true");

    if (enabled) {
      
      let provider = Social.provider || Social.defaultProvider;
      
      let label;
      if (Social.providers.length == 1) {
        label = gNavigatorBundle.getFormattedString(Social.provider
                                                    ? "social.turnOff.label"
                                                    : "social.turnOn.label",
                                                    [provider.name]);
      } else {
        label = gNavigatorBundle.getString(Social.provider
                                           ? "social.turnOffAll.label"
                                           : "social.turnOnAll.label");
      }
      let accesskey = gNavigatorBundle.getString(Social.provider
                                                 ? "social.turnOff.accesskey"
                                                 : "social.turnOn.accesskey");
      toggleCommand.setAttribute("label", label);
      toggleCommand.setAttribute("accesskey", accesskey);
    }
  },

  _updateMenuItems: function () {
    let provider = Social.provider || Social.defaultProvider;
    if (!provider)
      return;
    
    for (let id of ["menu_socialSidebar", "menu_socialAmbientMenu"])
      document.getElementById(id).setAttribute("label", provider.name);
  },

  
  
  _activationEventHandler: function SocialUI_activationHandler(e) {
    let targetDoc;
    let node;
    if (e.target instanceof HTMLDocument) {
      
      targetDoc = e.target;
      node = targetDoc.documentElement
    } else {
      targetDoc = e.target.ownerDocument;
      node = e.target;
    }
    if (!(targetDoc instanceof HTMLDocument))
      return;

    
    if (targetDoc.defaultView != content)
      return;

    
    
    if (PrivateBrowsingUtils.isWindowPrivate(window))
      return;

    
    let now = Date.now();
    if (now - Social.lastEventReceived < 1000)
      return;
    Social.lastEventReceived = now;

    
    let dwu = window.QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIDOMWindowUtils);
    if (!dwu.isHandlingUserInput) {
      Cu.reportError("attempt to activate provider without user input from " + targetDoc.nodePrincipal.origin);
      return;
    }

    let data = node.getAttribute("data-service");
    if (data) {
      try {
        data = JSON.parse(data);
      } catch(e) {
        Cu.reportError("Social Service manifest parse error: "+e);
        return;
      }
    }
    Social.installProvider(targetDoc, data, function(manifest) {
      this.doActivation(manifest.origin);
    }.bind(this));
  },

  doActivation: function SocialUI_doActivation(origin) {
    
    let oldOrigin = Social.provider ? Social.provider.origin : "";

    
    Social.activateFromOrigin(origin, function(provider) {
      
      if (!provider)
        return;

      
      let description = document.getElementById("social-activation-message");
      let labels = description.getElementsByTagName("label");
      let uri = Services.io.newURI(provider.origin, null, null)
      labels[0].setAttribute("value", uri.host);
      labels[1].setAttribute("onclick", "BrowserOpenAddonsMgr('addons://list/service'); SocialUI.activationPanel.hidePopup();")

      let icon = document.getElementById("social-activation-icon");
      if (provider.icon64URL || provider.icon32URL) {
        icon.setAttribute('src', provider.icon64URL || provider.icon32URL);
        icon.hidden = false;
      } else {
        icon.removeAttribute('src');
        icon.hidden = true;
      }

      let notificationPanel = SocialUI.activationPanel;
      
      notificationPanel.setAttribute("origin", provider.origin);
      notificationPanel.setAttribute("oldorigin", oldOrigin);

      
      notificationPanel.hidden = false;
      setTimeout(function () {
        notificationPanel.openPopup(SocialToolbar.button, "bottomcenter topright");
      }, 0);
    });
  },

  undoActivation: function SocialUI_undoActivation() {
    let origin = this.activationPanel.getAttribute("origin");
    let oldOrigin = this.activationPanel.getAttribute("oldorigin");
    Social.deactivateFromOrigin(origin, oldOrigin);
    this.activationPanel.hidePopup();
    Social.uninstallProvider(origin);
  },

  showLearnMore: function() {
    this.activationPanel.hidePopup();
    let url = Services.urlFormatter.formatURLPref("app.support.baseURL") + "social-api";
    openUILinkIn(url, "tab");
  },

  get activationPanel() {
    return document.getElementById("socialActivatedNotification");
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
    
    
    let chromeless = docElem.getAttribute("chromehidden").contains("extrachrome") ||
                     docElem.getAttribute('chromehidden').contains("toolbar");
    
    
    delete this._chromeless;
    this._chromeless = chromeless;
    return chromeless;
  },

  get enabled() {
    
    if (this._chromeless || PrivateBrowsingUtils.isWindowPrivate(window))
      return false;
    return !!Social.provider;
  },

  
  
  updateState: function() {
    if (!this.enabled)
      return;
    SocialMarks.update();
    SocialShare.update();
  }
}

SocialChatBar = {
  get chatbar() {
    return document.getElementById("pinnedchats");
  },
  
  
  get isAvailable() {
    return SocialUI.enabled;
  },
  
  get hasChats() {
    return !!this.chatbar.firstElementChild;
  },
  openChat: function(aProvider, aURL, aCallback, aMode) {
    if (!this.isAvailable)
      return false;
    this.chatbar.openChat(aProvider, aURL, aCallback, aMode);
    
    let dwu = window.QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIDOMWindowUtils);
    if (dwu.isHandlingUserInput)
      this.chatbar.focus();
    return true;
  },
  update: function() {
    let command = document.getElementById("Social:FocusChat");
    if (!this.isAvailable) {
      this.chatbar.hidden = command.hidden = true;
    } else {
      this.chatbar.hidden = command.hidden = false;
    }
    command.setAttribute("disabled", command.hidden ? "true" : "false");
  },
  focus: function SocialChatBar_focus() {
    this.chatbar.focus();
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
    iframe.setAttribute("origin", Social.provider.origin);
    panel.appendChild(iframe);
  },

  setFlyoutErrorMessage: function SF_setFlyoutErrorMessage() {
    this.iframe.removeAttribute("src");
    this.iframe.webNavigation.loadURI("about:socialerror?mode=compactInfo", null, null, null, null);
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
    if (!Social.provider)
      return;

    this.panel.hidden = false;
    let iframe = this.iframe;
    
    
    let src = iframe.contentDocument && iframe.contentDocument.documentURIObject;
    if (!src || !src.equalsExceptRef(Services.io.newURI(aURL, null, null))) {
      iframe.addEventListener("load", function documentLoaded() {
        iframe.removeEventListener("load", documentLoaded, true);
        cb();
      }, true);
      
      
      iframe.clientTop;
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
  get panel() {
    return document.getElementById("social-share-panel");
  },

  get iframe() {
    
    if (this.panel.childElementCount == 1)
      return null;
    else
      return this.panel.lastChild;
  },

  _createFrame: function() {
    let panel = this.panel;
    if (!SocialUI.enabled || this.iframe)
      return;
    this.panel.hidden = false;
    
    let iframe = document.createElement("iframe");
    iframe.setAttribute("type", "content");
    iframe.setAttribute("class", "social-share-frame");
    iframe.setAttribute("context", "contentAreaContextMenu");
    iframe.setAttribute("tooltip", "aHTMLTooltip");
    iframe.setAttribute("flex", "1");
    panel.appendChild(iframe);
    this.populateProviderMenu();
  },

  getSelectedProvider: function() {
    let provider;
    let lastProviderOrigin = this.iframe && this.iframe.getAttribute("origin");
    if (lastProviderOrigin) {
      provider = Social._getProviderFromOrigin(lastProviderOrigin);
    }
    if (!provider)
      provider = Social.provider || Social.defaultProvider;
    
    if (provider && !provider.shareURL) {
      let providers = [p for (p of Social.providers) if (p.shareURL)];
      provider = providers.length > 0  && providers[0];
    }
    return provider;
  },

  populateProviderMenu: function() {
    if (!this.iframe)
      return;
    let providers = [p for (p of Social.providers) if (p.shareURL)];
    let hbox = document.getElementById("social-share-provider-buttons");
    
    
    while (hbox.firstChild) {
      hbox.removeChild(hbox.firstChild);
    }
    
    
    if (!SocialUI.enabled || providers.length < 2) {
      this.panel.firstChild.hidden = true;
      return;
    }
    let selectedProvider = this.getSelectedProvider();
    for (let provider of providers) {
      let button = document.createElement("toolbarbutton");
      button.setAttribute("class", "toolbarbutton share-provider-button");
      button.setAttribute("type", "radio");
      button.setAttribute("group", "share-providers");
      button.setAttribute("image", provider.iconURL);
      button.setAttribute("tooltiptext", provider.name);
      button.setAttribute("origin", provider.origin);
      button.setAttribute("oncommand", "SocialShare.sharePage(this.getAttribute('origin')); this.checked=true;");
      if (provider == selectedProvider) {
        this.defaultButton = button;
      }
      hbox.appendChild(button);
    }
    if (!this.defaultButton) {
      this.defaultButton = hbox.firstChild
    }
    this.defaultButton.setAttribute("checked", "true");
    this.panel.firstChild.hidden = false;
  },

  get shareButton() {
    return document.getElementById("social-share-button");
  },

  canSharePage: function(aURI) {
    
    if (PrivateBrowsingUtils.isWindowPrivate(window))
      return false;

    if (!aURI || !(aURI.schemeIs('http') || aURI.schemeIs('https')))
      return false;
    return true;
  },

  update: function() {
    let shareButton = this.shareButton;
    shareButton.hidden = !SocialUI.enabled ||
                         [p for (p of Social.providers) if (p.shareURL)].length == 0;
    shareButton.disabled = shareButton.hidden || !this.canSharePage(gBrowser.currentURI);

    
    
    let cmd = document.getElementById("Social:SharePage");
    cmd.setAttribute("disabled", shareButton.disabled ? "true" : "false");
  },

  onShowing: function() {
    this.shareButton.setAttribute("open", "true");
  },

  onHidden: function() {
    this.shareButton.removeAttribute("open");
    this.iframe.setAttribute("src", "data:text/plain;charset=utf8,");
    this.currentShare = null;
  },

  setErrorMessage: function() {
    let iframe = this.iframe;
    if (!iframe)
      return;

    iframe.removeAttribute("src");
    iframe.webNavigation.loadURI("about:socialerror?mode=compactInfo&origin=" +
                                 encodeURIComponent(iframe.getAttribute("origin")),
                                 null, null, null, null);
    sizeSocialPanelToContent(this.panel, iframe);
  },

  sharePage: function(providerOrigin, graphData) {
    
    
    
    this._createFrame();
    let iframe = this.iframe;
    let provider;
    if (providerOrigin)
      provider = Social._getProviderFromOrigin(providerOrigin);
    else
      provider = this.getSelectedProvider();
    if (!provider || !provider.shareURL)
      return;

    
    
    
    
    
    let sharedURI = graphData ? Services.io.newURI(graphData.url, null, null) :
                                gBrowser.currentURI;
    if (!this.canSharePage(sharedURI))
      return;

    
    
    
    
    let pageData = graphData ? graphData : this.currentShare;
    if (!pageData || sharedURI == gBrowser.currentURI) {
      pageData = OpenGraphBuilder.getData(gBrowser);
      if (graphData) {
        
        for (let p in graphData) {
          pageData[p] = graphData[p];
        }
      }
    }
    this.currentShare = pageData;

    let shareEndpoint = OpenGraphBuilder.generateEndpointURL(provider.shareURL, pageData);

    this._dynamicResizer = new DynamicResizeWatcher();
    
    
    let reload = true;
    let endpointMatch = shareEndpoint == iframe.getAttribute("src");
    let docLoaded = iframe.contentDocument && iframe.contentDocument.readyState == "complete";
    if (endpointMatch && docLoaded) {
      reload = shareEndpoint != iframe.contentDocument.location.spec;
    }
    if (!reload) {
      this._dynamicResizer.start(this.panel, iframe);
      iframe.docShell.isActive = true;
      iframe.docShell.isAppTab = true;
      let evt = iframe.contentDocument.createEvent("CustomEvent");
      evt.initCustomEvent("OpenGraphData", true, true, JSON.stringify(pageData));
      iframe.contentDocument.documentElement.dispatchEvent(evt);
    } else {
      
      iframe.addEventListener("load", function panelBrowserOnload(e) {
        iframe.removeEventListener("load", panelBrowserOnload, true);
        iframe.docShell.isActive = true;
        iframe.docShell.isAppTab = true;
        setTimeout(function() {
          if (SocialShare._dynamicResizer) { 
            SocialShare._dynamicResizer.start(iframe.parentNode, iframe);
          }
        }, 0);
        let evt = iframe.contentDocument.createEvent("CustomEvent");
        evt.initCustomEvent("OpenGraphData", true, true, JSON.stringify(pageData));
        iframe.contentDocument.documentElement.dispatchEvent(evt);
      }, true);
    }
    
    let uri = Services.io.newURI(shareEndpoint, null, null);
    iframe.setAttribute("origin", provider.origin);
    iframe.setAttribute("src", shareEndpoint);

    let navBar = document.getElementById("nav-bar");
    let anchor = navBar.getAttribute("mode") == "text" ?
                   document.getAnonymousElementByAttribute(this.shareButton, "class", "toolbarbutton-text") :
                   document.getAnonymousElementByAttribute(this.shareButton, "class", "toolbarbutton-icon");
    this.panel.openPopup(anchor, "bottomcenter topright", 0, 0, false, false);
    Social.setErrorListener(iframe, this.setErrorMessage.bind(this));
  }
};

SocialMenu = {
  populate: function SocialMenu_populate() {
    let submenu = document.getElementById("menu_social-statusarea-popup");
    let ambientMenuItems = submenu.getElementsByClassName("ambient-menuitem");
    while (ambientMenuItems.length)
      submenu.removeChild(ambientMenuItems.item(0));

    let separator = document.getElementById("socialAmbientMenuSeparator");
    separator.hidden = true;
    let provider = SocialUI.enabled ? Social.provider : null;
    if (!provider)
      return;

    let iconNames = Object.keys(provider.ambientNotificationIcons);
    for (let name of iconNames) {
      let icon = provider.ambientNotificationIcons[name];
      if (!icon.label || !icon.menuURL)
        continue;
      separator.hidden = false;
      let menuitem = document.createElement("menuitem");
      menuitem.setAttribute("label", icon.label);
      menuitem.classList.add("ambient-menuitem");
      menuitem.addEventListener("command", function() {
        openUILinkIn(icon.menuURL, "tab");
      }, false);
      submenu.insertBefore(menuitem, separator);
    }
  }
};


SocialToolbar = {
  
  
  get _dynamicResizer() {
    delete this._dynamicResizer;
    this._dynamicResizer = new DynamicResizeWatcher();
    return this._dynamicResizer;
  },

  update: function() {
    this._updateButtonHiddenState();
    this.updateProvider();
    this.populateProviderMenus();
  },

  
  updateProvider: function () {
    let provider = Social.provider;
    if (provider) {
      this.button.setAttribute("label", provider.name);
      this.button.setAttribute("tooltiptext", provider.name);
      this.button.style.listStyleImage = "url(" + provider.iconURL + ")";

      this.updateProfile();
    } else {
      this.button.setAttribute("label", gNavigatorBundle.getString("service.toolbarbutton.label"));
      this.button.setAttribute("tooltiptext", gNavigatorBundle.getString("service.toolbarbutton.tooltiptext"));
      this.button.style.removeProperty("list-style-image");
    }
    this.updateButton();
  },

  get button() {
    return document.getElementById("social-provider-button");
  },

  
  
  _updateButtonHiddenState: function SocialToolbar_updateButtonHiddenState() {
    let socialEnabled = SocialUI.enabled;
    for (let className of ["social-statusarea-separator", "social-statusarea-user"]) {
      for (let element of document.getElementsByClassName(className))
        element.hidden = !socialEnabled;
    }
    let toggleNotificationsCommand = document.getElementById("Social:ToggleNotifications");
    toggleNotificationsCommand.setAttribute("hidden", !socialEnabled);

    
    
    
    
    let tbi = document.getElementById("social-provider-button");
    if (tbi) {
      
      let next = tbi.nextSibling;
      let currentOrigin = Social.provider ? Social.provider.origin : null;

      while (next) {
        let button = next;
        next = next.nextSibling;
        
        let frameId = button.getAttribute("notificationFrameId");
        let frame = document.getElementById(frameId);
        if (!socialEnabled || frame.getAttribute("origin") != currentOrigin) {
          SharedFrame.forgetGroup(frame.id);
          frame.parentNode.removeChild(frame);
          button.parentNode.removeChild(button);
        }
      }
    }
  },

  updateProfile: function SocialToolbar_updateProfile() {
    
    
    
    if (!Social.provider)
      return;
    let profile = Social.provider.profile || {};
    let userPortrait = profile.portrait;

    let userDetailsBroadcaster = document.getElementById("socialBroadcaster_userDetails");
    let loggedInStatusValue = profile.userName ||
                              userDetailsBroadcaster.getAttribute("notLoggedInLabel");

    
    
    if (userPortrait) {
      userDetailsBroadcaster.setAttribute("src", userPortrait);
      userDetailsBroadcaster.setAttribute("image", userPortrait);
    } else {
      userDetailsBroadcaster.removeAttribute("src");
      userDetailsBroadcaster.removeAttribute("image");
    }

    userDetailsBroadcaster.setAttribute("value", loggedInStatusValue);
    userDetailsBroadcaster.setAttribute("label", loggedInStatusValue);
  },

  updateButton: function SocialToolbar_updateButton() {
    this._updateButtonHiddenState();
    let panel = document.getElementById("social-notification-panel");
    panel.hidden = !SocialUI.enabled;

    let command = document.getElementById("Social:ToggleNotifications");
    command.setAttribute("checked", Services.prefs.getBoolPref("social.toast-notifications.enabled"));

    const CACHE_PREF_NAME = "social.cached.ambientNotificationIcons";
    
    
    if (!SocialUI.enabled ||
        (!Social.provider.haveLoggedInUser() && Social.provider.profile !== undefined)) {
      
      
      
      
      Services.prefs.clearUserPref(CACHE_PREF_NAME);
      return;
    }

    
    
    
    
    
    if (Social.provider.statusURL && Social.allowMultipleWorkers)
      return;

    let icons = Social.provider.ambientNotificationIcons;
    let iconNames = Object.keys(icons);

    if (Social.provider.profile === undefined) {
      
      
      let cached;
      try {
        cached = JSON.parse(Services.prefs.getComplexValue(CACHE_PREF_NAME,
                                                           Ci.nsISupportsString).data);
      } catch (ex) {}
      if (cached && cached.provider == Social.provider.origin && cached.data) {
        icons = cached.data;
        iconNames = Object.keys(icons);
        
        for each(let name in iconNames) {
          icons[name].counter = '';
        }
      }
    } else {
      
      
      let str = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
      str.data = JSON.stringify({provider: Social.provider.origin, data: icons});
      Services.prefs.setComplexValue(CACHE_PREF_NAME,
                                     Ci.nsISupportsString,
                                     str);
    }

    let toolbarButtons = document.createDocumentFragment();

    let createdFrames = [];

    for each(let name in iconNames) {
      let icon = icons[name];

      let notificationFrameId = "social-status-" + icon.name;
      let notificationFrame = document.getElementById(notificationFrameId);

      if (!notificationFrame) {
        notificationFrame = SharedFrame.createFrame(
          notificationFrameId, 
          panel, 
          {
            "type": "content",
            "mozbrowser": "true",
            "class": "social-panel-frame",
            "id": notificationFrameId,
            "tooltip": "aHTMLTooltip",

            
            
            "style": "width: " + PANEL_MIN_WIDTH + "px;",

            "origin": Social.provider.origin,
            "src": icon.contentPanel
          }
        );

        createdFrames.push(notificationFrame);
      } else {
        notificationFrame.setAttribute("origin", Social.provider.origin);
        SharedFrame.updateURL(notificationFrameId, icon.contentPanel);
      }

      let toolbarButtonId = "social-notification-icon-" + icon.name;
      let toolbarButton = document.getElementById(toolbarButtonId);
      if (!toolbarButton) {
        toolbarButton = document.createElement("toolbarbutton");
        toolbarButton.setAttribute("type", "badged");
        toolbarButton.classList.add("toolbarbutton-1");
        toolbarButton.setAttribute("id", toolbarButtonId);
        toolbarButton.setAttribute("notificationFrameId", notificationFrameId);
        toolbarButton.addEventListener("mousedown", function (event) {
          if (event.button == 0 && panel.state == "closed")
            SocialToolbar.showAmbientPopup(toolbarButton);
        });

        toolbarButtons.appendChild(toolbarButton);
      }

      toolbarButton.style.listStyleImage = "url(" + icon.iconURL + ")";
      toolbarButton.setAttribute("label", icon.label);
      toolbarButton.setAttribute("tooltiptext", icon.label);

      let badge = icon.counter || "";
      toolbarButton.setAttribute("badge", badge);
      let ariaLabel = icon.label;
      
      if (badge)
        ariaLabel = gNavigatorBundle.getFormattedString("social.aria.toolbarButtonBadgeText",
                                                        [ariaLabel, badge]);
      toolbarButton.setAttribute("aria-label", ariaLabel);
    }
    let socialToolbarItem = document.getElementById("social-toolbar-item");
    socialToolbarItem.appendChild(toolbarButtons);

    for (let frame of createdFrames) {
      if (frame.socialErrorListener)
        frame.socialErrorListener.remove();
      if (frame.docShell) {
        frame.docShell.isActive = false;
        Social.setErrorListener(frame, this.setPanelErrorMessage.bind(this));
      }
    }
  },

  showAmbientPopup: function SocialToolbar_showAmbientPopup(aToolbarButton) {
    
    SocialFlyout.panel.hidePopup();

    let panel = document.getElementById("social-notification-panel");
    let notificationFrameId = aToolbarButton.getAttribute("notificationFrameId");
    let notificationFrame = document.getElementById(notificationFrameId);

    let wasAlive = SharedFrame.isGroupAlive(notificationFrameId);
    SharedFrame.setOwner(notificationFrameId, notificationFrame);

    
    
    let frameIter = panel.firstElementChild;
    while (frameIter) {
      frameIter.collapsed = (frameIter != notificationFrame);
      frameIter = frameIter.nextElementSibling;
    }

    function dispatchPanelEvent(name) {
      let evt = notificationFrame.contentDocument.createEvent("CustomEvent");
      evt.initCustomEvent(name, true, true, {});
      notificationFrame.contentDocument.documentElement.dispatchEvent(evt);
    }

    let dynamicResizer = this._dynamicResizer;
    panel.addEventListener("popuphidden", function onpopuphiding() {
      panel.removeEventListener("popuphidden", onpopuphiding);
      aToolbarButton.removeAttribute("open");
      aToolbarButton.parentNode.removeAttribute("open");
      dynamicResizer.stop();
      notificationFrame.docShell.isActive = false;
      dispatchPanelEvent("socialFrameHide");
    });

    panel.addEventListener("popupshown", function onpopupshown() {
      panel.removeEventListener("popupshown", onpopupshown);
      
      
      
      
      aToolbarButton.setAttribute("open", "true");
      aToolbarButton.parentNode.setAttribute("open", "true");
      notificationFrame.docShell.isActive = true;
      notificationFrame.docShell.isAppTab = true;
      if (notificationFrame.contentDocument.readyState == "complete" && wasAlive) {
        dynamicResizer.start(panel, notificationFrame);
        dispatchPanelEvent("socialFrameShow");
      } else {
        
        notificationFrame.addEventListener("load", function panelBrowserOnload(e) {
          notificationFrame.removeEventListener("load", panelBrowserOnload, true);
          dynamicResizer.start(panel, notificationFrame);
          setTimeout(function() {
            dispatchPanelEvent("socialFrameShow");
          }, 0);
        }, true);
      }
    });

    let navBar = document.getElementById("nav-bar");
    let anchor = navBar.getAttribute("mode") == "text" ?
                   document.getAnonymousElementByAttribute(aToolbarButton, "class", "toolbarbutton-text") :
                   document.getAnonymousElementByAttribute(aToolbarButton, "class", "toolbarbutton-badge-container");
    
    
    setTimeout(function() {
      panel.openPopup(anchor, "bottomcenter topright", 0, 0, false, false);
    }, 0);
  },

  setPanelErrorMessage: function SocialToolbar_setPanelErrorMessage(aNotificationFrame) {
    if (!aNotificationFrame)
      return;

    let src = aNotificationFrame.getAttribute("src");
    aNotificationFrame.removeAttribute("src");
    aNotificationFrame.webNavigation.loadURI("about:socialerror?mode=tryAgainOnly&url=" +
                                             encodeURIComponent(src), null, null, null, null);
    let panel = aNotificationFrame.parentNode;
    sizeSocialPanelToContent(panel, aNotificationFrame);
  },

  populateProviderMenus: function SocialToolbar_renderProviderMenus() {
    let providerMenuSeps = document.getElementsByClassName("social-provider-menu");
    for (let providerMenuSep of providerMenuSeps)
      this._populateProviderMenu(providerMenuSep);
  },

  _populateProviderMenu: function SocialToolbar_renderProviderMenu(providerMenuSep) {
    let menu = providerMenuSep.parentNode;
    
    
    while (providerMenuSep.previousSibling.nodeName == "menuitem") {
      menu.removeChild(providerMenuSep.previousSibling);
    }
    
    let providers = [p for (p of Social.providers) if (p.workerURL || p.sidebarURL)];
    if (providers.length < 2) {
      providerMenuSep.hidden = true;
      return;
    }
    for (let provider of providers) {
      let menuitem = document.createElement("menuitem");
      menuitem.className = "menuitem-iconic social-provider-menuitem";
      menuitem.setAttribute("image", provider.iconURL);
      menuitem.setAttribute("label", provider.name);
      menuitem.setAttribute("origin", provider.origin);
      if (provider == Social.provider) {
        menuitem.setAttribute("checked", "true");
      } else {
        menuitem.setAttribute("oncommand", "Social.setProviderByOrigin(this.getAttribute('origin'));");
      }
      menu.insertBefore(menuitem, providerMenuSep);
    }
    providerMenuSep.hidden = false;
  }
}

SocialSidebar = {
  
  get canShow() {
    return SocialUI.enabled && Social.provider.sidebarURL;
  },

  
  get opened() {
    return Services.prefs.getBoolPref("social.sidebar.open") && !document.mozFullScreen;
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

  update: function SocialSidebar_update() {
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
      sbrowser.setAttribute("origin", Social.provider.origin);
      if (Social.provider.errorState == "frameworker-error") {
        SocialSidebar.setSidebarErrorMessage();
        return;
      }

      
      if (sbrowser.getAttribute("src") != Social.provider.sidebarURL) {
        Social.setErrorListener(sbrowser, this.setSidebarErrorMessage.bind(this));
        
        sbrowser.docShell.isAppTab = true;
        sbrowser.setAttribute("src", Social.provider.sidebarURL);
        PopupNotifications.locationChange(sbrowser);
      }

      
      if (sbrowser.contentDocument.readyState != "complete") {
        sbrowser.addEventListener("load", SocialSidebar._loadListener, true);
      } else {
        this.setSidebarVisibilityState(true);
      }
    }
  },

  _loadListener: function SocialSidebar_loadListener() {
    let sbrowser = document.getElementById("social-sidebar-browser");
    sbrowser.removeEventListener("load", SocialSidebar._loadListener, true);
    SocialSidebar.setSidebarVisibilityState(true);
  },

  unloadSidebar: function SocialSidebar_unloadSidebar() {
    let sbrowser = document.getElementById("social-sidebar-browser");
    if (!sbrowser.hasAttribute("origin"))
      return;

    sbrowser.stop();
    sbrowser.removeAttribute("origin");
    sbrowser.setAttribute("src", "about:blank");
    SocialFlyout.unload();
  },

  _unloadTimeoutId: 0,

  setSidebarErrorMessage: function() {
    let sbrowser = document.getElementById("social-sidebar-browser");
    
    if (Social.provider.errorState == "frameworker-error") {
      sbrowser.setAttribute("src", "about:socialerror?mode=workerFailure");
    } else {
      let url = encodeURIComponent(Social.provider.sidebarURL);
      sbrowser.loadURI("about:socialerror?mode=tryAgain&url=" + url, null, null);
    }
  }
}




















function ToolbarHelper(type, createButtonFn) {
  this._createButton = createButtonFn;
  this._type = type;
}

ToolbarHelper.prototype = {
  idFromOrgin: function(origin) {
    return this._type + "-" + origin;
  },

  
  _getExistingButton: function(id) {
    let button = document.getElementById(id);
    if (button)
      return button;
    let palette = document.getElementById("navigator-toolbox").palette;
    let paletteItem = palette.firstChild;
    while (paletteItem) {
      if (paletteItem.id == id)
        return paletteItem;
      paletteItem = paletteItem.nextSibling;
    }
    return null;
  },

  setPersistentPosition: function(id) {
    
    let toolbar = document.getElementById("nav-bar");
    
    
    let currentset = toolbar.currentSet;
    if (currentset == "__empty")
      currentset = []
    else
      currentset = currentset.split(",");
    if (currentset.indexOf(id) >= 0)
      return;
    
    
    currentset.push(id);
    toolbar.setAttribute("currentset", currentset.join(","));
    document.persist(toolbar.id, "currentset");
  },

  removeProviderButton: function(origin) {
    
    let button = this._getExistingButton(this.idFromOrgin(origin));
    if (button)
      button.parentNode.removeChild(button);
  },

  removePersistence: function(id) {
    let persisted = document.querySelectorAll("*[currentset]");
    for (let pent of persisted) {
      
      
      
      let currentset = pent.getAttribute("currentset").split(",");

      let pos = currentset.indexOf(id);
      if (pos >= 0) {
        currentset.splice(pos, 1);
        pent.setAttribute("currentset", currentset.join(","));
        document.persist(pent.id, "currentset");
        return;
      }
    }
  },

  
  
  clearPalette: function() {
    [this.removeProviderButton(p.origin) for (p of Social.providers)];
  },

  
  
  
  populatePalette: function() {
    if (!Social.enabled) {
      this.clearPalette();
      return;
    }
    let persisted = document.querySelectorAll("*[currentset]");
    let persistedById = {};
    for (let pent of persisted) {
      let pset = pent.getAttribute("currentset").split(',');
      for (let id of pset)
        persistedById[id] = pent;
    }

    
    
    for (let provider of Social.providers) {
      let id = this.idFromOrgin(provider.origin);
      if (this._getExistingButton(id))
        continue;
      let button = this._createButton(provider);
      if (button && persistedById.hasOwnProperty(id)) {
        let parent = persistedById[id];
        let pset = persistedById[id].getAttribute("currentset").split(',');
        let pi = pset.indexOf(id) + 1;
        let next = document.getElementById(pset[pi]);
        parent.insertItem(id, next, null, false);
      }
    }
  }
}

SocialStatus = {
  populateToolbarPalette: function() {
    if (!Social.allowMultipleWorkers)
      return;
    this._toolbarHelper.populatePalette();
  },

  setPosition: function(origin) {
    if (!Social.allowMultipleWorkers)
      return;
    
    
    let manifest = Social.getManifestByOrigin(origin);
    if (!manifest.statusURL)
      return;
    let tbh = this._toolbarHelper;
    tbh.setPersistentPosition(tbh.idFromOrgin(origin));
  },

  removePosition: function(origin) {
    if (!Social.allowMultipleWorkers)
      return;
    let tbh = this._toolbarHelper;
    tbh.removePersistence(tbh.idFromOrgin(origin));
  },

  removeProvider: function(origin) {
    if (!Social.allowMultipleWorkers)
      return;
    this._toolbarHelper.removeProviderButton(origin);
  },

  get _toolbarHelper() {
    delete this._toolbarHelper;
    this._toolbarHelper = new ToolbarHelper("social-status-button", this._createButton.bind(this));
    return this._toolbarHelper;
  },

  get _dynamicResizer() {
    delete this._dynamicResizer;
    this._dynamicResizer = new DynamicResizeWatcher();
    return this._dynamicResizer;
  },

  _createButton: function(provider) {
    if (!provider.statusURL)
      return null;
    let palette = document.getElementById("navigator-toolbox").palette;
    let button = document.createElement("toolbarbutton");
    button.setAttribute("class", "toolbarbutton-1 social-status-button");
    button.setAttribute("type", "badged");
    button.setAttribute("removable", "true");
    button.setAttribute("image", provider.iconURL);
    button.setAttribute("label", provider.name);
    button.setAttribute("tooltiptext", provider.name);
    button.setAttribute("origin", provider.origin);
    button.setAttribute("oncommand", "SocialStatus.showPopup(this);");
    button.setAttribute("id", this._toolbarHelper.idFromOrgin(provider.origin));
    palette.appendChild(button);
    return button;
  },

  
  
  _attachNotificatonPanel: function(aButton, provider) {
    let panel = document.getElementById("social-notification-panel");
    panel.hidden = !SocialUI.enabled;
    let notificationFrameId = "social-status-" + provider.origin;
    let frame = document.getElementById(notificationFrameId);

    if (!frame) {
      frame = SharedFrame.createFrame(
        notificationFrameId, 
        panel, 
        {
          "type": "content",
          "mozbrowser": "true",
          "class": "social-panel-frame",
          "id": notificationFrameId,
          "tooltip": "aHTMLTooltip",

          
          
          "style": "width: " + PANEL_MIN_WIDTH + "px;",

          "origin": provider.origin,
          "src": provider.statusURL
        }
      );

      if (frame.socialErrorListener)
        frame.socialErrorListener.remove();
      if (frame.docShell) {
        frame.docShell.isActive = false;
        Social.setErrorListener(frame, this.setPanelErrorMessage.bind(this));
      }
    } else {
      frame.setAttribute("origin", provider.origin);
      SharedFrame.updateURL(notificationFrameId, provider.statusURL);
    }
    aButton.setAttribute("notificationFrameId", notificationFrameId);
  },

  updateNotification: function(origin) {
    if (!Social.allowMultipleWorkers)
      return;
    let provider = Social._getProviderFromOrigin(origin);
    let button = document.getElementById(this._toolbarHelper.idFromOrgin(provider.origin));
    if (button) {
      
      let icons = provider.ambientNotificationIcons;
      let iconNames = Object.keys(icons);
      let notif = icons[iconNames[0]];
      if (!notif) {
        button.setAttribute("badge", "");
        button.setAttribute("aria-label", "");
        button.setAttribute("tooltiptext", "");
        return;
      }

      button.style.listStyleImage = "url(" + notif.iconURL || provider.iconURL + ")";
      button.setAttribute("tooltiptext", notif.label);

      let badge = notif.counter || "";
      button.setAttribute("badge", badge);
      let ariaLabel = notif.label;
      
      if (badge)
        ariaLabel = gNavigatorBundle.getFormattedString("social.aria.toolbarButtonBadgeText",
                                                        [ariaLabel, badge]);
      button.setAttribute("aria-label", ariaLabel);
    }
  },

  showPopup: function(aToolbarButton) {
    if (!Social.allowMultipleWorkers)
      return;
    
    let origin = aToolbarButton.getAttribute("origin");
    let provider = Social._getProviderFromOrigin(origin);
    this._attachNotificatonPanel(aToolbarButton, provider);

    let panel = document.getElementById("social-notification-panel");
    let notificationFrameId = aToolbarButton.getAttribute("notificationFrameId");
    let notificationFrame = document.getElementById(notificationFrameId);

    let wasAlive = SharedFrame.isGroupAlive(notificationFrameId);
    SharedFrame.setOwner(notificationFrameId, notificationFrame);

    
    
    let frameIter = panel.firstElementChild;
    while (frameIter) {
      frameIter.collapsed = (frameIter != notificationFrame);
      frameIter = frameIter.nextElementSibling;
    }

    function dispatchPanelEvent(name) {
      let evt = notificationFrame.contentDocument.createEvent("CustomEvent");
      evt.initCustomEvent(name, true, true, {});
      notificationFrame.contentDocument.documentElement.dispatchEvent(evt);
    }

    let dynamicResizer = this._dynamicResizer;
    panel.addEventListener("popuphidden", function onpopuphiding() {
      panel.removeEventListener("popuphidden", onpopuphiding);
      aToolbarButton.removeAttribute("open");
      dynamicResizer.stop();
      notificationFrame.docShell.isActive = false;
      dispatchPanelEvent("socialFrameHide");
    });

    panel.addEventListener("popupshown", function onpopupshown() {
      panel.removeEventListener("popupshown", onpopupshown);
      
      
      
      
      
      aToolbarButton.setAttribute("open", "true");
      notificationFrame.docShell.isActive = true;
      notificationFrame.docShell.isAppTab = true;
      if (notificationFrame.contentDocument.readyState == "complete" && wasAlive) {
        dynamicResizer.start(panel, notificationFrame);
        dispatchPanelEvent("socialFrameShow");
      } else {
        
        notificationFrame.addEventListener("load", function panelBrowserOnload(e) {
          notificationFrame.removeEventListener("load", panelBrowserOnload, true);
          dynamicResizer.start(panel, notificationFrame);
          dispatchPanelEvent("socialFrameShow");
        }, true);
      }
    });

    let navBar = document.getElementById("nav-bar");
    let anchor = navBar.getAttribute("mode") == "text" ?
                   document.getAnonymousElementByAttribute(aToolbarButton, "class", "toolbarbutton-text") :
                   document.getAnonymousElementByAttribute(aToolbarButton, "class", "toolbarbutton-badge-container");
    
    
    setTimeout(function() {
      panel.openPopup(anchor, "bottomcenter topright", 0, 0, false, false);
    }, 0);
  },

  setPanelErrorMessage: function(aNotificationFrame) {
    if (!aNotificationFrame)
      return;

    let src = aNotificationFrame.getAttribute("src");
    aNotificationFrame.removeAttribute("src");
    aNotificationFrame.webNavigation.loadURI("about:socialerror?mode=tryAgainOnly&url=" +
                                             encodeURIComponent(src),
                                             null, null, null, null);
    let panel = aNotificationFrame.parentNode;
    sizeSocialPanelToContent(panel, aNotificationFrame);
  },

};







SocialMarks = {
  update: function() {
    
    let currentButtons = document.querySelectorAll('toolbarbutton[type="socialmark"]');
    for (let elt of currentButtons)
      elt.update();
  },

  getProviders: function() {
    
    
    
    
    let tbh = this._toolbarHelper;
    return [p for (p of Social.providers) if (p.markURL &&
                                              document.getElementById(tbh.idFromOrgin(p.origin)))];
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

  setPosition: function(origin) {
    
    
    let manifest = Social.getManifestByOrigin(origin);
    if (!manifest.markURL)
      return;
    let tbh = this._toolbarHelper;
    tbh.setPersistentPosition(tbh.idFromOrgin(origin));
  },

  removePosition: function(origin) {
    let tbh = this._toolbarHelper;
    tbh.removePersistence(tbh.idFromOrgin(origin));
  },

  removeProvider: function(origin) {
    this._toolbarHelper.removeProviderButton(origin);
  },

  get _toolbarHelper() {
    delete this._toolbarHelper;
    this._toolbarHelper = new ToolbarHelper("social-mark-button", this._createButton.bind(this));
    return this._toolbarHelper;
  },

  _createButton: function(provider) {
    if (!provider.markURL)
      return null;
    let palette = document.getElementById("navigator-toolbox").palette;
    let button = document.createElement("toolbarbutton");
    button.setAttribute("type", "socialmark");
    button.setAttribute("class", "toolbarbutton-1 social-mark-button");
    button.style.listStyleImage = "url(" + provider.iconURL + ")";
    button.setAttribute("origin", provider.origin);
    button.setAttribute("id", this._toolbarHelper.idFromOrgin(provider.origin));
    palette.appendChild(button);
    return button
  },

  markLink: function(aOrigin, aUrl) {
    
    let id = this._toolbarHelper.idFromOrgin(aOrigin);
    document.getElementById(id).markLink(aUrl);
  }
};

})();
