







































let gSyncUtils = {
  
  _openLink: function (url) {
    let thisDocEl = document.documentElement,
        openerDocEl = window.opener && window.opener.document.documentElement;
    if (thisDocEl.id == "accountSetup" && window.opener &&
        openerDocEl.id == "BrowserPreferences" && !openerDocEl.instantApply)
      openUILinkIn(url, "window");
    else if (thisDocEl.id == "BrowserPreferences" && !thisDocEl.instantApply)
      openUILinkIn(url, "window");
    else
      openUILinkIn(url, "tab");
  },

  changeName: function changeName(input) {
    
    Weave.Clients.localName = input.value;
    input.value = Weave.Clients.localName;
  },

  openChange: function openChange(type) {
    
    let openedDialog = Weave.Svc.WinMediator.getMostRecentWindow("Sync:" + type);
    if (openedDialog != null) {
      openedDialog.focus();
      return;
    }

    
    let changeXUL = "chrome://browser/content/syncGenericChange.xul";
    let changeOpt = "centerscreen,chrome,dialog,modal,resizable=no";
    Weave.Svc.WinWatcher.activeWindow.openDialog(changeXUL, "", changeOpt, type);
  },

  changePassword: function () {
    this.openChange("ChangePassword");
  },

  resetPassphrase: function () {
    this.openChange("ResetPassphrase");
  },

  updatePassphrase: function () {
    this.openChange("UpdatePassphrase");
  },

  resetPassword: function () {
    this._openLink(Weave.Service.pwResetURL);
  },

  openToS: function () {
    this._openLink(Weave.Svc.Prefs.get("termsURL"));
  },

  openPrivacyPolicy: function () {
    this._openLink(Weave.Svc.Prefs.get("privacyURL"));
  },

  
  _baseURL: "http://www.mozilla.com/firefox/sync/",

  openFirstClientFirstrun: function () {
    let url = this._baseURL + "firstrun.html";
    this._openLink(url);
  },

  openAddedClientFirstrun: function () {
    let url = this._baseURL + "secondrun.html";
    this._openLink(url);
  },

  








  validatePassword: function (el1, el2) {
    return this._validate(el1, el2, true);
  },

  validatePassphrase: function (el1, el2) {
    return this._validate(el1, el2, false);
  },

  _validate: function (el1, el2, isPassword) {
    let valid = false;
    let val1 = el1.value;
    let val2 = el2 ? el2.value : "";
    let error = "";

    if (isPassword) {
      if (!el2)
        valid = val1.length >= Weave.MIN_PASS_LENGTH;
      else if (val1 && val1 == Weave.Service.username)
        error = "change.password.pwSameAsUsername";
      else if (val1 && val1 == Weave.Service.password)
        error = "change.password.pwSameAsPassword";
      else if (val1 && val1 == Weave.Service.passphrase)
        error = "change.password.pwSameAsPassphrase";
      else if (val1 && val2) {
        if (val1 == val2 && val1.length >= Weave.MIN_PASS_LENGTH)
          valid = true;
        else if (val1.length < Weave.MIN_PASS_LENGTH)
          error = "change.password.tooShort";
        else if (val1 != val2)
          error = "change.password.mismatch";
      }
    }
    else {
      if (!el2)
        valid = val1.length >= Weave.MIN_PP_LENGTH;
      else if (val1 == Weave.Service.username)
        error = "change.passphrase.ppSameAsUsername";
      else if (val1 == Weave.Service.password)
        error = "change.passphrase.ppSameAsPassword";
      else if (val1 == Weave.Service.passphrase)
        error = "change.passphrase.ppSameAsPassphrase";
      else if (val1 && val2) {
        if (val1 == val2 && val1.length >= Weave.MIN_PP_LENGTH)
          valid = true;
        else if (val1.length < Weave.MIN_PP_LENGTH)
          error = "change.passphrase.tooShort";
        else if (val1 != val2)
          error = "change.passphrase.mismatch";
      }
    }
    let errorString = error ? Weave.Utils.getErrorString(error) : "";
    dump("valid: " + valid + " error: " + errorString + "\n");
    return [valid, errorString];
  }
}

