








































let gSyncUtils = {
  get bundle() {
    delete this.bundle;
    return this.bundle = Services.strings.createBundle("chrome://browser/locale/syncSetup.properties");
  },

  
  _openLink: function (url) {
    let thisDocEl = document.documentElement,
        openerDocEl = window.opener && window.opener.document.documentElement;
    if (thisDocEl.id == "accountSetup" && window.opener &&
        openerDocEl.id == "BrowserPreferences" && !openerDocEl.instantApply)
      openUILinkIn(url, "window");
    else if (thisDocEl.id == "BrowserPreferences" && !thisDocEl.instantApply)
      openUILinkIn(url, "window");
    else if (document.documentElement.id == "change-dialog")
      Services.wm.getMostRecentWindow("navigator:browser")
              .openUILinkIn(url, "tab");
    else
      openUILinkIn(url, "tab");
  },

  changeName: function changeName(input) {
    
    Weave.Clients.localName = input.value;
    input.value = Weave.Clients.localName;
  },

  openChange: function openChange(type, duringSetup) {
    
    let openedDialog = Services.wm.getMostRecentWindow("Sync:" + type);
    if (openedDialog != null) {
      openedDialog.focus();
      return;
    }

    
    let changeXUL = "chrome://browser/content/syncGenericChange.xul";
    let changeOpt = "centerscreen,chrome,resizable=no";
    Services.ww.activeWindow.openDialog(changeXUL, "", changeOpt,
                                        type, duringSetup);
  },

  changePassword: function () {
    if (Weave.Utils.ensureMPUnlocked())
      this.openChange("ChangePassword");
  },

  resetPassphrase: function (duringSetup) {
    if (Weave.Utils.ensureMPUnlocked())
      this.openChange("ResetPassphrase", duringSetup);
  },

  updatePassphrase: function () {
    if (Weave.Utils.ensureMPUnlocked())
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

  






  _preparePPiframe: function(elid, callback) {
    let pp = document.getElementById(elid).value;

    
    let iframe = document.createElement("iframe");
    iframe.setAttribute("src", "chrome://browser/content/syncKey.xhtml");
    iframe.collapsed = true;
    document.documentElement.appendChild(iframe);
    iframe.contentWindow.addEventListener("load", function() {
      iframe.contentWindow.removeEventListener("load", arguments.callee, false);

      
      let el = iframe.contentDocument.getElementById("synckey");
      el.firstChild.nodeValue = pp;

      
      let termsURL = Weave.Svc.Prefs.get("termsURL");
      el = iframe.contentDocument.getElementById("tosLink");
      el.setAttribute("href", termsURL);
      el.firstChild.nodeValue = termsURL;

      let privacyURL = Weave.Svc.Prefs.get("privacyURL");
      el = iframe.contentDocument.getElementById("ppLink");
      el.setAttribute("href", privacyURL);
      el.firstChild.nodeValue = privacyURL;

      callback(iframe);
    }, false);
  },

  




  passphrasePrint: function(elid) {
    this._preparePPiframe(elid, function(iframe) {
      let webBrowserPrint = iframe.contentWindow
                                  .QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIWebBrowserPrint);
      let printSettings = PrintUtils.getPrintSettings();

      
      printSettings.headerStrLeft
        = printSettings.headerStrCenter
        = printSettings.headerStrRight
        = printSettings.footerStrLeft
        = printSettings.footerStrCenter = "";
      printSettings.footerStrRight = "&D";

      try {
        webBrowserPrint.print(printSettings, null);
      } catch (ex) {
        
      }
    });
  },

  




  passphraseSave: function(elid) {
    let dialogTitle = this.bundle.GetStringFromName("save.synckey.title");
    let defaultSaveName = this.bundle.GetStringFromName("save.default.label");
    this._preparePPiframe(elid, function(iframe) {
      let filepicker = Cc["@mozilla.org/filepicker;1"]
                         .createInstance(Ci.nsIFilePicker);
      filepicker.init(window, dialogTitle, Ci.nsIFilePicker.modeSave);
      filepicker.appendFilters(Ci.nsIFilePicker.filterHTML);
      filepicker.defaultString = defaultSaveName;
      let rv = filepicker.show();
      if (rv == Ci.nsIFilePicker.returnOK
          || rv == Ci.nsIFilePicker.returnReplace) {
        let stream = Cc["@mozilla.org/network/file-output-stream;1"]
                       .createInstance(Ci.nsIFileOutputStream);
        stream.init(filepicker.file, -1, -1, 0);

        let serializer = new XMLSerializer();
        let output = serializer.serializeToString(iframe.contentDocument);
        output = output.replace(/<!DOCTYPE (.|\n)*?]>/,
          '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" ' +
          '"DTD/xhtml1-strict.dtd">');
        output = Weave.Utils.encodeUTF8(output);
        stream.write(output, output.length);
      }
      return false;
    });
  },

  







  validatePassword: function (el1, el2) {
    let valid = false;
    let val1 = el1.value;
    let val2 = el2 ? el2.value : "";
    let error = "";

    if (!el2)
      valid = val1.length >= Weave.MIN_PASS_LENGTH;
    else if (val1 && val1 == Weave.Service.username)
      error = "change.password.pwSameAsUsername";
    else if (val1 && val1 == Weave.Service.account)
      error = "change.password.pwSameAsEmail";
    else if (val1 && val1 == Weave.Service.password)
      error = "change.password.pwSameAsPassword";
    else if (val1 && val1 == Weave.Service.passphrase)
      error = "change.password.pwSameAsSyncKey";
    else if (val1 && val2) {
      if (val1 == val2 && val1.length >= Weave.MIN_PASS_LENGTH)
        valid = true;
      else if (val1.length < Weave.MIN_PASS_LENGTH)
        error = "change.password.tooShort";
      else if (val1 != val2)
        error = "change.password.mismatch";
    }
    let errorString = error ? Weave.Utils.getErrorString(error) : "";
    return [valid, errorString];
  }
};
