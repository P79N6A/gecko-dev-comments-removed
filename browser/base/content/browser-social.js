



let SocialUI = {
  
  init: function SocialUI_init() {
    Services.obs.addObserver(this, "social:pref-changed", false);
    Services.obs.addObserver(this, "social:ambient-notification-changed", false);
    Services.obs.addObserver(this, "social:profile-changed", false);

    Social.init(this._providerReady.bind(this));
  },

  
  uninit: function SocialUI_uninit() {
    Services.obs.removeObserver(this, "social:pref-changed");
    Services.obs.removeObserver(this, "social:ambient-notification-changed");
    Services.obs.removeObserver(this, "social:profile-changed");
  },

  showProfile: function SocialUI_showProfile() {
    if (Social.provider)
      openUILink(Social.provider.profile.profileURL);
  },

  observe: function SocialUI_observe(subject, topic, data) {
    switch (topic) {
      case "social:pref-changed":
        SocialShareButton.updateButtonHiddenState();
        SocialToolbar.updateButtonHiddenState();
        break;
      case "social:ambient-notification-changed":
        SocialToolbar.updateButton();
        break;
      case "social:profile-changed":
        SocialToolbar.updateProfile();
        break;
    }
  },

  
  _providerReady: function SocialUI_providerReady() {
    SocialToolbar.init();
    SocialShareButton.init();
  }
}

let SocialShareButton = {
  init: function SSB_init() {
    this.sharePopup.hidden = false;
    this.updateButtonHiddenState();
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
    
    
    document.getElementById("social-statusarea-popup").addEventListener("popupshowing", function(e) {
      document.getElementById("social-toolbar-button").setAttribute("open", "true");
    }, false);
    document.getElementById("social-statusarea-popup").addEventListener("popuphiding", function(e) {
      document.getElementById("social-toolbar-button").removeAttribute("open");
    }, false);

    this.updateButton();
    this.updateProfile();
  },

  updateButtonHiddenState: function SocialToolbar_updateButtonHiddenState() {
    let toolbarbutton = document.getElementById("social-toolbar-button");
    toolbarbutton.hidden = !Social.uiVisible;
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
      
      
      let body = doc.getElementById("notif") || doc.body.firstChild;
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
      
      document.getElementById("social-toolbar-button").removeAttribute("open");
      notifBrowser.setAttribute("src", "about:blank");
    });

    notifBrowser.service = Social.provider;
    notifBrowser.setAttribute("src", iconImage.getAttribute("contentPanel"));
    document.getElementById("social-toolbar-button").setAttribute("open", "true");
    panel.openPopup(iconImage, "bottomcenter topleft", 0, 0, false, false);
  }
}
