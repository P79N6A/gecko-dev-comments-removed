




let SocialUI,
    SocialChatBar,
    SocialFlyout,
    SocialMark,
    SocialMenu,
    SocialToolbar,
    SocialSidebar;

(function() {


const PANEL_MIN_HEIGHT = 100;
const PANEL_MIN_WIDTH = 330;

XPCOMUtils.defineLazyModuleGetter(this, "SharedFrame",
  "resource:///modules/SharedFrame.jsm");

SocialUI = {
  
  init: function SocialUI_init() {
    Services.obs.addObserver(this, "social:ambient-notification-changed", false);
    Services.obs.addObserver(this, "social:profile-changed", false);
    Services.obs.addObserver(this, "social:page-mark-config", false);
    Services.obs.addObserver(this, "social:frameworker-error", false);
    Services.obs.addObserver(this, "social:provider-set", false);
    Services.obs.addObserver(this, "social:providers-changed", false);

    Services.prefs.addObserver("social.sidebar.open", this, false);
    Services.prefs.addObserver("social.toast-notifications.enabled", this, false);

    gBrowser.addEventListener("ActivateSocialFeature", this._activationEventHandler.bind(this), true, true);

    
    window.addEventListener("mozfullscreenchange", function () {
      SocialSidebar.update();
      SocialChatBar.update();
    });

    SocialChatBar.init();
    SocialMark.init();
    SocialMenu.init();
    SocialToolbar.init();
    SocialSidebar.init();

    if (!Social.initialized) {
      Social.init();
    } else {
      
      
      this.observe(null, "social:providers-changed", null);
      this.observe(null, "social:provider-set", Social.provider ? Social.provider.origin : null);
    }
  },

  
  uninit: function SocialUI_uninit() {
    Services.obs.removeObserver(this, "social:ambient-notification-changed");
    Services.obs.removeObserver(this, "social:profile-changed");
    Services.obs.removeObserver(this, "social:page-mark-config");
    Services.obs.removeObserver(this, "social:frameworker-error");
    Services.obs.removeObserver(this, "social:provider-set");
    Services.obs.removeObserver(this, "social:providers-changed");

    Services.prefs.removeObserver("social.sidebar.open", this);
    Services.prefs.removeObserver("social.toast-notifications.enabled", this);
  },

  _matchesCurrentProvider: function (origin) {
    return Social.provider && Social.provider.origin == origin;
  },

  observe: function SocialUI_observe(subject, topic, data) {
    
    
    try {
      switch (topic) {
        case "social:provider-set":
          
          
          this._updateActiveUI();
          this._updateMenuItems();

          SocialFlyout.unload();
          SocialChatBar.update();
          SocialSidebar.update();
          SocialMark.update();
          SocialToolbar.update();
          SocialMenu.populate();
          break;
        case "social:providers-changed":
          
          this._updateActiveUI();
          
          SocialToolbar.populateProviderMenus();
          break;

        
        case "social:ambient-notification-changed":
          if (this._matchesCurrentProvider(data)) {
            SocialToolbar.updateButton();
            SocialMenu.populate();
          }
          break;
        case "social:profile-changed":
          if (this._matchesCurrentProvider(data)) {
            SocialToolbar.updateProfile();
            SocialMark.update();
            SocialChatBar.update();
          }
          break;
        case "social:page-mark-config":
          if (this._matchesCurrentProvider(data)) {
            SocialMark.updateMarkState();
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
    if (Social.haveLoggedInUser())
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
      
      let label = gNavigatorBundle.getFormattedString(Social.provider ?
                                                        "social.turnOff.label" :
                                                        "social.turnOn.label",
                                                      [provider.name]);
      let accesskey = gNavigatorBundle.getString(Social.provider ?
                                                   "social.turnOff.accesskey" :
                                                   "social.turnOn.accesskey");
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
      containerParent.hidePopup();
    }
  },

  get _chromeless() {
    
    let docElem = document.documentElement;
    let chromeless = docElem.getAttribute("chromehidden").indexOf("extrachrome") >= 0;
    
    
    delete this._chromeless;
    this._chromeless = chromeless;
    return chromeless;
  },

  get enabled() {
    
    if (this._chromeless || PrivateBrowsingUtils.isWindowPrivate(window))
      return false;
    return !!Social.provider;
  },

}

SocialChatBar = {
  init: function() {
  },
  get chatbar() {
    return document.getElementById("pinnedchats");
  },
  
  
  get isAvailable() {
    return SocialUI.enabled && Social.haveLoggedInUser();
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
      this.chatbar.removeAll();
      this.chatbar.hidden = command.hidden = true;
    } else {
      this.chatbar.hidden = command.hidden = document.mozFullScreen;
    }
    command.setAttribute("disabled", command.hidden ? "true" : "false");
  },
  focus: function SocialChatBar_focus() {
    this.chatbar.focus();
  }
}

function sizeSocialPanelToContent(panel, iframe) {
  
  let doc = iframe.contentDocument;
  if (!doc || !doc.body) {
    return;
  }
  let body = doc.body;
  
  let cs = doc.defaultView.getComputedStyle(body);
  let computedHeight = parseInt(cs.marginTop) + body.offsetHeight + parseInt(cs.marginBottom);
  let height = Math.max(computedHeight, PANEL_MIN_HEIGHT);
  let computedWidth = parseInt(cs.marginLeft) + body.offsetWidth + parseInt(cs.marginRight);
  let width = Math.max(computedWidth, PANEL_MIN_WIDTH);
  let wDiff = width - iframe.getBoundingClientRect().width;
  
  
  
  if (wDiff !== 0 && panel.getAttribute("side") == "right") {
    let box = panel.boxObject;
    panel.moveTo(box.screenX - wDiff, box.screenY);
  }
  iframe.style.height = height + "px";
  iframe.style.width = width + "px";
}

function DynamicResizeWatcher() {
  this._mutationObserver = null;
}

DynamicResizeWatcher.prototype = {
  start: function DynamicResizeWatcher_start(panel, iframe) {
    this.stop(); 
    let doc = iframe.contentDocument;
    this._mutationObserver = new iframe.contentWindow.MutationObserver(function(mutations) {
      sizeSocialPanelToContent(panel, iframe);
    });
    
    let config = {attributes: true, characterData: true, childList: true, subtree: true};
    this._mutationObserver.observe(doc, config);
    
    
    sizeSocialPanelToContent(panel, iframe);
  },
  stop: function DynamicResizeWatcher_stop() {
    if (this._mutationObserver) {
      try {
        this._mutationObserver.disconnect();
      } catch (ex) {
        
        
      }
      this._mutationObserver = null;
    }
  }
}

SocialFlyout = {
  get panel() {
    return document.getElementById("social-flyout-panel");
  },

  dispatchPanelEvent: function(name) {
    let doc = this.panel.firstChild.contentDocument;
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
    let iframe = this.panel.firstChild;
    if (!iframe)
      return;

    iframe.removeAttribute("src");
    iframe.webNavigation.loadURI("about:socialerror?mode=compactInfo", null, null, null, null);
    sizeSocialPanelToContent(this.panel, iframe);
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
    let iframe = panel.firstChild;
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
    this.panel.firstChild.docShell.isActive = false;
    this.dispatchPanelEvent("socialFrameHide");
  },

  open: function(aURL, yOffset, aCallback) {
    
    document.getElementById("social-notification-panel").hidePopup();

    if (!SocialUI.enabled)
      return;
    let panel = this.panel;
    if (!panel.firstChild)
      this._createFrame();
    panel.hidden = false;
    let iframe = panel.firstChild;

    let src = iframe.getAttribute("src");
    if (src != aURL) {
      iframe.addEventListener("load", function documentLoaded() {
        iframe.removeEventListener("load", documentLoaded, true);
        if (aCallback) {
          try {
            aCallback(iframe.contentWindow);
          } catch(e) {
            Cu.reportError(e);
          }
        }
      }, true);
      iframe.setAttribute("src", aURL);
    }
    else if (aCallback) {
      try {
        aCallback(iframe.contentWindow);
      } catch(e) {
        Cu.reportError(e);
      }
    }

    sizeSocialPanelToContent(panel, iframe);
    let anchor = document.getElementById("social-sidebar-browser");
    if (panel.state == "open") {
      
      
      
      
      let yAdjust = yOffset - this.yOffset;
      let box = panel.boxObject;
      panel.moveTo(box.screenX, box.screenY + yAdjust);
    } else {
      panel.openPopup(anchor, "start_before", 0, yOffset, false, false);
      
      
      panel.firstChild.clientTop;
      Social.setErrorListener(iframe, this.setFlyoutErrorMessage.bind(this))
    }
    this.yOffset = yOffset;
  }
}

SocialMark = {
  
  init: function SSB_init() {
  },

  get button() {
    return document.getElementById("social-mark-button");
  },

  canMarkPage: function SSB_canMarkPage(aURI) {
    
    return aURI && (aURI.schemeIs('http') || aURI.schemeIs('https'));
  },

  
  update: function SSB_updateButtonState() {
    let markButton = this.button;
    
    markButton.hidden = !SocialUI.enabled || Social.provider.pageMarkInfo == null;
    markButton.disabled = markButton.hidden || !this.canMarkPage(gBrowser.currentURI);

    
    
    let cmd = document.getElementById("Social:TogglePageMark");
    cmd.setAttribute("disabled", markButton.disabled ? "true" : "false");
  },

  togglePageMark: function(aCallback) {
    if (this.button.disabled)
      return;
    this.toggleURIMark(gBrowser.currentURI, aCallback)
  },
  
  toggleURIMark: function(aURI, aCallback) {
    let update = function(marked) {
      this._updateMarkState(marked);
      if (aCallback)
        aCallback(marked);
    }.bind(this);
    Social.isURIMarked(aURI, function(marked) {
      if (marked) {
        Social.unmarkURI(aURI, update);
      } else {
        Social.markURI(aURI, update);
      }
    });
  },

  updateMarkState: function SSB_updateMarkState() {
    this.update();
    if (!this.button.hidden)
      Social.isURIMarked(gBrowser.currentURI, this._updateMarkState.bind(this));
  },

  _updateMarkState: function(currentPageMarked) {
    
    let markButton = this.button;
    let pageMarkInfo = SocialUI.enabled ? Social.provider.pageMarkInfo : null;

    
    if (!markButton || markButton.hidden || !pageMarkInfo)
      return;

    let imageURL;
    if (!markButton.disabled && currentPageMarked) {
      markButton.setAttribute("marked", "true");
      markButton.setAttribute("label", pageMarkInfo.messages.markedLabel);
      markButton.setAttribute("tooltiptext", pageMarkInfo.messages.markedTooltip);
      imageURL = pageMarkInfo.images.marked;
    } else {
      markButton.removeAttribute("marked");
      markButton.setAttribute("label", pageMarkInfo.messages.unmarkedLabel);
      markButton.setAttribute("tooltiptext", pageMarkInfo.messages.unmarkedTooltip);
      imageURL = pageMarkInfo.images.unmarked;
    }
    markButton.style.listStyleImage = "url(" + imageURL + ")";
  }
};

SocialMenu = {
  init: function SocialMenu_init() {
  },

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
  
  
  init: function SocialToolbar_init() {
    this._dynamicResizer = new DynamicResizeWatcher();
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

    if (!Social.haveLoggedInUser() || !socialEnabled) {
      let parent = document.getElementById("social-notification-panel");
      while (parent.hasChildNodes()) {
        let frame = parent.firstChild;
        SharedFrame.forgetGroup(frame.id);
        parent.removeChild(frame);
      }

      let tbi = document.getElementById("social-toolbar-item");
      if (tbi) {
        
        let next = SocialMark.button.previousSibling;
        while (next != tbi.firstChild) {
          tbi.removeChild(next);
          next = SocialMark.button.previousSibling;
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
        (!Social.haveLoggedInUser() && Social.provider.profile !== undefined)) {
      
      
      
      
      Services.prefs.clearUserPref(CACHE_PREF_NAME);
      return;
    }
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
    socialToolbarItem.insertBefore(toolbarButtons, SocialMark.button);

    for (let frame of createdFrames) {
      if (frame.socialErrorListener) {
        frame.socialErrorListener.remove();
      }
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
    
    if (Social.providers.length < 2) {
      providerMenuSep.hidden = true;
      return;
    }
    for (let provider of Social.providers) {
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
  
  init: function SocialSidebar_init() {
    let sbrowser = document.getElementById("social-sidebar-browser");
    Social.setErrorListener(sbrowser, this.setSidebarErrorMessage.bind(this));
    
    sbrowser.docShell.isAppTab = true;
  },

  
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
        sbrowser.setAttribute("src", Social.provider.sidebarURL);
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

})();
