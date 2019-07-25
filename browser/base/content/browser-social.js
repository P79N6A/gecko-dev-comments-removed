



let SocialUI = {
  
  init: function SocialUI_init() {
    Services.obs.addObserver(this, "social:pref-changed", false);
    Services.obs.addObserver(this, "social:ambient-notification-changed", false);
    Services.obs.addObserver(this, "social:profile-changed", false);

    Services.prefs.addObserver("social.sidebar.open", this, false);

    gBrowser.addEventListener("ActivateSocialFeature", this._activationEventHandler, true, true);

    Social.init(this._providerReady.bind(this));
  },

  
  uninit: function SocialUI_uninit() {
    Services.obs.removeObserver(this, "social:pref-changed");
    Services.obs.removeObserver(this, "social:ambient-notification-changed");
    Services.obs.removeObserver(this, "social:profile-changed");

    Services.prefs.removeObserver("social.sidebar.open", this);
  },

  showProfile: function SocialUI_showProfile() {
    if (Social.provider)
      openUILink(Social.provider.profile.profileURL);
  },

  observe: function SocialUI_observe(subject, topic, data) {
    switch (topic) {
      case "social:pref-changed":
        this.updateToggleCommand();
        SocialShareButton.updateButtonHiddenState();
        SocialToolbar.updateButtonHiddenState();
        SocialSidebar.updateSidebar();
        break;
      case "social:ambient-notification-changed":
        SocialToolbar.updateButton();
        break;
      case "social:profile-changed":
        SocialToolbar.updateProfile();
        SocialShareButton.updateProfileInfo();
        break;
      case "nsPref:changed":
        SocialSidebar.updateSidebar();
    }
  },

  get toggleCommand() {
    return document.getElementById("Social:Toggle");
  },

  
  _providerReady: function SocialUI_providerReady() {
    
    if (!Social.provider)
      return;

    this.updateToggleCommand();

    let toggleCommand = this.toggleCommand;
    let label = gNavigatorBundle.getFormattedString("social.enable.label",
                                                    [Social.provider.name]);
    let accesskey = gNavigatorBundle.getString("social.enable.accesskey");
    toggleCommand.setAttribute("label", label);
    toggleCommand.setAttribute("accesskey", accesskey);

    SocialToolbar.init();
    SocialShareButton.init();
    SocialSidebar.init();
  },

  updateToggleCommand: function SocialUI_updateToggleCommand() {
    let toggleCommand = this.toggleCommand;
    toggleCommand.setAttribute("checked", Social.enabled);

    
    
    
    for (let id of ["appmenu_socialToggle", "menu_socialToggle"]) {
      let el = document.getElementById(id);
      if (!el)
        continue;

      if (Social.active)
        el.removeAttribute("hidden");
      else
        el.setAttribute("hidden", "true");
    }
  },

  
  
  _activationEventHandler: function SocialUI_activationHandler(e) {
    
    
    if (Social.active || !Social.provider)
      return;

    let targetDoc = e.target;

    
    if (!(targetDoc instanceof HTMLDocument))
      return;

    
    if (targetDoc.defaultView.top != content)
      return;

    
    let prePath = targetDoc.documentURIObject.prePath;
    let whitelist = Services.prefs.getCharPref("browser.social.whitelist");
    if (whitelist.split(",").indexOf(prePath) == -1)
      return;

    
    let now = Date.now();
    if (now - Social.lastEventReceived < 1000)
      return;
    Social.lastEventReceived = now;

    
    Social.active = true;

    
    let description = document.getElementById("social-activation-message");
    let brandShortName = document.getElementById("bundle_brand").getString("brandShortName");
    let message = gNavigatorBundle.getFormattedString("social.activated.message",
                                                      [Social.provider.name, brandShortName]);
    description.value = message;

    SocialUI.notificationPanel.hidden = false;

    setTimeout(function () {
      SocialUI.notificationPanel.openPopup(SocialToolbar.button, "bottomcenter topright");
    }.bind(this), 0);
  },

  get notificationPanel() {
    return document.getElementById("socialActivatedNotification")
  },

  undoActivation: function SocialUI_undoActivation() {
    Social.active = false;
    this.notificationPanel.hidePopup();
  }
}

let SocialShareButton = {
  
  init: function SSB_init() {
    this.updateButtonHiddenState();
    this.updateProfileInfo();
  },

  updateProfileInfo: function SSB_updateProfileInfo() {
    let profileRow = document.getElementById("editSharePopupHeader");
    let profile = Social.provider.profile;
    if (profile && profile.portrait && profile.displayName) {
      profileRow.hidden = false;
      let portrait = document.getElementById("socialUserPortrait");
      portrait.style.listStyleImage = profile.portrait;
      let displayName = document.getElementById("socialUserDisplayName");
      displayName.setAttribute("label", profile.displayName);
    } else {
      profileRow.hidden = true;
    }
  },

  get shareButton() {
    return document.getElementById("share-button");
  },
  get sharePopup() {
    return document.getElementById("editSharePopup");
  },

  dismissSharePopup: function SSB_dismissSharePopup() {
    this.sharePopup.hidePopup();
  },

  updateButtonHiddenState: function SSB_updateButtonHiddenState() {
    let shareButton = this.shareButton;
    if (shareButton)
      shareButton.hidden = !Social.uiVisible;
  },

  onClick: function SSB_onClick(aEvent) {
    if (aEvent.button != 0)
      return;

    
    aEvent.stopPropagation();

    this.sharePage();
  },

  panelShown: function SSB_panelShown(aEvent) {
    let sharePopupOkButton = document.getElementById("editSharePopupOkButton");
    if (sharePopupOkButton)
      sharePopupOkButton.focus();
  },

  sharePage: function SSB_sharePage() {
    this.sharePopup.hidden = false;

    let uri = gBrowser.currentURI;
    if (!Social.isPageShared(uri)) {
      Social.sharePage(uri);
      this.updateShareState();
    } else {
      this.sharePopup.openPopup(this.shareButton, "bottomcenter topright");
    }
  },

  unsharePage: function SSB_unsharePage() {
    Social.unsharePage(gBrowser.currentURI);
    this.updateShareState();
    this.dismissSharePopup();
  },

  updateShareState: function SSB_updateShareState() {
    let currentPageShared = Social.isPageShared(gBrowser.currentURI);

    
    let status = document.getElementById("share-button-status");
    if (status) {
      let statusString = currentPageShared ?
                           gNavigatorBundle.getString("social.pageShared.label") : "";
      status.setAttribute("value", statusString);
    }

    
    let shareButton = this.shareButton;
    if (!shareButton)
      return;

    if (currentPageShared) {
      shareButton.setAttribute("shared", "true");
      shareButton.setAttribute("tooltiptext", gNavigatorBundle.getString("social.shareButton.sharedtooltip"));
    } else {
      shareButton.removeAttribute("shared");
      shareButton.setAttribute("tooltiptext", gNavigatorBundle.getString("social.shareButton.tooltip"));
    }
  }
};

var SocialToolbar = {
  
  init: function SocialToolbar_init() {
    document.getElementById("social-provider-image").setAttribute("image", Social.provider.iconURL);

    let removeItem = document.getElementById("social-remove-menuitem");
    let brandShortName = document.getElementById("bundle_brand").getString("brandShortName");
    let label = gNavigatorBundle.getFormattedString("social.remove.label",
                                                    [brandShortName]);
    let accesskey = gNavigatorBundle.getString("social.remove.accesskey");
    removeItem.setAttribute("label", label);
    removeItem.setAttribute("accesskey", accesskey);

    let statusAreaPopup = document.getElementById("social-statusarea-popup");
    statusAreaPopup.addEventListener("popupshowing", function(e) {
      this.button.setAttribute("open", "true");
    }.bind(this));
    statusAreaPopup.addEventListener("popuphidden", function(e) {
      this.button.removeAttribute("open");
    }.bind(this));

    this.updateButton();
    this.updateProfile();
  },

  get button() {
    return document.getElementById("social-toolbar-button");
  },

  updateButtonHiddenState: function SocialToolbar_updateButtonHiddenState() {
    this.button.hidden = !Social.uiVisible;
  },

  updateProfile: function SocialToolbar_updateProfile() {
    
    
    
    let profile = Social.provider.profile || {};
    let userPortrait = profile.portrait || "chrome://browser/skin/social/social.png";
    document.getElementById("social-statusarea-user-portrait").setAttribute("src", userPortrait);

    let notLoggedInLabel = document.getElementById("social-statusarea-notloggedin");
    let userNameBtn = document.getElementById("social-statusarea-username");
    if (profile.userName) {
      notLoggedInLabel.hidden = true;
      userNameBtn.hidden = false;
      userNameBtn.label = profile.userName;
    } else {
      notLoggedInLabel.hidden = false;
      userNameBtn.hidden = true;
    }
  },

  updateButton: function SocialToolbar_updateButton() {
    this.updateButtonHiddenState();

    let provider = Social.provider;
    
    let iconNames = Object.keys(provider.ambientNotificationIcons);
    let iconBox = document.getElementById("social-status-iconbox");
    for (var i = 0; i < iconBox.childNodes.length; i++) {
      let iconContainer = iconBox.childNodes[i];
      if (i > iconNames.length - 1) {
        iconContainer.collapsed = true;
        continue;
      }

      iconContainer.collapsed = false;
      let icon = provider.ambientNotificationIcons[iconNames[i]];
      let iconImage = iconContainer.firstChild;
      let iconCounter = iconImage.nextSibling;

      iconImage.setAttribute("contentPanel", icon.contentPanel);
      iconImage.setAttribute("src", icon.iconURL);

      if (iconCounter.firstChild)
        iconCounter.removeChild(iconCounter.firstChild);

      if (icon.counter) {
        iconCounter.appendChild(document.createTextNode(icon.counter));
        iconCounter.collapsed = false;
      } else {
        iconCounter.collapsed = true;
      }
    }
  },

  showAmbientPopup: function SocialToolbar_showAmbientPopup(iconContainer) {
    let iconImage = iconContainer.firstChild;
    let panel = document.getElementById("social-notification-panel");
    let notifBrowser = document.getElementById("social-notification-browser");

    panel.hidden = false;

    function sizePanelToContent() {
      
      
      let doc = notifBrowser.contentDocument;
      
      
      let body = doc.getElementById("notif") || (doc.body && doc.body.firstChild);
      if (!body)
        return;
      let h = body.scrollHeight > 0 ? body.scrollHeight : 300;
      notifBrowser.style.width = body.scrollWidth + "px";
      notifBrowser.style.height = h + "px";
    }

    notifBrowser.addEventListener("DOMContentLoaded", function onload() {
      notifBrowser.removeEventListener("DOMContentLoaded", onload);
      sizePanelToContent();
    });

    panel.addEventListener("popuphiding", function onpopuphiding() {
      panel.removeEventListener("popuphiding", onpopuphiding);
      
      SocialToolbar.button.removeAttribute("open");
      notifBrowser.setAttribute("src", "about:blank");
    });

    notifBrowser.setAttribute("origin", Social.provider.origin);
    notifBrowser.setAttribute("src", iconImage.getAttribute("contentPanel"));
    this.button.setAttribute("open", "true");
    panel.openPopup(iconImage, "bottomcenter topleft", 0, 0, false, false);
  }
}

var SocialSidebar = {
  
  init: function SocialSidebar_init() {
    this.updateSidebar();
  },

  
  get canShow() {
    return Social.uiVisible && Social.provider.sidebarURL && !this.chromeless;
  },

  
  
  get chromeless() {
    let docElem = document.documentElement;
    return docElem.getAttribute('disablechrome') ||
           docElem.getAttribute('chromehidden').indexOf("extrachrome") >= 0;
  },

  
  get enabled() {
    return Services.prefs.getBoolPref("social.sidebar.open");
  },

  updateSidebar: function SocialSidebar_updateSidebar() {
    
    let command = document.getElementById("Social:ToggleSidebar");
    command.hidden = !this.canShow;

    
    
    let hideSidebar = !this.canShow || !this.enabled;
    let broadcaster = document.getElementById("socialSidebarBroadcaster");
    broadcaster.hidden = hideSidebar;
    command.setAttribute("checked", !hideSidebar);

    
    
    let sbrowser = document.getElementById("social-sidebar-browser");
    if (broadcaster.hidden) {
      sbrowser.removeAttribute("origin");
      sbrowser.setAttribute("src", "about:blank");
      return;
    }

    
    sbrowser.setAttribute("origin", Social.provider.origin);
    sbrowser.setAttribute("src", Social.provider.sidebarURL);
  }
}
