




const PANEL_MIN_HEIGHT = 100;
const PANEL_MIN_WIDTH = 330;

let SocialUI = {
  
  init: function SocialUI_init() {
    Services.obs.addObserver(this, "social:pref-changed", false);
    Services.obs.addObserver(this, "social:ambient-notification-changed", false);
    Services.obs.addObserver(this, "social:profile-changed", false);

    Services.prefs.addObserver("social.sidebar.open", this, false);
    Services.prefs.addObserver("social.toast-notifications.enabled", this, false);

    gBrowser.addEventListener("ActivateSocialFeature", this._activationEventHandler, true, true);

    
    window.addEventListener("mozfullscreenchange", function () SocialSidebar.updateSidebar());

    Social.init(this._providerReady.bind(this));
  },

  
  uninit: function SocialUI_uninit() {
    Services.obs.removeObserver(this, "social:pref-changed");
    Services.obs.removeObserver(this, "social:ambient-notification-changed");
    Services.obs.removeObserver(this, "social:profile-changed");

    Services.prefs.removeObserver("social.sidebar.open", this);
    Services.prefs.removeObserver("social.toast-notifications.enabled", this);
  },

  showProfile: function SocialUI_showProfile() {
    if (this.haveLoggedInUser())
      openUILinkIn(Social.provider.profile.profileURL, "tab");
    else {
      
      openUILinkIn(Social.provider.origin, "tab");
    }
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
        SocialMenu.populate();
        break;
      case "social:profile-changed":
        SocialToolbar.updateProfile();
        SocialShareButton.updateProfileInfo();
        SocialChatBar.update();
        break;
      case "nsPref:changed":
        SocialSidebar.updateSidebar();
        SocialToolbar.updateButton();
        SocialMenu.populate();
        break;
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
    SocialMenu.populate();
  },

  updateToggleCommand: function SocialUI_updateToggleCommand() {
    let toggleCommand = this.toggleCommand;
    
    toggleCommand.setAttribute("checked", Services.prefs.getBoolPref("social.enabled"));
    toggleCommand.setAttribute("hidden", Social.active ? "false" : "true");
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
  },

  haveLoggedInUser: function SocialUI_haveLoggedInUser() {
    return !!(Social.provider && Social.provider.profile && Social.provider.profile.userName);
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
  }
}

let SocialChatBar = {
  get chatbar() {
    return document.getElementById("pinnedchats");
  },
  
  get canShow() {
    if (!SocialUI.haveLoggedInUser())
      return false;
    let docElem = document.documentElement;
    let chromeless = docElem.getAttribute("disablechrome") ||
                     docElem.getAttribute("chromehidden").indexOf("extrachrome") >= 0;
    return Social.uiVisible && !chromeless && !document.mozFullScreen;
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
    if (!Social.provider || panel.firstChild)
      return;
    
    let iframe = document.createElement("iframe");
    iframe.setAttribute("type", "content");
    iframe.setAttribute("class", "social-panel-frame");
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("origin", Social.provider.origin);
    panel.appendChild(iframe);
  },

  setUpProgressListener: function SF_setUpProgressListener() {
    if (!this._progressListenerSet) {
      this._progressListenerSet = true;
      
      
      this.panel.firstChild.clientTop;
      this.panel.firstChild.docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                                    .getInterface(Ci.nsIWebProgress)
                                    .addProgressListener(new SocialErrorListener("flyout"),
                                                         Ci.nsIWebProgress.NOTIFY_STATE_REQUEST |
                                                         Ci.nsIWebProgress.NOTIFY_LOCATION);
    }
  },

  setFlyoutErrorMessage: function SF_setFlyoutErrorMessage() {
    let iframe = this.panel.firstChild;
    if (!iframe)
      return;

    iframe.removeAttribute("src");
    iframe.webNavigation.loadURI("about:socialerror?mode=compactInfo", null, null, null, null);
    sizeSocialPanelToContent(iframe);
  },

  unload: function() {
    let panel = this.panel;
    panel.hidePopup();
    if (!panel.firstChild)
      return
    panel.removeChild(panel.firstChild);
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
          SocialFlyout._dynamicResizer.start(panel, iframe);
          SocialFlyout.dispatchPanelEvent("socialFrameShow");
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
      this.setUpProgressListener();
    }
    this.yOffset = yOffset;
  }
}

let SocialShareButton = {
  
  
  
  promptImages: null,
  promptMessages: null,

  
  init: function SSB_init() {
    this.updateButtonHiddenState();
    this.updateProfileInfo();
  },

  updateProfileInfo: function SSB_updateProfileInfo() {
    let profileRow = document.getElementById("unsharePopupHeader");
    let profile = Social.provider.profile;
    this.promptImages = null;
    this.promptMessages = null;
    if (profile && profile.displayName) {
      profileRow.hidden = false;
      let portrait = document.getElementById("socialUserPortrait");
      portrait.setAttribute("src", profile.portrait || "chrome://global/skin/icons/information-32.png");
      let displayName = document.getElementById("socialUserDisplayName");
      displayName.setAttribute("label", profile.displayName);
    } else {
      profileRow.hidden = true;
      this.updateButtonHiddenState();
      return;
    }
    
    
    
    
    let port = Social.provider.getWorkerPort();
    if (port) {
      port.onmessage = function(evt) {
        if (evt.data.topic == "social.user-recommend-prompt-response") {
          port.close();
          this.acceptRecommendInfo(evt.data.data);
          this.updateButtonHiddenState();
          this.updateShareState();
        }
      }.bind(this);
      port.postMessage({topic: "social.user-recommend-prompt"});
    }
  },

  acceptRecommendInfo: function SSB_acceptRecommendInfo(data) {
    
    let promptImages = {};
    let promptMessages = {};
    function reportError(reason) {
      Cu.reportError("Invalid recommend data from provider: " + reason + ": sharing is disabled for this provider");
      return false;
    }
    if (!data ||
        !data.images || typeof data.images != "object" ||
        !data.messages || typeof data.messages != "object") {
      return reportError("data is missing valid 'images' or 'messages' elements");
    }
    for (let sub of ["share", "unshare"]) {
      let url = data.images[sub];
      if (!url || typeof url != "string" || url.length == 0) {
        return reportError('images["' + sub + '"] is missing or not a non-empty string');
      }
      
      url = Services.io.newURI(Social.provider.origin, null, null).resolve(url);
      let uri = Services.io.newURI(url, null, null);
      if (!uri.schemeIs("http") && !uri.schemeIs("https") && !uri.schemeIs("data")) {
        return reportError('images["' + sub + '"] does not have a valid scheme');
      }
      promptImages[sub] = url;
    }
    for (let sub of ["shareTooltip", "unshareTooltip",
                     "sharedLabel", "unsharedLabel", "unshareLabel",
                     "portraitLabel", 
                     "unshareConfirmLabel", "unshareConfirmAccessKey",
                     "unshareCancelLabel", "unshareCancelAccessKey"]) {
      if (typeof data.messages[sub] != "string" || data.messages[sub].length == 0) {
        return reportError('messages["' + sub + '"] is not a valid string');
      }
      promptMessages[sub] = data.messages[sub];
    }
    this.promptImages = promptImages;
    this.promptMessages = promptMessages;
    return true;
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

  updateButtonHiddenState: function SSB_updateButtonHiddenState() {
    let shareButton = this.shareButton;
    if (shareButton)
      shareButton.hidden = !Social.uiVisible || this.promptImages == null ||
                           !SocialUI.haveLoggedInUser();
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
    updateElement("unsharePopupContinueSharingButton",
                  {label: this.promptMessages.unshareCancelLabel,
                   accesskey: this.promptMessages.unshareCancelAccessKey});
    updateElement("unsharePopupStopSharingButton",
                  {label: this.promptMessages.unshareConfirmLabel,
                  accesskey: this.promptMessages.unshareConfirmAccessKey});
    updateElement("socialUserPortrait",
                  {"aria-label": this.promptMessages.portraitLabel});
    updateElement("socialUserRecommendedText",
                  {value: this.promptMessages.unshareLabel});
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
    let currentPageShared = Social.isPageShared(gBrowser.currentURI);

    
    let status = document.getElementById("share-button-status");
    if (status) {
      
      
      
      
      
      
      let statusString = currentPageShared ?
                           this.promptMessages['sharedLabel'] : "";
      status.setAttribute("value", statusString);
    }

    
    let shareButton = this.shareButton;
    if (!shareButton || shareButton.hidden)
      return;

    let imageURL;
    if (currentPageShared) {
      shareButton.setAttribute("shared", "true");
      shareButton.setAttribute("tooltiptext", this.promptMessages['unshareTooltip']);
      imageURL = this.promptImages["unshare"]
    } else {
      shareButton.removeAttribute("shared");
      shareButton.setAttribute("tooltiptext", this.promptMessages['shareTooltip']);
      imageURL = this.promptImages["share"]
    }
    shareButton.style.backgroundImage = 'url("' + encodeURI(imageURL) + '")';
  }
};

var SocialMenu = {
  populate: function SocialMenu_populate() {
    
    let submenu = document.getElementById("menu_socialAmbientMenuPopup");
    let ambientMenuItems = submenu.getElementsByClassName("ambient-menuitem");
    for (let ambientMenuItem of ambientMenuItems)
      submenu.removeChild(ambientMenuItem);
    let provider = Social.provider;
    if (Social.active && provider) {
      let iconNames = Object.keys(provider.ambientNotificationIcons);
      let separator = document.getElementById("socialAmbientMenuSeparator");
      for (let name of iconNames) {
        let icon = provider.ambientNotificationIcons[name];
        if (!icon.label || !icon.menuURL)
          continue;
        let menuitem = document.createElement("menuitem");
        menuitem.setAttribute("label", icon.label);
        menuitem.classList.add("ambient-menuitem");
        menuitem.addEventListener("command", function() {
          openUILinkIn(icon.menuURL, "tab");
        }, false);
        submenu.insertBefore(menuitem, separator);
      }
      separator.hidden = !iconNames.length;
    }
    document.getElementById("menu_socialAmbientMenu").hidden = !Social.enabled;
  }
};

var SocialToolbar = {
  
  init: function SocialToolbar_init() {
    this.button.setAttribute("image", Social.provider.iconURL);
    this.updateButton();
    this.updateProfile();
    this._dynamicResizer = new DynamicResizeWatcher();
  },

  get button() {
    return document.getElementById("social-provider-button");
  },

  updateButtonHiddenState: function SocialToolbar_updateButtonHiddenState() {
    let tbi = document.getElementById("social-toolbar-item");
    tbi.hidden = !Social.uiVisible;
    if (!SocialUI.haveLoggedInUser()) {
      let parent = document.getElementById("social-notification-panel");
      while (parent.hasChildNodes())
        parent.removeChild(parent.firstChild);

      while (tbi.lastChild != tbi.firstChild)
        tbi.removeChild(tbi.lastChild);
    }
  },

  updateProfile: function SocialToolbar_updateProfile() {
    
    
    
    let profile = Social.provider.profile || {};
    let userPortrait = profile.portrait || "chrome://global/skin/icons/information-32.png";
    document.getElementById("social-statusarea-user-portrait").setAttribute("src", userPortrait);

    let notLoggedInLabel = document.getElementById("social-statusarea-notloggedin");
    let userNameBtn = document.getElementById("social-statusarea-username");
    if (profile.userName) {
      notLoggedInLabel.hidden = true;
      userNameBtn.hidden = false;
      userNameBtn.value = profile.userName;
    } else {
      notLoggedInLabel.hidden = false;
      userNameBtn.hidden = true;
    }
  },

  updateButton: function SocialToolbar_updateButton() {
    this.updateButtonHiddenState();
    let provider = Social.provider;
    let icons = provider.ambientNotificationIcons;
    let iconNames = Object.keys(icons);
    let iconBox = document.getElementById("social-toolbar-item");
    let panel = document.getElementById("social-notification-panel");
    panel.hidden = false;

    let command = document.getElementById("Social:ToggleNotifications");
    command.setAttribute("checked", Services.prefs.getBoolPref("social.toast-notifications.enabled"));

    const CACHE_PREF_NAME = "social.cached.notificationIcons";
    
    
    if (!Social.provider || !Social.provider.enabled ||
        (!SocialUI.haveLoggedInUser() && provider.profile !== undefined)) {
      
      
      
      
      Services.prefs.clearUserPref(CACHE_PREF_NAME);
      return;
    }
    if (Social.provider.profile === undefined) {
      
      
      let cached;
      try {
        cached = JSON.parse(Services.prefs.getCharPref(CACHE_PREF_NAME));
      } catch (ex) {}
      if (cached && cached.provider == Social.provider.origin && cached.data) {
        icons = cached.data;
        iconNames = Object.keys(icons);
        
        for each(let name in iconNames) {
          icons[name].counter = '';
        }
      }
    } else {
      
      
      Services.prefs.setCharPref(CACHE_PREF_NAME,
                                 JSON.stringify({provider: Social.provider.origin,
                                                 data: icons}));
    }

    let notificationFrames = document.createDocumentFragment();
    let iconContainers = document.createDocumentFragment();

    let createdFrames = [];

    for each(let name in iconNames) {
      let icon = icons[name];

      let notificationFrameId = "social-status-" + icon.name;
      let notificationFrame = document.getElementById(notificationFrameId);
      if (!notificationFrame) {
        notificationFrame = document.createElement("iframe");
        notificationFrame.setAttribute("type", "content");
        notificationFrame.setAttribute("class", "social-panel-frame");
        notificationFrame.setAttribute("id", notificationFrameId);
        notificationFrame.setAttribute("mozbrowser", "true");
        
        
        notificationFrame.style.width = PANEL_MIN_WIDTH + "px";

        createdFrames.push(notificationFrame);
        notificationFrames.appendChild(notificationFrame);
      }
      notificationFrame.setAttribute("origin", provider.origin);
      if (notificationFrame.getAttribute("src") != icon.contentPanel)
        notificationFrame.setAttribute("src", icon.contentPanel);

      let iconId = "social-notification-icon-" + icon.name;
      let imageId = iconId + "-image";
      let labelId = iconId + "-label";
      let stackId = iconId + "-stack";
      let stack = document.getElementById(stackId);
      let image, label;
      if (stack) {
        image = document.getElementById(imageId);
        label = document.getElementById(labelId);
      } else {
        let box = document.createElement("box");
        box.classList.add("toolbarbutton-1");
        box.setAttribute("id", iconId);
        
        if (icon.label)
          box.setAttribute("tooltiptext", icon.label);
        box.addEventListener("mousedown", function (e) {
          if (e.button == 0)
            SocialToolbar.showAmbientPopup(box);
        }, false);
        box.setAttribute("notificationFrameId", notificationFrameId);
        stack = document.createElement("stack");
        stack.setAttribute("id", stackId);
        stack.classList.add("social-notification-icon-stack");
        stack.classList.add("toolbarbutton-icon");
        image = document.createElement("image");
        image.setAttribute("id", imageId);
        image.classList.add("social-notification-icon-image");
        image = stack.appendChild(image);
        label = document.createElement("label");
        label.setAttribute("id", labelId);
        label.classList.add("social-notification-icon-label");
        let hbox = document.createElement("hbox");
        hbox.classList.add("social-notification-icon-hbox");
        hbox.setAttribute("align", "start");
        hbox.setAttribute("pack", "end");
        label = hbox.appendChild(label);
        stack.appendChild(hbox);
        box.appendChild(stack);
        iconContainers.appendChild(box);
      }

      let labelValue = icon.counter || "";
      
      if (!label.hasAttribute("value") || label.getAttribute("value") != labelValue)
        label.setAttribute("value", labelValue);

      if (image.getAttribute("src") != icon.iconURL)
        image.setAttribute("src", icon.iconURL);
    }
    panel.appendChild(notificationFrames);
    iconBox.appendChild(iconContainers);

    for (let frame of createdFrames) {
      if (frame.docShell) {
        frame.docShell.isActive = false;
        frame.docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIWebProgress)
                      .addProgressListener(new SocialErrorListener("notification-panel"),
                                           Ci.nsIWebProgress.NOTIFY_STATE_REQUEST |
                                           Ci.nsIWebProgress.NOTIFY_LOCATION);
      }
    }
  },

  showAmbientPopup: function SocialToolbar_showAmbientPopup(aToolbarButtonBox) {
    
    SocialFlyout.panel.hidePopup();

    let panel = document.getElementById("social-notification-panel");
    let notificationFrameId = aToolbarButtonBox.getAttribute("notificationFrameId");
    let notificationFrame = document.getElementById(notificationFrameId);

    
    
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
      aToolbarButtonBox.removeAttribute("open");
      dynamicResizer.stop();
      notificationFrame.docShell.isActive = false;
      dispatchPanelEvent("socialFrameHide");
    });

    panel.addEventListener("popupshown", function onpopupshown() {
      panel.removeEventListener("popupshown", onpopupshown);
      aToolbarButtonBox.setAttribute("open", "true");
      notificationFrame.docShell.isActive = true;
      notificationFrame.docShell.isAppTab = true;
      if (notificationFrame.contentDocument.readyState == "complete") {
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

    let imageId = aToolbarButtonBox.getAttribute("id") + "-image";
    let toolbarButtonImage = document.getElementById(imageId);
    panel.openPopup(toolbarButtonImage, "bottomcenter topleft", 0, 0, false, false);
  },

  setPanelErrorMessage: function SocialToolbar_setPanelErrorMessage(aNotificationFrame) {
    if (!aNotificationFrame)
      return;

    let src = aNotificationFrame.getAttribute("src");
    aNotificationFrame.removeAttribute("src");
    aNotificationFrame.webNavigation.loadURI("about:socialerror?mode=tryAgainOnly&url=" +
                                             encodeURIComponent(src), null, null, null, null);
    sizeSocialPanelToContent(aNotificationFrame);
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
           docElem.getAttribute('chromehidden').contains("toolbar");
  },

  
  get opened() {
    return Services.prefs.getBoolPref("social.sidebar.open") && !document.mozFullScreen;
  },

  dispatchEvent: function(aType, aDetail) {
    let sbrowser = document.getElementById("social-sidebar-browser");
    let evt = sbrowser.contentDocument.createEvent("CustomEvent");
    evt.initCustomEvent(aType, true, true, aDetail ? aDetail : {});
    sbrowser.contentDocument.documentElement.dispatchEvent(evt);
  },

  updateSidebar: function SocialSidebar_updateSidebar() {
    clearTimeout(this._unloadTimeoutId);
    
    let command = document.getElementById("Social:ToggleSidebar");
    command.setAttribute("hidden", this.canShow ? "false" : "true");

    
    
    let hideSidebar = !this.canShow || !this.opened;
    let broadcaster = document.getElementById("socialSidebarBroadcaster");
    broadcaster.hidden = hideSidebar;
    command.setAttribute("checked", !hideSidebar);

    let sbrowser = document.getElementById("social-sidebar-browser");
    sbrowser.docShell.isActive = !hideSidebar;
    if (hideSidebar) {
      this.dispatchEvent("socialFrameHide");
      
      
      
      if (!this.canShow) {
        this.unloadSidebar();
      } else {
        this._unloadTimeoutId = setTimeout(
          this.unloadSidebar,
          Services.prefs.getIntPref("social.sidebar.unload_timeout_ms")
        );
      }
    } else {
      
      if (sbrowser.getAttribute("origin") != Social.provider.origin) {
        sbrowser.setAttribute("origin", Social.provider.origin);
        
        sbrowser.docShell.isAppTab = true;
        sbrowser.webProgress.addProgressListener(new SocialErrorListener("sidebar"),
                                                 Ci.nsIWebProgress.NOTIFY_STATE_REQUEST |
                                                 Ci.nsIWebProgress.NOTIFY_LOCATION);
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
  },

  unloadSidebar: function SocialSidebar_unloadSidebar() {
    let sbrowser = document.getElementById("social-sidebar-browser");
    if (!sbrowser.hasAttribute("origin"))
      return;

    
    
    
    let container = sbrowser.parentNode;
    container.removeChild(sbrowser);
    sbrowser.removeAttribute("origin");
    sbrowser.removeAttribute("src");
    container.appendChild(sbrowser);

    SocialFlyout.unload();
  },

  _unloadTimeoutId: 0,

  setSidebarErrorMessage: function() {
    let sbrowser = document.getElementById("social-sidebar-browser");
    let url = encodeURIComponent(Social.provider.sidebarURL);
    sbrowser.loadURI("about:socialerror?mode=tryAgain&url=" + url, null, null);
  }
}



function SocialErrorListener(aType) {
  this.type = aType;
}

SocialErrorListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports]),

  onStateChange: function SPL_onStateChange(aWebProgress, aRequest, aState, aStatus) {
    let failure = false;
    if ((aState & Ci.nsIWebProgressListener.STATE_STOP)) {
      if (aRequest instanceof Ci.nsIHttpChannel) {
        try {
          
          
          failure = aRequest.responseStatus >= 400 &&
                    aRequest.responseStatus < 600;
        } catch (e) {}
      }
    }

    
    
    if (failure && aStatus != Components.results.NS_BINDING_ABORTED) {
      aRequest.cancel(Components.results.NS_BINDING_ABORTED);
      this.setErrorMessage(aWebProgress);
    }
  },

  onLocationChange: function SPL_onLocationChange(aWebProgress, aRequest, aLocation, aFlags) {
    let failure = aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_ERROR_PAGE;
    if (failure) {
      aRequest.cancel(Components.results.NS_BINDING_ABORTED);
      window.setTimeout(function(self) {
        self.setErrorMessage(aWebProgress);
      }, 0, this);
    }
  },

  onProgressChange: function SPL_onProgressChange() {},
  onStatusChange: function SPL_onStatusChange() {},
  onSecurityChange: function SPL_onSecurityChange() {},

  setErrorMessage: function(aWebProgress) {
    switch (this.type) {
      case "flyout":
        SocialFlyout.setFlyoutErrorMessage();
        break;

      case "sidebar":
        SocialSidebar.setSidebarErrorMessage();
        break;

      case "notification-panel":
        let frame = aWebProgress.QueryInterface(Ci.nsIDocShell)
                                .chromeEventHandler;
        SocialToolbar.setPanelErrorMessage(frame);
        break;
    }
  }
};
