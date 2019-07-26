# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

XPCOMUtils.defineLazyGetter(this, "FxAccountsCommon", function () {
  return Cu.import("resource://gre/modules/FxAccountsCommon.js", {});
});

let gFxAccounts = {

  _initialized: false,
  _originalLabel: null,
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
      FxAccountsCommon.ONLOGIN_NOTIFICATION,
      FxAccountsCommon.ONVERIFIED_NOTIFICATION,
      FxAccountsCommon.ONLOGOUT_NOTIFICATION
    ];
  },

  get button() {
    delete this.button;
    return this.button = document.getElementById("PanelUI-fxa-status");
  },

  get syncNeedsCustomization() {
    try {
      return Services.prefs.getBoolPref("services.sync.needsCustomization");
    } catch (e) {
      return false;
    }
  },

  init: function () {
    if (this._initialized) {
      return;
    }

    for (let topic of this.topics) {
      Services.obs.addObserver(this, topic, false);
    }

    gNavToolbox.addEventListener("customizationstarting", this);
    gNavToolbox.addEventListener("customizationending", this);

    
    
    this._originalLabel = this.button.getAttribute("label");
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
    if (topic != FxAccountsCommon.ONVERIFIED_NOTIFICATION) {
      this.updateUI();
    } else if (!this.syncNeedsCustomization) {
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

    
    
    fxAccounts.getSignedInUser().then(userData => {
      if (userData && !this._inCustomizationMode) {
        this.button.setAttribute("signedin", "true");
        this.button.setAttribute("label", userData.email);
        this.button.setAttribute("tooltiptext", userData.email);
      } else {
        this.button.removeAttribute("signedin");
        this.button.setAttribute("label", this._originalLabel);
        this.button.removeAttribute("tooltiptext");
      }
    });
  },

  toggle: function (event) {
    if (event.originalTarget.hasAttribute("signedin")) {
      this.openPreferences();
    } else {
      this.openSignInPage();
    }

    PanelUI.hide();
  },

  openPreferences: function () {
    openPreferences("paneSync");
  },

  openSignInPage: function () {
    switchToTabHavingURI("about:accounts", true);
  }
};
