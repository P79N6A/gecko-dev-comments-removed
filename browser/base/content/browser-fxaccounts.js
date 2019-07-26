# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

XPCOMUtils.defineLazyGetter(this, "FxAccountsCommon", function () {
  return Cu.import("resource://gre/modules/FxAccountsCommon.js", {});
});

const PREF_SYNC_START_DOORHANGER = "services.sync.ui.showSyncStartDoorhanger";

let gFxAccounts = {

  _initialized: false,
  _inCustomizationMode: false,

  get weave() {
    delete this.weave;
    return this.weave = Cc["@mozilla.org/weave/service;1"]
                          .getService(Ci.nsISupports)
                          .wrappedJSObject;
  },

  get topics() {
    
    delete this.topics;
    return this.topics = [
      "weave:service:ready",
      "weave:service:sync:start",
      "weave:service:login:error",
      FxAccountsCommon.ONLOGIN_NOTIFICATION,
      FxAccountsCommon.ONVERIFIED_NOTIFICATION,
      FxAccountsCommon.ONLOGOUT_NOTIFICATION
    ];
  },

  
  get activeWindowTopics() {
    
    delete this.activeWindowTopics;
    return this.activeWindowTopics = new Set([
      "weave:service:sync:start",
      FxAccountsCommon.ONVERIFIED_NOTIFICATION
    ]);
  },

  get button() {
    delete this.button;
    return this.button = document.getElementById("PanelUI-fxa-status");
  },

  get loginFailed() {
    
    
    let service = Cc["@mozilla.org/weave/service;1"]
                  .getService(Components.interfaces.nsISupports)
                  .wrappedJSObject;
    if (!service.ready) {
      return false;
    }
    
    
    
    return Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED;
  },

  get isActiveWindow() {
    let mostRecentNonPopupWindow =
      RecentWindow.getMostRecentBrowserWindow({allowPopups: false});
    return window == mostRecentNonPopupWindow;
  },

  init: function () {
    
    if (this._initialized || !window.toolbar.visible) {
      return;
    }

    for (let topic of this.topics) {
      Services.obs.addObserver(this, topic, false);
    }

    gNavToolbox.addEventListener("customizationstarting", this);
    gNavToolbox.addEventListener("customizationending", this);

    this._initialized = true;

    this.updateUI();
  },

  uninit: function () {
    if (!this._initialized) {
      return;
    }

    for (let topic of this.topics) {
      Services.obs.removeObserver(this, topic);
    }

    this._initialized = false;
  },

  observe: function (subject, topic) {
    
    if (this.activeWindowTopics.has(topic) && !this.isActiveWindow) {
      return;
    }

    switch (topic) {
      case FxAccountsCommon.ONVERIFIED_NOTIFICATION:
        Services.prefs.setBoolPref(PREF_SYNC_START_DOORHANGER, true);
        break;
      case "weave:service:sync:start":
        this.onSyncStart();
        break;
      default:
        this.updateUI();
        break;
    }
  },

  onSyncStart: function () {
    let showDoorhanger = false;

    try {
      showDoorhanger = Services.prefs.getBoolPref(PREF_SYNC_START_DOORHANGER);
    } catch (e) {  }

    if (showDoorhanger) {
      Services.prefs.clearUserPref(PREF_SYNC_START_DOORHANGER);
      this.showSyncStartedDoorhanger();
    }
  },

  handleEvent: function (event) {
    this._inCustomizationMode = event.type == "customizationstarting";
    this.updateUI();
  },

  showDoorhanger: function (id) {
    let panel = document.getElementById(id);
    let anchor = document.getElementById("PanelUI-menu-button");

    let iconAnchor =
      document.getAnonymousElementByAttribute(anchor, "class",
                                              "toolbarbutton-icon");

    panel.hidden = false;
    panel.openPopup(iconAnchor || anchor, "bottomcenter topright");
  },

  showSyncStartedDoorhanger: function () {
    this.showDoorhanger("sync-start-panel");
  },

  showSyncFailedDoorhanger: function () {
    this.showDoorhanger("sync-error-panel");
  },

  updateUI: function () {
    
    if (!this.weave.fxAccountsEnabled) {
      return;
    }

    
    this.button.removeAttribute("hidden");

    
    if (this._inCustomizationMode) {
      this.button.setAttribute("disabled", "true");
    } else {
      this.button.removeAttribute("disabled");
    }

    let defaultLabel = this.button.getAttribute("defaultlabel");
    let errorLabel = this.button.getAttribute("errorlabel");

    
    
    fxAccounts.getSignedInUser().then(userData => {
      
      this.button.setAttribute("label", defaultLabel);
      this.button.removeAttribute("tooltiptext");
      this.button.removeAttribute("signedin");
      this.button.removeAttribute("failed");

      if (!this._inCustomizationMode) {
        if (this.loginFailed) {
          this.button.setAttribute("failed", "true");
          this.button.setAttribute("label", errorLabel);
        } else if (userData) {
          this.button.setAttribute("signedin", "true");
          this.button.setAttribute("label", userData.email);
          this.button.setAttribute("tooltiptext", userData.email);
        }
      }
    });
  },

  onMenuPanelCommand: function (event) {
    let button = event.originalTarget;

    if (button.hasAttribute("signedin")) {
      this.openPreferences();
    } else if (button.hasAttribute("failed")) {
      this.openSignInAgainPage();
    } else {
      this.openAccountsPage();
    }

    PanelUI.hide();
  },

  openPreferences: function () {
    openPreferences("paneSync");
  },

  openAccountsPage: function () {
    switchToTabHavingURI("about:accounts", true);
  },

  openSignInAgainPage: function () {
    switchToTabHavingURI("about:accounts?action=reauth", true);
  }
};
