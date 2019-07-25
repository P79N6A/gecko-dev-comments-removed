








































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

  


  generatePassphrase: function() {
    let rng = Cc["@mozilla.org/security/random-generator;1"]
                .createInstance(Ci.nsIRandomGenerator);
    let bytes = rng.generateRandomBytes(20);
    return [String.fromCharCode(97 + Math.floor(byte * 26 / 256))
            for each (byte in bytes)].join("");
  },

  


  hyphenatePassphrase: function(passphrase) {
    return passphrase.slice(0, 5) + '-'
         + passphrase.slice(5, 10) + '-'
         + passphrase.slice(10, 15) + '-'
         + passphrase.slice(15, 20);
  },

  


  normalizePassphrase: function(pp) {
    if (pp.length == 23 && pp[5] == '-' && pp[11] == '-' && pp[17] == '-')
      return pp.slice(0, 5) + pp.slice(6, 11)
           + pp.slice(12, 17) + pp.slice(18, 23);
    return pp;
  },

  




  passphraseEmail: function(elid) {
    let pp = document.getElementById(elid).value;
    let subject = this.bundle.GetStringFromName("email.synckey.subject");
    let body = this.bundle.formatStringFromName("email.synckey.body", [pp], 1);
    let uri = Weave.Utils.makeURI("mailto:?subject=" + subject + "&body=" + body);
    let protoSvc = Cc["@mozilla.org/uriloader/external-protocol-service;1"]
                     .getService(Ci.nsIExternalProtocolService);
    protoSvc.loadURI(uri);
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
    this._preparePPiframe(elid, function(iframe) {
      let filepicker = Cc["@mozilla.org/filepicker;1"]
                         .createInstance(Ci.nsIFilePicker);
      filepicker.init(window, dialogTitle, Ci.nsIFilePicker.modeSave);
      filepicker.appendFilters(Ci.nsIFilePicker.filterHTML);
      filepicker.defaultString = "Firefox Sync Key.html";
      let rv = filepicker.show();
      if (rv == Ci.nsIFilePicker.returnOK
          || rv == Ci.nsIFilePicker.returnReplace) {
        let stream = Cc["@mozilla.org/network/file-output-stream;1"]
                       .createInstance(Ci.nsIFileOutputStream);
        stream.init(filepicker.file, -1, -1, 0);

        let serializer = new XMLSerializer();
        let output = serializer.serializeToString(iframe.contentDocument);
        output = Weave.Utils.encodeUTF8(output);
        stream.write(output, output.length);
      }
      return false;
    });
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

