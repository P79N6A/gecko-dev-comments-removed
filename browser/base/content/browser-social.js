




const PANEL_MIN_HEIGHT = 100;
const PANEL_MIN_WIDTH = 330;

XPCOMUtils.defineLazyModuleGetter(this, "SharedFrame",
  "resource:///modules/SharedFrame.jsm");

let SocialUI = {
  
  init: function SocialUI_init() {
    Services.obs.addObserver(this, "social:pref-changed", false);
    Services.obs.addObserver(this, "social:ambient-notification-changed", false);
    Services.obs.addObserver(this, "social:profile-changed", false);
    Services.obs.addObserver(this, "social:recommend-info-changed", false);
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
    SocialShareButton.init();
    SocialMenu.init();
    SocialToolbar.init();
    SocialSidebar.init();

    Social.init();
    
    
    
    if (Social.provider) {
      this.observe(null, "social:provider-set", Social.provider.origin);
      this.observe(null, "social:providers-changed", null);
    }
  },

  
  uninit: function SocialUI_uninit() {
    Services.obs.removeObserver(this, "social:pref-changed");
    Services.obs.removeObserver(this, "social:ambient-notification-changed");
    Services.obs.removeObserver(this, "social:profile-changed");
    Services.obs.removeObserver(this, "social:recommend-info-changed");
    Services.obs.removeObserver(this, "social:frameworker-error");
    Services.obs.removeObserver(this, "social:provider-set");
    Services.obs.removeObserver(this, "social:providers-changed");

    Services.prefs.removeObserver("social.sidebar.open", this);
    Services.prefs.removeObserver("social.toast-notifications.enabled", this);
  },

  
  
  
  _updateProvider: function () {
    
    this._updateActiveUI();
    this._updateMenuItems();

    SocialChatBar.update();
    SocialShareButton.updateProvider();
    SocialMenu.populate();
    SocialToolbar.updateProvider();
    SocialSidebar.update();
  },

  
  _updateEnabledState: function () {
    this._updateActiveUI();
    SocialChatBar.update();
    SocialSidebar.update();
    SocialShareButton.updateButtonHiddenState();
    SocialMenu.populate();
    SocialToolbar.updateButtonHiddenState();
    SocialToolbar.populateProviderMenus();
  },

  _matchesCurrentProvider: function (origin) {
    return Social.provider && Social.provider.origin == origin;
  },

  observe: function SocialUI_observe(subject, topic, data) {
    
    
    try {
      switch (topic) {
        case "social:provider-set":
          this._updateProvider();
          break;
        case "social:providers-changed":
          
          this._updateActiveUI();
          
          SocialToolbar.populateProviderMenus();
          break;
        case "social:pref-changed":
          this._updateEnabledState();
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
            SocialShareButton.updateProfileInfo();
            SocialChatBar.update();
          }
          break;
        case "social:recommend-info-changed":
          if (this._matchesCurrentProvider(data)) {
            SocialShareButton.updateShareState();
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
    if (!Social.provider)
      return;

    
    for (let id of ["menu_socialSidebar", "menu_socialAmbientMenu"])
      document.getElementById(id).setAttribute("label", Social.provider.name);
  },

  
  
  _activationEventHandler: function SocialUI_activationHandler(e) {
    let targetDoc = e.target;

    
    if (!(targetDoc instanceof HTMLDocument))
      return;

    
    if (targetDoc.defaultView.top != content)
      return;

    
    let providerOrigin = targetDoc.nodePrincipal.origin;
    if (!Social.canActivateOrigin(providerOrigin))
      return;

    
    
    if (PrivateBrowsingUtils.isWindowPrivate(window))
      return;

    
    let now = Date.now();
    if (now - Social.lastEventReceived < 1000)
      return;
    Social.lastEventReceived = now;

    
    let oldOrigin = Social.provider ? Social.provider.origin : "";

    
    Social.activateFromOrigin(providerOrigin, function(provider) {
      
      if (!provider)
        return;

      
      let description = document.getElementById("social-activation-message");
      let brandShortName = document.getElementById("bundle_brand").getString("brandShortName");
      let message = gNavigatorBundle.getFormattedString("social.activated.description",
                                                        [provider.name, brandShortName]);
      description.value = message;

      let icon = document.getElementById("social-activation-icon");
      if (provider.icon64URL || provider.icon32URL) {
        icon.setAttribute('src', provider.icon64URL || provider.icon32URL);
        icon.hidden = false;
      } else {
        icon.removeAttribute('src');
        icon.hidden = true;
      }

      let notificationPanel = SocialUI.notificationPanel;
      
      notificationPanel.setAttribute("origin", provider.origin);
      notificationPanel.setAttribute("oldorigin", oldOrigin);

      
      notificationPanel.hidden = false;
      setTimeout(function () {
        notificationPanel.openPopup(SocialToolbar.button, "bottomcenter topright");
      }, 0);
    });
  },

  undoActivation: function SocialUI_undoActivation() {
    let origin = this.notificationPanel.getAttribute("origin");
    let oldOrigin = this.notificationPanel.getAttribute("oldorigin");
    Social.deactivateFromOrigin(origin, oldOrigin);
    this.notificationPanel.hidePopup();
  },

  get notificationPanel() {
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

  disableWithConfirmation: function SocialUI_disableWithConfirmation() {
    let brandShortName = document.getElementById("bundle_brand").getString("brandShortName");
    let dialogTitle = gNavigatorBundle.getFormattedString("social.remove.confirmationOK",
                                                          [Social.provider.name]);
    let text = gNavigatorBundle.getFormattedString("social.remove.confirmationLabel",
                                                   [Social.provider.name, brandShortName]);
    let okButtonText = dialogTitle;

    let ps = Services.prompt;
    let flags = ps.BUTTON_TITLE_IS_STRING * ps.BUTTON_POS_0 +
                ps.BUTTON_TITLE_CANCEL * ps.BUTTON_POS_1 +
                ps.BUTTON_POS_0_DEFAULT;

    let confirmationIndex = ps.confirmEx(null, dialogTitle, text, flags,
                                         okButtonText, null, null, null, {});
    if (confirmationIndex == 0) {
      Social.deactivateFromOrigin(Social.provider.origin);
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

let SocialChatBar = {
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
    if (this.isAvailable) {
      this.chatbar.openChat(aProvider, aURL, aCallback, aMode);
      
      let dwu = window.QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIDOMWindowUtils);
      if (dwu.isHandlingUserInput)
        this.chatbar.focus();
    }
  },
  update: function() {
    let command = document.getElementById("Social:FocusChat");
    if (!this.isAvailable) {
      this.chatbar.removeAll();
      command.hidden = true;
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

let SocialFlyout = {
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

let SocialShareButton = {
  
  init: function SSB_init() {
  },

  
  updateProvider: function () {
    this.updateButtonHiddenState();
    if (!Social.provider)
      return;
    this.updateProfileInfo();
  },

  
  
  updateProfileInfo: function SSB_updateProfileInfo() {
    let profileRow = document.getElementById("unsharePopupHeader");
    let profile = SocialUI.enabled ? Social.provider.profile : null;
    if (profile && profile.displayName) {
      profileRow.hidden = false;
      let portrait = document.getElementById("socialUserPortrait");
      portrait.setAttribute("src", profile.portrait || "chrome://global/skin/icons/information-32.png");
      let displayName = document.getElementById("socialUserDisplayName");
      displayName.setAttribute("label", profile.displayName);
    } else {
      profileRow.hidden = true;
      this.updateButtonHiddenState();
    }
  },

  get shareButton() {
    return document.getElementById("share-button");
  },
  get unsharePopup() {
    return document.getElementById("unsharePopup");
  },

  dismissUnsharePopup: function SSB_dismissUnsharePopup() {
    this.unsharePopup.hidePopup();
  },

  canSharePage: function SSB_canSharePage(aURI) {
    
    return aURI && (aURI.schemeIs('http') || aURI.schemeIs('https'));
  },

  updateButtonHiddenState: function SSB_updateButtonHiddenState() {
    let shareButton = this.shareButton;
    if (shareButton)
      shareButton.hidden = !SocialUI.enabled || Social.provider.recommendInfo == null ||
                           !Social.haveLoggedInUser() ||
                           !this.canSharePage(gBrowser.currentURI);

    
    
    let cmd = document.getElementById("Social:SharePage");
    cmd.setAttribute("disabled", shareButton.hidden ? "true" : "false");
  },

  onClick: function SSB_onClick(aEvent) {
    if (aEvent.button != 0)
      return;

    
    aEvent.stopPropagation();

    this.sharePage();
  },

  panelShown: function SSB_panelShown(aEvent) {
    function updateElement(id, attrs) {
      let el = document.getElementById(id);
      Object.keys(attrs).forEach(function(attr) {
        el.setAttribute(attr, attrs[attr]);
      });
    }
    let continueSharingButton = document.getElementById("unsharePopupContinueSharingButton");
    continueSharingButton.focus();
    let recommendInfo = Social.provider.recommendInfo;
    updateElement("unsharePopupContinueSharingButton",
                  {label: recommendInfo.messages.unshareCancelLabel,
                   accesskey: recommendInfo.messages.unshareCancelAccessKey});
    updateElement("unsharePopupStopSharingButton",
                  {label: recommendInfo.messages.unshareConfirmLabel,
                  accesskey: recommendInfo.messages.unshareConfirmAccessKey});
    updateElement("socialUserPortrait",
                  {"aria-label": recommendInfo.messages.portraitLabel});
    updateElement("socialUserRecommendedText",
                  {value: recommendInfo.messages.unshareLabel});
  },

  sharePage: function SSB_sharePage() {
    this.unsharePopup.hidden = false;

    let uri = gBrowser.currentURI;
    if (!Social.isPageShared(uri)) {
      Social.sharePage(uri);
      this.updateShareState();
    } else {
      this.unsharePopup.openPopup(this.shareButton, "bottomcenter topright");
    }
  },

  unsharePage: function SSB_unsharePage() {
    Social.unsharePage(gBrowser.currentURI);
    this.updateShareState();
    this.dismissUnsharePopup();
  },

  updateShareState: function SSB_updateShareState() {
    this.updateButtonHiddenState();

    let shareButton = this.shareButton;
    let currentPageShared = shareButton && !shareButton.hidden && Social.isPageShared(gBrowser.currentURI);

    let recommendInfo = SocialUI.enabled ? Social.provider.recommendInfo : null;
    
    let status = document.getElementById("share-button-status");
    if (status) {
      
      
      
      
      
      
      let statusString = currentPageShared && recommendInfo ?
                           recommendInfo.messages.sharedLabel : "";
      status.setAttribute("value", statusString);
    }

    
    if (!shareButton || shareButton.hidden)
      return;

    let imageURL;
    if (currentPageShared) {
      shareButton.setAttribute("shared", "true");
      shareButton.setAttribute("tooltiptext", recommendInfo.messages.unshareTooltip);
      imageURL = recommendInfo.images.unshare;
    } else {
      shareButton.removeAttribute("shared");
      shareButton.setAttribute("tooltiptext", recommendInfo.messages.shareTooltip);
      imageURL = recommendInfo.images.share;
    }
    shareButton.src = imageURL;
  }
};

var SocialMenu = {
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


var SocialToolbar = {
  
  
  init: function SocialToolbar_init() {
    let accesskey = gNavigatorBundle.getString("social.removeProvider.accesskey");
    let removeCommand = document.getElementById("Social:Remove");
    removeCommand.setAttribute("accesskey", accesskey);
    this._dynamicResizer = new DynamicResizeWatcher();
  },

  
  updateProvider: function () {
    let provider = Social.provider || Social.defaultProvider;
    if (provider) {
      let label = gNavigatorBundle.getFormattedString("social.removeProvider.label",
                                                      [provider.name]);
      let removeCommand = document.getElementById("Social:Remove");
      removeCommand.setAttribute("label", label);
      this.button.setAttribute("label", provider.name);
      this.button.setAttribute("tooltiptext", provider.name);
      this.button.style.listStyleImage = "url(" + provider.iconURL + ")";

      this.updateProfile();
    }
    this.updateButton();
    this.populateProviderMenus();
  },

  get button() {
    return document.getElementById("social-provider-button");
  },

  
  
  updateButtonHiddenState: function SocialToolbar_updateButtonHiddenState() {
    let tbi = document.getElementById("social-toolbar-item");
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

      while (tbi.lastChild != tbi.firstChild)
        tbi.removeChild(tbi.lastChild);
    }
  },

  updateProfile: function SocialToolbar_updateProfile() {
    
    
    
    if (!Social.provider)
      return;
    let profile = Social.provider.profile || {};
    let userPortrait = profile.portrait || "chrome://global/skin/icons/information-32.png";

    let userDetailsBroadcaster = document.getElementById("socialBroadcaster_userDetails");
    let loggedInStatusValue = profile.userName ||
                              userDetailsBroadcaster.getAttribute("notLoggedInLabel");

    
    
    userDetailsBroadcaster.setAttribute("src", userPortrait);
    userDetailsBroadcaster.setAttribute("image", userPortrait);

    userDetailsBroadcaster.setAttribute("value", loggedInStatusValue);
    userDetailsBroadcaster.setAttribute("label", loggedInStatusValue);
  },

  
  updateButton: function SocialToolbar_updateButton() {
    this.updateButtonHiddenState();
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

      let toolbarButtonContainerId = "social-notification-container-" + icon.name;
      let toolbarButtonId = "social-notification-icon-" + icon.name;
      let toolbarButtonContainer = document.getElementById(toolbarButtonContainerId);
      let toolbarButton = document.getElementById(toolbarButtonId);
      if (!toolbarButtonContainer) {
        
        
        
        toolbarButtonContainer = document.createElement("toolbaritem");
        toolbarButtonContainer.classList.add("social-notification-container");
        toolbarButtonContainer.setAttribute("id", toolbarButtonContainerId);

        toolbarButton = document.createElement("toolbarbutton");
        toolbarButton.classList.add("toolbarbutton-1");
        toolbarButton.setAttribute("id", toolbarButtonId);
        toolbarButton.setAttribute("notificationFrameId", notificationFrameId);
        toolbarButton.addEventListener("mousedown", function (event) {
          if (event.button == 0 && panel.state == "closed")
            SocialToolbar.showAmbientPopup(toolbarButton);
        });

        toolbarButtonContainer.appendChild(toolbarButton);
        toolbarButtons.appendChild(toolbarButtonContainer);
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
                   document.getAnonymousElementByAttribute(aToolbarButton, "class", "toolbarbutton-icon");
    panel.openPopup(anchor, "bottomcenter topright", 0, 0, false, false);
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
    
    if (!SocialUI.enabled || Social.providers.length < 2) {
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

var SocialSidebar = {
  
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
