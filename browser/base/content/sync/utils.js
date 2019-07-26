




let gSyncUtils = {
  
  _openLink: function (url) {
    let thisDocEl = document.documentElement,
        openerDocEl = window.opener && window.opener.document.documentElement;
    if (thisDocEl.id == "BrowserPreferences" && !thisDocEl.instantApply) {
      openUILinkIn(url, "window");
    } else {
      openUILinkIn(url, "tab");
    }
  },

  changeName: function changeName(input) {
    
    Weave.Service.clientsEngine.localName = input.value;
    input.value = Weave.Service.clientsEngine.localName;
  },

  openToS: function () {
    this._openLink(Weave.Svc.Prefs.get("termsURL"));
  },

  openPrivacyPolicy: function () {
    this._openLink(Weave.Svc.Prefs.get("privacyURL"));
  },

  openAccountsPage: function () {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    win.switchToTabHavingURI("about:accounts", true);
  }
};
