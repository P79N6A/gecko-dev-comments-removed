


const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Prompt",
                                  "resource://gre/modules/Prompt.jsm");

var gPromptService = null;

function PromptService() {
  gPromptService = this;
}

PromptService.prototype = {
  classID: Components.ID("{9a61149b-2276-4a0a-b79c-be994ad106cf}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPromptFactory, Ci.nsIPromptService, Ci.nsIPromptService2]),

  
  
  getPrompt: function getPrompt(domWin, iid) {
    let doc = this.getDocument();
    if (!doc) {
      let fallback = this._getFallbackService();
      return fallback.QueryInterface(Ci.nsIPromptFactory).getPrompt(domWin, iid);
    }

    let p = new InternalPrompt(domWin, doc);
    p.QueryInterface(iid);
    return p;
  },

  

  _getFallbackService: function _getFallbackService() {
    return Components.classesByID["{7ad1b327-6dfa-46ec-9234-f2a620ea7e00}"]
                     .getService(Ci.nsIPromptService);
  },

  getDocument: function getDocument() {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    return win ? win.document : null;
  },

  
  
  callProxy: function(aMethod, aArguments) {
    let prompt;
    let doc = this.getDocument();
    if (!doc) {
      let fallback = this._getFallbackService();
      return fallback[aMethod].apply(fallback, aArguments);
    }
    let domWin = aArguments[0];
    prompt = new InternalPrompt(domWin, doc);
    return prompt[aMethod].apply(prompt, Array.prototype.slice.call(aArguments, 1));
  },

  

  alert: function() {
    return this.callProxy("alert", arguments);
  },
  alertCheck: function() {
    return this.callProxy("alertCheck", arguments);
  },
  confirm: function() {
    return this.callProxy("confirm", arguments);
  },
  confirmCheck: function() {
    return this.callProxy("confirmCheck", arguments);
  },
  confirmEx: function() {
    return this.callProxy("confirmEx", arguments);
  },
  prompt: function() {
    return this.callProxy("prompt", arguments);
  },
  promptUsernameAndPassword: function() {
    return this.callProxy("promptUsernameAndPassword", arguments);
  },
  promptPassword: function() {
    return this.callProxy("promptPassword", arguments);
  },
  select: function() {
    return this.callProxy("select", arguments);
  },

  
  promptAuth: function() {
    return this.callProxy("promptAuth", arguments);
  },
  asyncPromptAuth: function() {
    return this.callProxy("asyncPromptAuth", arguments);
  }
};

function InternalPrompt(aDomWin, aDocument) {
  this._domWin = aDomWin;
  this._doc = aDocument;
}

InternalPrompt.prototype = {
  _domWin: null,
  _doc: null,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt, Ci.nsIAuthPrompt, Ci.nsIAuthPrompt2]),

  
  _getPrompt: function _getPrompt(aTitle, aText, aButtons, aCheckMsg, aCheckState) {
    let p = new Prompt({
      window: this._domWin,
      title: aTitle,
      message: aText,
      buttons: aButtons || [
        PromptUtils.getLocaleString("OK"),
        PromptUtils.getLocaleString("Cancel")
      ]
    });
    return p;
  },

  addCheckbox: function addCheckbox(aPrompt, aCheckMsg, aCheckState) {
    
    
    
    if (aCheckMsg) {
      aPrompt.addCheckbox({
        label: PromptUtils.cleanUpLabel(aCheckMsg),
        checked: aCheckState.value
      });
    }

    return aPrompt;
  },

  addTextbox: function(prompt, value, autofocus, hint) {
    prompt.addTextbox({
      value: (value !== null) ? value : "",
      autofocus: autofocus,
      hint: hint
    });
  },

  addPassword: function(prompt, value, autofocus, hint) {
    prompt.addPassword({
      value: (value !== null) ? value : "",
      autofocus: autofocus,
      hint: hint
    });
  },

  


  showPrompt: function showPrompt(aPrompt) {
    if (this._domWin) {
      PromptUtils.fireDialogEvent(this._domWin, "DOMWillOpenModalDialog");
      let winUtils = this._domWin.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
      winUtils.enterModalState();
    }

    let retval = null;
    aPrompt.show(function(data) {
      retval = data;
    });

    
    let thread = Services.tm.currentThread;
    while (retval == null)
      thread.processNextEvent(true);

    if (this._domWin) {
      let winUtils = this._domWin.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
      winUtils.leaveModalState();
      PromptUtils.fireDialogEvent(this._domWin, "DOMModalDialogClosed");
    }

    return retval;
  },

  









  prompt: function prompt() {
    if (gPromptService.inContentProcess)
      return gPromptService.callProxy("prompt", [null].concat(Array.prototype.slice.call(arguments)));

    
    if (typeof arguments[2] == "object")
      return this.nsIPrompt_prompt.apply(this, arguments);
    else
      return this.nsIAuthPrompt_prompt.apply(this, arguments);
  },

  promptUsernameAndPassword: function promptUsernameAndPassword() {
    
    if (typeof arguments[2] == "object")
      return this.nsIPrompt_promptUsernameAndPassword.apply(this, arguments);
    else
      return this.nsIAuthPrompt_promptUsernameAndPassword.apply(this, arguments);
  },

  promptPassword: function promptPassword() {
    
    if (typeof arguments[2] == "object")
      return this.nsIPrompt_promptPassword.apply(this, arguments);
    else
      return this.nsIAuthPrompt_promptPassword.apply(this, arguments);
  },

  

  alert: function alert(aTitle, aText) {
    let p = this._getPrompt(aTitle, aText, [ PromptUtils.getLocaleString("OK") ]);
    p.setHint("alert");
    this.showPrompt(p);
  },

  alertCheck: function alertCheck(aTitle, aText, aCheckMsg, aCheckState) {
    let p = this._getPrompt(aTitle, aText, [ PromptUtils.getLocaleString("OK") ]);
    this.addCheckbox(p, aCheckMsg, aCheckState);
    let data = this.showPrompt(p);
    if (aCheckState && data.button > -1)
      aCheckState.value = data.checkbox0;
  },

  confirm: function confirm(aTitle, aText) {
    let p = this._getPrompt(aTitle, aText);
    p.setHint("confirm");
    let data = this.showPrompt(p);
    return (data.button == 0);
  },

  confirmCheck: function confirmCheck(aTitle, aText, aCheckMsg, aCheckState) {
    let p = this._getPrompt(aTitle, aText, null);
    this.addCheckbox(p, aCheckMsg, aCheckState);
    let data = this.showPrompt(p);
    let ok = data.button == 0;
    if (aCheckState && data.button > -1)
      aCheckState.value = data.checkbox0;
    return ok;
  },

  confirmEx: function confirmEx(aTitle, aText, aButtonFlags, aButton0,
                      aButton1, aButton2, aCheckMsg, aCheckState) {
    let buttons = [];
    let titles = [aButton0, aButton1, aButton2];
    for (let i = 0; i < 3; i++) {
      let bTitle = null;
      switch (aButtonFlags & 0xff) {
        case Ci.nsIPromptService.BUTTON_TITLE_OK :
          bTitle = PromptUtils.getLocaleString("OK");
          break;
        case Ci.nsIPromptService.BUTTON_TITLE_CANCEL :
          bTitle = PromptUtils.getLocaleString("Cancel");
          break;
        case Ci.nsIPromptService.BUTTON_TITLE_YES :
          bTitle = PromptUtils.getLocaleString("Yes");
          break;
        case Ci.nsIPromptService.BUTTON_TITLE_NO :
          bTitle = PromptUtils.getLocaleString("No");
          break;
        case Ci.nsIPromptService.BUTTON_TITLE_SAVE :
          bTitle = PromptUtils.getLocaleString("Save");
          break;
        case Ci.nsIPromptService.BUTTON_TITLE_DONT_SAVE :
          bTitle = PromptUtils.getLocaleString("DontSave");
          break;
        case Ci.nsIPromptService.BUTTON_TITLE_REVERT :
          bTitle = PromptUtils.getLocaleString("Revert");
          break;
        case Ci.nsIPromptService.BUTTON_TITLE_IS_STRING :
          bTitle = PromptUtils.cleanUpLabel(titles[i]);
        break;
      }

      if (bTitle)
        buttons.push(bTitle);

      aButtonFlags >>= 8;
    }

    let p = this._getPrompt(aTitle, aText, buttons);
    this.addCheckbox(p, aCheckMsg, aCheckState);
    let data = this.showPrompt(p);
    if (aCheckState && data.button > -1)
      aCheckState.value = data.checkbox0;
    return data.button;
  },

  nsIPrompt_prompt: function nsIPrompt_prompt(aTitle, aText, aValue, aCheckMsg, aCheckState) {
    let p = this._getPrompt(aTitle, aText, null, aCheckMsg, aCheckState);
    p.setHint("prompt");
    this.addTextbox(p, aValue.value, true);
    this.addCheckbox(p, aCheckMsg, aCheckState);
    let data = this.showPrompt(p);

    let ok = data.button == 0;
    if (aCheckState && data.button > -1)
      aCheckState.value = data.checkbox0;
    if (ok)
      aValue.value = data.textbox0;
    return ok;
  },

  nsIPrompt_promptPassword: function nsIPrompt_promptPassword(
      aTitle, aText, aPassword, aCheckMsg, aCheckState) {
    let p = this._getPrompt(aTitle, aText, null);
    this.addPassword(p, aPassword.value, true, PromptUtils.getLocaleString("password", "passwdmgr"));
    this.addCheckbox(p, aCheckMsg, aCheckState);
    let data = this.showPrompt(p);

    let ok = data.button == 0;
    if (aCheckState && data.button > -1)
      aCheckState.value = data.checkbox0;
    if (ok)
      aPassword.value = data.password0;
    return ok;
  },

  nsIPrompt_promptUsernameAndPassword: function nsIPrompt_promptUsernameAndPassword(
      aTitle, aText, aUsername, aPassword, aCheckMsg, aCheckState) {
    let p = this._getPrompt(aTitle, aText, null);
    this.addTextbox(p, aUsername.value, true, PromptUtils.getLocaleString("username", "passwdmgr"));
    this.addPassword(p, aPassword.value, false, PromptUtils.getLocaleString("password", "passwdmgr"));
    this.addCheckbox(p, aCheckMsg, aCheckState);
    let data = this.showPrompt(p);

    let ok = data.button == 0;
    if (aCheckState && data.button > -1)
      aCheckState.value = data.checkbox0;

    if (ok) {
      aUsername.value = data.textbox0;
      aPassword.value = data.password0;
    }
    return ok;
  },

  select: function select(aTitle, aText, aCount, aSelectList, aOutSelection) {
    let p = this._getPrompt(aTitle, aText, [ PromptUtils.getLocaleString("OK") ]);
    p.addMenulist({ values: aSelectList });
    let data = this.showPrompt(p);

    let ok = data.button == 0;
    if (ok)
      aOutSelection.value = data.menulist0;

    return ok;
  },

  

  nsIAuthPrompt_prompt : function (title, text, passwordRealm, savePassword, defaultText, result) {
    
    if (defaultText)
      result.value = defaultText;
    return this.nsIPrompt_prompt(title, text, result, null, {});
  },

  nsIAuthPrompt_promptUsernameAndPassword : function (aTitle, aText, aPasswordRealm, aSavePassword, aUser, aPass) {
    return nsIAuthPrompt_loginPrompt(aTitle, aText, aPasswordRealm, aSavePassword, aUser, aPass);
  },

  nsIAuthPrompt_promptPassword : function (aTitle, aText, aPasswordRealm, aSavePassword, aPass) {
    return nsIAuthPrompt_loginPrompt(aTitle, aText, aPasswordRealm, aSavePassword, null, aPass);
  },

  nsIAuthPrompt_loginPrompt: function(aTitle, aPasswordRealm, aSavePassword, aUser, aPass) {
    let checkMsg = null;
    let check = { value: false };
    let hostname, realm;
    [hostname, realm, aUser] = PromptUtils.getHostnameAndRealm(aPasswordRealm);

    let canSave = PromptUtils.canSaveLogin(hostname, aSavePassword);
    if (canSave) {
      
      let foundLogins = PromptUtils.pwmgr.findLogins({}, hostname, null, realm);
      [checkMsg, check] = PromptUtils.getUsernameAndPassword(foundLogins, aUser, aPass);
    }

    let ok = false;
    if (aUser)
      ok = this.nsIPrompt_promptUsernameAndPassword(aTitle, aText, aUser, aPass, checkMsg, check);
    else
      ok = this.nsIPrompt_promptPassword(aTitle, aText, aPass, checkMsg, check);

    if (ok && canSave && check.value)
      PromptUtils.savePassword(hostname, realm, aUser, aPass);

    return ok;
  },

  

  promptAuth: function promptAuth(aChannel, aLevel, aAuthInfo) {
    let checkMsg = null;
    let check = { value: false };
    let message = PromptUtils.makeDialogText(aChannel, aAuthInfo);
    let [username, password] = PromptUtils.getAuthInfo(aAuthInfo);
    let [hostname, httpRealm] = PromptUtils.getAuthTarget(aChannel, aAuthInfo);
    let foundLogins = PromptUtils.pwmgr.findLogins({}, hostname, null, httpRealm);

    let canSave = PromptUtils.canSaveLogin(hostname, null);
    if (canSave)
      [checkMsg, check] = PromptUtils.getUsernameAndPassword(foundLogins, username, password);

    if (username.value && password.value) {
      PromptUtils.setAuthInfo(aAuthInfo, username.value, password.value);
    }

    let canAutologin = false;
    if (aAuthInfo.flags & Ci.nsIAuthInformation.AUTH_PROXY &&
        !(aAuthInfo.flags & Ci.nsIAuthInformation.PREVIOUS_FAILED) &&
        Services.prefs.getBoolPref("signon.autologin.proxy"))
      canAutologin = true;

    let ok = canAutologin;
    if (!ok && aAuthInfo.flags & Ci.nsIAuthInformation.ONLY_PASSWORD)
      ok = this.nsIPrompt_promptPassword(null, message, password, checkMsg, check);
    else if (!ok)
      ok = this.nsIPrompt_promptUsernameAndPassword(null, message, username, password, checkMsg, check);

    PromptUtils.setAuthInfo(aAuthInfo, username.value, password.value);

    if (ok && canSave && check.value)
      PromptUtils.savePassword(foundLogins, username, password, hostname, httpRealm);

    return ok;
  },

  _asyncPrompts: {},
  _asyncPromptInProgress: false,

  _doAsyncPrompt : function() {
    if (this._asyncPromptInProgress)
      return;

    
    let hashKey = null;
    for (hashKey in this._asyncPrompts)
      break;

    if (!hashKey)
      return;

    
    
    let prompt = this._asyncPrompts[hashKey];
    let prompter = prompt.prompter;
    let [hostname, httpRealm] = PromptUtils.getAuthTarget(prompt.channel, prompt.authInfo);
    let foundLogins = PromptUtils.pwmgr.findLogins({}, hostname, null, httpRealm);
    if (foundLogins.length > 0 && PromptUtils.pwmgr.uiBusy)
      return;

    this._asyncPromptInProgress = true;
    prompt.inProgress = true;

    let self = this;

    let runnable = {
      run: function() {
        let ok = false;
        try {
          ok = prompter.promptAuth(prompt.channel, prompt.level, prompt.authInfo);
        } catch (e) {
          Cu.reportError("_doAsyncPrompt:run: " + e + "\n");
        }

        delete self._asyncPrompts[hashKey];
        prompt.inProgress = false;
        self._asyncPromptInProgress = false;

        for (let consumer of prompt.consumers) {
          if (!consumer.callback)
            
            
            continue;

          try {
            if (ok)
              consumer.callback.onAuthAvailable(consumer.context, prompt.authInfo);
            else
              consumer.callback.onAuthCancelled(consumer.context, true);
          } catch (e) {  }
        }
        self._doAsyncPrompt();
      }
    }

    Services.tm.mainThread.dispatch(runnable, Ci.nsIThread.DISPATCH_NORMAL);
  },

  asyncPromptAuth: function asyncPromptAuth(aChannel, aCallback, aContext, aLevel, aAuthInfo) {
    let cancelable = null;
    try {
      
      
      
      

      cancelable = {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsICancelable]),
        callback: aCallback,
        context: aContext,
        cancel: function() {
          this.callback.onAuthCancelled(this.context, false);
          this.callback = null;
          this.context = null;
        }
      };
      let [hostname, httpRealm] = PromptUtils.getAuthTarget(aChannel, aAuthInfo);
      let hashKey = aLevel + "|" + hostname + "|" + httpRealm;
      let asyncPrompt = this._asyncPrompts[hashKey];
      if (asyncPrompt) {
        asyncPrompt.consumers.push(cancelable);
        return cancelable;
      }

      asyncPrompt = {
        consumers: [cancelable],
        channel: aChannel,
        authInfo: aAuthInfo,
        level: aLevel,
        inProgress : false,
        prompter: this
      }

      this._asyncPrompts[hashKey] = asyncPrompt;
      this._doAsyncPrompt();
    } catch (e) {
      Cu.reportError("PromptService: " + e + "\n");
      throw e;
    }
    return cancelable;
  }
};

let PromptUtils = {
  getLocaleString: function pu_getLocaleString(aKey, aService) {
    if (aService == "passwdmgr")
      return this.cleanUpLabel(this.passwdBundle.GetStringFromName(aKey));

    return this.cleanUpLabel(this.bundle.GetStringFromName(aKey));
  },

  
  
  
  cleanUpLabel: function cleanUpLabel(aLabel) {
    
    
    
    
    

    
    
    if (!aLabel)
      return "";

    if (/ *\(\&([^&])\)(:?)$/.test(aLabel)) {
      aLabel = RegExp.leftContext + RegExp.$2;
    } else if (/^([^&]*)\&(([^&]).*$)/.test(aLabel)) {
      aLabel = RegExp.$1 + RegExp.$2;
    }

    
    aLabel = aLabel.replace(/\&\&/g, "&");

    return aLabel;
  },

  get pwmgr() {
    delete this.pwmgr;
    return this.pwmgr = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
  },

  getHostnameAndRealm: function pu_getHostnameAndRealm(aRealmString) {
    let httpRealm = /^.+ \(.+\)$/;
    if (httpRealm.test(aRealmString))
      return [null, null, null];

    let uri = Services.io.newURI(aRealmString, null, null);
    let pathname = "";

    if (uri.path != "/")
      pathname = uri.path;

    let formattedHostname = this._getFormattedHostname(uri);
    return [formattedHostname, formattedHostname + pathname, uri.username];
  },

  canSaveLogin: function pu_canSaveLogin(aHostname, aSavePassword) {
    let canSave = !this._inPrivateBrowsing && this.pwmgr.getLoginSavingEnabled(aHostname)
    if (aSavePassword)
      canSave = canSave && (aSavePassword == Ci.nsIAuthPrompt.SAVE_PASSWORD_PERMANENTLY)
    return canSave;
  },

  getUsernameAndPassword: function pu_getUsernameAndPassword(aFoundLogins, aUser, aPass) {
    let checkLabel = null;
    let check = { value: false };
    let selectedLogin;

    checkLabel = this.getLocaleString("saveButton", "passwdmgr");

    
    
    if (aFoundLogins.length > 0) {
      selectedLogin = aFoundLogins[0];

      
      
      
      if (aUser.value)
        selectedLogin = this.findLogin(aFoundLogins, "username", aUser.value);

      if (selectedLogin) {
        check.value = true;
        aUser.value = selectedLogin.username;
        
        if (!aPass.value)
          aPass.value = selectedLogin.password;
      }
    }

    return [checkLabel, check];
  },

  findLogin: function pu_findLogin(aLogins, aName, aValue) {
    for (let i = 0; i < aLogins.length; i++)
      if (aLogins[i][aName] == aValue)
        return aLogins[i];
    return null;
  },

  savePassword: function pu_savePassword(aLogins, aUser, aPass, aHostname, aRealm) {
    let selectedLogin = this.findLogin(aLogins, "username", aUser.value);

    
    
    if (!selectedLogin) {
      
      var newLogin = Cc["@mozilla.org/login-manager/loginInfo;1"].createInstance(Ci.nsILoginInfo);
      newLogin.init(aHostname, null, aRealm, aUser.value, aPass.value, "", "");
      this.pwmgr.addLogin(newLogin);
    } else if (aPass.value != selectedLogin.password) {
      
      this.updateLogin(selectedLogin, aPass.value);
    } else {
      this.updateLogin(selectedLogin);
    }
  },

  updateLogin: function pu_updateLogin(aLogin, aPassword) {
    let now = Date.now();
    let propBag = Cc["@mozilla.org/hash-property-bag;1"].createInstance(Ci.nsIWritablePropertyBag);
    if (aPassword) {
      propBag.setProperty("password", aPassword);
      
      
      
      propBag.setProperty("timePasswordChanged", now);
    }
    propBag.setProperty("timeLastUsed", now);
    propBag.setProperty("timesUsedIncrement", 1);

    this.pwmgr.modifyLogin(aLogin, propBag);
  },

  
  makeDialogText: function pu_makeDialogText(aChannel, aAuthInfo) {
    let isProxy    = (aAuthInfo.flags & Ci.nsIAuthInformation.AUTH_PROXY);
    let isPassOnly = (aAuthInfo.flags & Ci.nsIAuthInformation.ONLY_PASSWORD);

    let username = aAuthInfo.username;
    let [displayHost, realm] = this.getAuthTarget(aChannel, aAuthInfo);

    
    if (!aAuthInfo.realm && !isProxy)
    realm = "";

    
    if (realm.length > 150) {
      realm = realm.substring(0, 150);
      
      realm += this.ellipsis;
    }

    let text;
    if (isProxy)
      text = this.bundle.formatStringFromName("EnterLoginForProxy", [realm, displayHost], 2);
    else if (isPassOnly)
      text = this.bundle.formatStringFromName("EnterPasswordFor", [username, displayHost], 2);
    else if (!realm)
      text = this.bundle.formatStringFromName("EnterUserPasswordFor", [displayHost], 1);
    else
      text = this.bundle.formatStringFromName("EnterLoginForRealm", [realm, displayHost], 2);

    return text;
  },

  
  getAuthHostPort: function pu_getAuthHostPort(aChannel, aAuthInfo) {
    let uri = aChannel.URI;
    let res = { host: null, port: -1 };
    if (aAuthInfo.flags & aAuthInfo.AUTH_PROXY) {
      let proxy = aChannel.QueryInterface(Ci.nsIProxiedChannel);
      res.host = proxy.proxyInfo.host;
      res.port = proxy.proxyInfo.port;
    } else {
      res.host = uri.host;
      res.port = uri.port;
    }
    return res;
  },

  getAuthTarget : function pu_getAuthTarget(aChannel, aAuthInfo) {
    let hostname, realm;
    
    
    if (aAuthInfo.flags & Ci.nsIAuthInformation.AUTH_PROXY) {
        if (!(aChannel instanceof Ci.nsIProxiedChannel))
          throw "proxy auth needs nsIProxiedChannel";

      let info = aChannel.proxyInfo;
      if (!info)
        throw "proxy auth needs nsIProxyInfo";

      
      
      let idnService = Cc["@mozilla.org/network/idn-service;1"].getService(Ci.nsIIDNService);
      hostname = "moz-proxy://" + idnService.convertUTF8toACE(info.host) + ":" + info.port;
      realm = aAuthInfo.realm;
      if (!realm)
        realm = hostname;

      return [hostname, realm];
    }
    hostname = this.getFormattedHostname(aChannel.URI);

    
    
    
    realm = aAuthInfo.realm;
    if (!realm)
      realm = hostname;

    return [hostname, realm];
  },

  getAuthInfo : function pu_getAuthInfo(aAuthInfo) {
    let flags = aAuthInfo.flags;
    let username = {value: ""};
    let password = {value: ""};

    if (flags & Ci.nsIAuthInformation.NEED_DOMAIN && aAuthInfo.domain)
      username.value = aAuthInfo.domain + "\\" + aAuthInfo.username;
    else
      username.value = aAuthInfo.username;

    password.value = aAuthInfo.password

    return [username, password];
  },

  setAuthInfo : function (aAuthInfo, username, password) {
    var flags = aAuthInfo.flags;
    if (flags & Ci.nsIAuthInformation.NEED_DOMAIN) {
      
      var idx = username.indexOf("\\");
      if (idx == -1) {
        aAuthInfo.username = username;
      } else {
        aAuthInfo.domain   =  username.substring(0, idx);
        aAuthInfo.username =  username.substring(idx+1);
      }
    } else {
      aAuthInfo.username = username;
    }
    aAuthInfo.password = password;
  },

  getFormattedHostname : function pu_getFormattedHostname(uri) {
    let scheme = uri.scheme;
    let hostname = scheme + "://" + uri.host;

    
    
    port = uri.port;
    if (port != -1) {
      let handler = Services.io.getProtocolHandler(scheme);
      if (port != handler.defaultPort)
        hostname += ":" + port;
    }
    return hostname;
  },

  fireDialogEvent: function(aDomWin, aEventName) {
    
    try {
      if (!aDomWin.document)
        return;
      let event = aDomWin.document.createEvent("Events");
      event.initEvent(aEventName, true, true);
      let winUtils = aDomWin.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      winUtils.dispatchEventToChromeOnly(aDomWin, event);
    } catch(ex) {
    }
  }
};

XPCOMUtils.defineLazyGetter(PromptUtils, "passwdBundle", function () {
  return Services.strings.createBundle("chrome://passwordmgr/locale/passwordmgr.properties");
});

XPCOMUtils.defineLazyGetter(PromptUtils, "bundle", function () {
  return Services.strings.createBundle("chrome://global/locale/commonDialogs.properties");
});

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PromptService]);
