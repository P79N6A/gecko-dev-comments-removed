



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
        
        
        try {
          this.updateToggleCommand();
          SocialShareButton.updateButtonHiddenState();
          SocialToolbar.updateButtonHiddenState();
          SocialSidebar.updateSidebar();
          SocialChatBar.update();
          SocialFlyout.unload();
        } catch (e) {
          Components.utils.reportError(e);
          throw e;
        }
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
    let brandShortName = document.getElementById("bundle_brand").getString("brandShortName");
    let label = gNavigatorBundle.getFormattedString("social.toggle.label",
                                                    [Social.provider.name,
                                                     brandShortName]);
    let accesskey = gNavigatorBundle.getString("social.toggle.accesskey");
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
    let whitelist = Services.prefs.getCharPref("social.activation.whitelist");
    if (whitelist.split(",").indexOf(prePath) == -1)
      return;

    
    let now = Date.now();
    if (now - Social.lastEventReceived < 1000)
      return;
    Social.lastEventReceived = now;

    
    Social.active = true;

    
    let description = document.getElementById("social-activation-message");
    let brandShortName = document.getElementById("bundle_brand").getString("brandShortName");
    let message = gNavigatorBundle.getFormattedString("social.activated.description",
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

let SocialChatBar = {
  get chatbar() {
    return document.getElementById("pinnedchats");
  },
  
  get canShow() {
    let docElem = document.documentElement;
    let chromeless = docElem.getAttribute("disablechrome") ||
                     docElem.getAttribute("chromehidden").indexOf("extrachrome") >= 0;
    return Social.uiVisible && !chromeless;
  },
  openChat: function(aProvider, aURL, aCallback, aMode) {
    if (this.canShow)
      this.chatbar.openChat(aProvider, aURL, aCallback, aMode);
  },
  update: function() {
    if (!this.canShow)
      this.chatbar.removeAll();
  }
}

function sizeSocialPanelToContent(iframe) {
  
  
  let doc = iframe.contentDocument;
  if (!doc) {
    return;
  }
  
  
  let body = doc.getElementById("notif") || doc.body;
  if (!body || !body.firstChild) {
    return;
  }

  let [height, width] = [body.firstChild.offsetHeight || 300, 330];
  iframe.style.width = width + "px";
  iframe.style.height = height + "px";
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
    if (!Social.provider || panel.firstChild)
      return;
    
    let iframe = document.createElement("iframe");
    iframe.setAttribute("type", "content");
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("origin", Social.provider.origin);
    panel.appendChild(iframe);
  },

  unload: function() {
    let panel = this.panel;
    if (!panel.firstChild)
      return
    panel.removeChild(panel.firstChild);
  },

  onShown: function(aEvent) {
    let iframe = this.panel.firstChild;
    iframe.docShell.isActive = true;
    iframe.docShell.isAppTab = true;
    if (iframe.contentDocument.readyState == "complete") {
      this.dispatchPanelEvent("socialFrameShow");
    } else {
      
      iframe.addEventListener("load", function panelBrowserOnload(e) {
        iframe.removeEventListener("load", panelBrowserOnload, true);
        setTimeout(function() {
          SocialFlyout.dispatchPanelEvent("socialFrameShow");
        }, 0);
      }, true);
    }
  },

  onHidden: function(aEvent) {
    this.panel.firstChild.docShell.isActive = false;
    this.dispatchPanelEvent("socialFrameHide");
  },

  open: function(aURL, yOffset, aCallback) {
    if (!Social.provider)
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
        sizeSocialPanelToContent(iframe);
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

    sizeSocialPanelToContent(iframe);
    let anchor = document.getElementById("social-sidebar-browser");
    panel.openPopup(anchor, "start_before", 0, yOffset, false, false);
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
    if (profile && profile.displayName) {
      profileRow.hidden = false;
      let portrait = document.getElementById("socialUserPortrait");
      portrait.setAttribute("src", profile.portrait || "chrome://browser/skin/social/social.png");
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

    let statusAreaPopup = document.getElementById("social-statusarea-popup");
    statusAreaPopup.addEventListener("popupshown", function(e) {
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
    if (!Social.provider || !Social.provider.profile || !Social.provider.profile.userName) {
      ["social-notification-box",
       "social-status-iconbox"].forEach(function removeChildren(parentId) {
        let parent = document.getElementById(parentId);
        while(parent.hasChildNodes())
          parent.removeChild(parent.firstChild);
      });
    }
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
    let notifBox = document.getElementById("social-notification-box");
    let panel = document.getElementById("social-notification-panel");
    panel.hidden = false;
    let notificationFrames = document.createDocumentFragment();
    let iconContainers = document.createDocumentFragment();

    for each(let name in iconNames) {
      let icon = provider.ambientNotificationIcons[name];

      let notificationFrameId = "social-status-" + icon.name;
      let notificationFrame = document.getElementById(notificationFrameId);
      if (!notificationFrame) {
        notificationFrame = document.createElement("iframe");
        notificationFrame.setAttribute("type", "content");
        notificationFrame.setAttribute("id", notificationFrameId);
        notificationFrame.setAttribute("mozbrowser", "true");
        notificationFrames.appendChild(notificationFrame);
      }
      notificationFrame.setAttribute("origin", provider.origin);
      if (notificationFrame.getAttribute("src") != icon.contentPanel)
        notificationFrame.setAttribute("src", icon.contentPanel);

      let iconId = "social-notification-icon-" + icon.name;
      let iconContainer = document.getElementById(iconId);
      let iconImage, iconCounter;
      if (iconContainer) {
        iconImage = iconContainer.getElementsByClassName("social-notification-icon-image")[0];
        iconCounter = iconContainer.getElementsByClassName("social-notification-icon-counter")[0];
      } else {
        iconContainer = document.createElement("box");
        iconContainer.setAttribute("id", iconId);
        iconContainer.classList.add("social-notification-icon-container");
        iconContainer.addEventListener("click", function (e) { SocialToolbar.showAmbientPopup(iconContainer); }, false);

        iconImage = document.createElement("image");
        iconImage.classList.add("social-notification-icon-image");
        iconImage = iconContainer.appendChild(iconImage);

        iconCounter = document.createElement("box");
        iconCounter.classList.add("social-notification-icon-counter");
        iconCounter.appendChild(document.createTextNode(""));
        iconCounter = iconContainer.appendChild(iconCounter);

        iconContainers.appendChild(iconContainer);
      }
      if (iconImage.getAttribute("src") != icon.iconURL)
        iconImage.setAttribute("src", icon.iconURL);
      iconImage.setAttribute("notificationFrameId", notificationFrameId);

      iconCounter.collapsed = !icon.counter;
      iconCounter.firstChild.textContent = icon.counter || "";
    }
    notifBox.appendChild(notificationFrames);
    iconBox.appendChild(iconContainers);
  },

  showAmbientPopup: function SocialToolbar_showAmbientPopup(iconContainer) {
    let iconImage = iconContainer.firstChild;
    let panel = document.getElementById("social-notification-panel");
    let notifBox = document.getElementById("social-notification-box");
    let notificationFrame = document.getElementById(iconImage.getAttribute("notificationFrameId"));

    
    
    let frameIter = notifBox.firstElementChild;
    while (frameIter) {
      frameIter.collapsed = (frameIter != notificationFrame);
      frameIter = frameIter.nextElementSibling;
    }

    function dispatchPanelEvent(name) {
      let evt = notificationFrame.contentDocument.createEvent("CustomEvent");
      evt.initCustomEvent(name, true, true, {});
      notificationFrame.contentDocument.documentElement.dispatchEvent(evt);
    }

    panel.addEventListener("popuphidden", function onpopuphiding() {
      panel.removeEventListener("popuphidden", onpopuphiding);
      SocialToolbar.button.removeAttribute("open");
      notificationFrame.docShell.isActive = false;
      dispatchPanelEvent("socialFrameHide");
    });

    panel.addEventListener("popupshown", function onpopupshown() {
      panel.removeEventListener("popupshown", onpopupshown);
      SocialToolbar.button.setAttribute("open", "true");
      notificationFrame.docShell.isActive = true;
      notificationFrame.docShell.isAppTab = true;
      if (notificationFrame.contentDocument.readyState == "complete") {
        sizeSocialPanelToContent(notificationFrame);
        dispatchPanelEvent("socialFrameShow");
      } else {
        
        notificationFrame.addEventListener("load", function panelBrowserOnload(e) {
          notificationFrame.removeEventListener("load", panelBrowserOnload, true);
          sizeSocialPanelToContent(notificationFrame);
          setTimeout(function() {
            dispatchPanelEvent("socialFrameShow");
          }, 0);
        }, true);
      }
    });

    panel.openPopup(iconImage, "bottomcenter topleft", 0, 0, false, false);
  }
}

var SocialSidebar = {
  
  init: function SocialSidebar_init() {
    let sbrowser = document.getElementById("social-sidebar-browser");
    
    sbrowser.docShell.isAppTab = true;
  
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

  dispatchEvent: function(aType, aDetail) {
    let sbrowser = document.getElementById("social-sidebar-browser");
    let evt = sbrowser.contentDocument.createEvent("CustomEvent");
    evt.initCustomEvent(aType, true, true, aDetail ? aDetail : {});
    sbrowser.contentDocument.documentElement.dispatchEvent(evt);
  },

  updateSidebar: function SocialSidebar_updateSidebar() {
    
    let command = document.getElementById("Social:ToggleSidebar");
    command.hidden = !this.canShow;

    
    
    let hideSidebar = !this.canShow || !this.enabled;
    let broadcaster = document.getElementById("socialSidebarBroadcaster");
    broadcaster.hidden = hideSidebar;
    command.setAttribute("checked", !hideSidebar);

    let sbrowser = document.getElementById("social-sidebar-browser");
    sbrowser.docShell.isActive = !hideSidebar;
    if (hideSidebar) {
      this.dispatchEvent("socialFrameHide");
      
      if (!this.canShow) {
        sbrowser.removeAttribute("origin");
        sbrowser.setAttribute("src", "about:blank");
      }
    } else {
      
      if (sbrowser.getAttribute("origin") != Social.provider.origin) {
        sbrowser.setAttribute("origin", Social.provider.origin);
        sbrowser.setAttribute("src", Social.provider.sidebarURL);
        sbrowser.addEventListener("load", function sidebarOnShow() {
          sbrowser.removeEventListener("load", sidebarOnShow);
          
          setTimeout(function () {
            SocialSidebar.dispatchEvent("socialFrameShow");
          }, 0);
        });
      } else {
        this.dispatchEvent("socialFrameShow");
      }
    }
  }
}
