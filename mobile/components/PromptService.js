































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function PromptService() {
}

PromptService.prototype = {
  classID: Components.ID("{9a61149b-2276-4a0a-b79c-be994ad106cf}"),
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPromptFactory, Ci.nsIPromptService, Ci.nsIPromptService2]),

  

  
  getPrompt: function getPrompt(domWin, iid) {
    
    
    if (iid.equals(Ci.nsIAuthPrompt2) || iid.equals(Ci.nsIAuthPrompt)) {
      try {
        let pwmgr = Cc["@mozilla.org/passwordmanager/authpromptfactory;1"].
          getService(Ci.nsIPromptFactory);
        return pwmgr.getPrompt(domWin, iid);
      } catch (e) {
        Cu.reportError("nsPrompter: Delegation to password manager failed: " + e);
      }
    }

    let doc = this.getDocument();
    if (!doc) {
      let fallback = this._getFallbackService();
      return fallback.getPrompt(domWin, iid);
    }
    let p = new Prompt(domWin, doc);
    p.QueryInterface(iid);
    return p;
  },

  
  
  _getFallbackService: function _getFallbackService() {
    return Components.classesByID["{7ad1b327-6dfa-46ec-9234-f2a620ea7e00}"]
                     .getService(Ci.nsIPromptService);
  },

  getDocument: function getDocument() {
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
    let win = wm.getMostRecentWindow("navigator:browser");
    return win ? win.document : null;
  },

  
  
  callProxy: function(aMethod, aArguments) {
    let doc = this.getDocument();
    if (!doc) {
      let fallback = this._getFallbackService();
      return fallback[aMethod].apply(fallback, aArguments);
    }
    let domWin = aArguments[0];
    let p = new Prompt(domWin, doc);
    return p[aMethod].apply(p, Array.prototype.slice.call(aArguments, 1));
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

function Prompt(aDomWin, aDocument) {
  this._domWin = aDomWin;
  this._doc = aDocument;
}

Prompt.prototype = { 
  _domWin: null,
  _doc: null,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt, Ci.nsIAuthPrompt, Ci.nsIAuthPrompt2]),

  
 
  openDialog: function openDialog(aSrc, aParams) {
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
    let browser = wm.getMostRecentWindow("navigator:browser");
    return browser.importDialog(this._domWin, aSrc, aParams);
  },
  
  commonPrompt: function commonPrompt(aTitle, aText, aValue, aCheckMsg, aCheckState, isPassword) {
    var params = new Object();
    params.result = false;
    params.checkbox = aCheckState;
    params.value = aValue;

    let dialog = this.openDialog("chrome://browser/content/prompt/prompt.xul", params);
    let doc = this._doc;
    doc.getElementById("prompt-prompt-title").value = aTitle;
    doc.getElementById("prompt-prompt-message").appendChild(doc.createTextNode(aText));

    doc.getElementById("prompt-prompt-checkbox").checked = aCheckState.value;
    this.setLabelForNode(doc.getElementById("prompt-prompt-checkbox-label"), aCheckMsg);
    doc.getElementById("prompt-prompt-textbox").value = aValue.value;
    if (aCheckMsg)
      doc.getElementById("prompt-prompt-checkbox").removeAttribute("collapsed");

    if (isPassword)
      doc.getElementById("prompt-prompt-textbox").type = "password";

    dialog.waitForClose();
    return params.result;
  },

  
  
  
  setLabelForNode: function setLabelForNode(aNode, aLabel) {
    
    
    
    
    
    
  
    
    
 
    if (!aLabel)
      return;

    var accessKey = null;
    if (/ *\(\&([^&])\)(:)?$/.test(aLabel)) {
      aLabel = RegExp.leftContext + RegExp.$2;
      accessKey = RegExp.$1;
    } else if (/^(.*[^&])?\&(([^&]).*$)/.test(aLabel)) {
      aLabel = RegExp.$1 + RegExp.$2;
      accessKey = RegExp.$3;
    }
  
    
    aLabel = aLabel.replace(/\&\&/g, "&");
    if (aNode instanceof Ci.nsIDOMXULLabelElement) {
      aNode.setAttribute("value", aLabel);
    } else if (aNode instanceof Ci.nsIDOMXULDescriptionElement) {
      let text = aNode.ownerDocument.createTextNode(aLabel);
      aNode.appendChild(text);
    } else {    
      aNode.setAttribute("label", aLabel);
    }
    
    
    
    if (accessKey)
      aNode.setAttribute("accesskey", accessKey);
  },
  
  









  prompt: function prompt() {
    
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
    let dialog = this.openDialog("chrome://browser/content/prompt/alert.xul", null);
    let doc = this._doc;
    doc.getElementById("prompt-alert-title").value = aTitle;
    doc.getElementById("prompt-alert-message").appendChild(doc.createTextNode(aText));
    
    dialog.waitForClose();
  },
  
  alertCheck: function alertCheck(aTitle, aText, aCheckMsg, aCheckState) {
    let dialog = this.openDialog("chrome://browser/content/prompt/alert.xul", aCheckState);
    let doc = this._doc;
    doc.getElementById("prompt-alert-title").value = aTitle;
    doc.getElementById("prompt-alert-message").appendChild(doc.createTextNode(aText));
    
    doc.getElementById("prompt-alert-checkbox").checked = aCheckState.value;
    this.setLabelForNode(doc.getElementById("prompt-alert-checkbox-label"), aCheckMsg);
    doc.getElementById("prompt-alert-checkbox").removeAttribute("collapsed");
    
    dialog.waitForClose();
  },
  
  confirm: function confirm(aTitle, aText) {
    var params = new Object();
    params.result = false;

    let dialog = this.openDialog("chrome://browser/content/prompt/confirm.xul", params);
    let doc = this._doc;
    doc.getElementById("prompt-confirm-title").value = aTitle;
    doc.getElementById("prompt-confirm-message").appendChild(doc.createTextNode(aText));
    
    dialog.waitForClose();
    return params.result;
  },
  
  confirmCheck: function confirmCheck(aTitle, aText, aCheckMsg, aCheckState) {
    var params = new Object();
    params.result = false;
    params.checkbox = aCheckState;

    let dialog = this.openDialog("chrome://browser/content/prompt/confirm.xul", params);
    let doc = this._doc;
    doc.getElementById("prompt-confirm-title").value = aTitle;
    doc.getElementById("prompt-confirm-message").appendChild(doc.createTextNode(aText));

    doc.getElementById("prompt-confirm-checkbox").checked = aCheckState.value;
    this.setLabelForNode(doc.getElementById("prompt-confirm-checkbox-label"), aCheckMsg);
    doc.getElementById("prompt-confirm-checkbox").removeAttribute("collapsed");
    
    dialog.waitForClose();
    return params.result;
  },

  confirmEx: function confirmEx(aTitle, aText, aButtonFlags, aButton0,
                      aButton1, aButton2, aCheckMsg, aCheckState) {
    let numButtons = 0;
    let titles = [aButton0, aButton1, aButton2];

    let defaultButton = 0;
    if (aButtonFlags & Ci.nsIPromptService.BUTTON_POS_1_DEFAULT)
      defaultButton = 1;
    if (aButtonFlags & Ci.nsIPromptService.BUTTON_POS_2_DEFAULT)
      defaultButton = 2;
    
    var params = {
      result: false,
      checkbox: aCheckState,
      defaultButton: defaultButton
    }

    let dialog = this.openDialog("chrome://browser/content/prompt/confirm.xul", params);
    let doc = this._doc;
    doc.getElementById("prompt-confirm-title").value = aTitle;
    doc.getElementById("prompt-confirm-message").appendChild(doc.createTextNode(aText));

    doc.getElementById("prompt-confirm-checkbox").checked = aCheckState.value;
    this.setLabelForNode(doc.getElementById("prompt-confirm-checkbox-label"), aCheckMsg);
    if (aCheckMsg)
      doc.getElementById("prompt-confirm-checkbox").removeAttribute("collapsed");


    let bbox = doc.getElementById("prompt-confirm-buttons-box");
    while (bbox.lastChild)
      bbox.removeChild(bbox.lastChild);

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
          bTitle = titles[i];
        break;
      }
      
      if (bTitle) {
        let button = doc.createElement("button");
        this.setLabelForNode(button, bTitle);
        if (i == defaultButton) {
          button.setAttribute("command", "cmd_ok"); 
        }
        else {
          button.setAttribute("oncommand",
            "document.getElementById('prompt-confirm-dialog').PromptHelper.closeConfirm(" + i + ")");
        }
        bbox.appendChild(button);
      }
      
      aButtonFlags >>= 8;
    }
    
    dialog.waitForClose();
    return params.result;
  },
  
  nsIPrompt_prompt: function nsIPrompt_prompt(aTitle, aText, aValue, aCheckMsg, aCheckState) {
    return this.commonPrompt(aTitle, aText, aValue, aCheckMsg, aCheckState, false);
  },
  
  nsIPrompt_promptPassword: function nsIPrompt_promptPassword(
      aTitle, aText, aPassword, aCheckMsg, aCheckState) {
    return this.commonPrompt(aTitle, aText, aPassword, aCheckMsg, aCheckState, true);
  },
  
  nsIPrompt_promptUsernameAndPassword: function nsIPrompt_promptUsernameAndPassword(
      aTitle, aText, aUsername, aPassword, aCheckMsg, aCheckState) {
    var params = new Object();
    params.result = false;
    params.checkbox = aCheckState;
    params.user = aUsername;
    params.password = aPassword;

    let dialog = this.openDialog("chrome://browser/content/prompt/promptPassword.xul", params);
    let doc = this._doc;
    doc.getElementById("prompt-password-title").value = aTitle;
    doc.getElementById("prompt-password-message").appendChild(doc.createTextNode(aText));
    doc.getElementById("prompt-password-checkbox").checked = aCheckState.value;
    
    doc.getElementById("prompt-password-user").value = aUsername.value;
    doc.getElementById("prompt-password-password").value = aPassword.value;
    if (aCheckMsg) {
      doc.getElementById("prompt-password-checkbox").removeAttribute("collapsed");
      this.setLabelForNode(doc.getElementById("prompt-password-checkbox-label"), aCheckMsg);
    }
    
    dialog.waitForClose();
    return params.result;
  },
  
  select: function select(aTitle, aText, aCount, aSelectList, aOutSelection) {
    var params = new Object();
    params.result = false;
    params.selection = aOutSelection;

    let dialog = this.openDialog("chrome://browser/content/prompt/select.xul", params);
    let doc = this._doc;
    doc.getElementById("prompt-select-title").value = aTitle;
    doc.getElementById("prompt-select-message").appendChild(doc.createTextNode(aText));
    
    let list = doc.getElementById("prompt-select-list");
    for (let i = 0; i < aCount; i++)
      list.appendItem(aSelectList[i], null, null);
      
    
    list.selectedIndex = 0;
    
    dialog.waitForClose();
    return params.result;
  },

  

  nsIAuthPrompt_prompt : function (title, text, passwordRealm, savePassword, defaultText, result) {
    
    if (defaultText)
      result.value = defaultText;
    return this.nsIPrompt_prompt(title, text, result, null, {});
  },

  nsIAuthPrompt_promptUsernameAndPassword : function (title, text, passwordRealm, savePassword, user, pass) {
    
    return this.nsIPrompt_promptUsernameAndPassword(title, text, user, pass, null, {});
  },

  nsIAuthPrompt_promptPassword : function (title, text, passwordRealm, savePassword, pass) {
    
    return this.nsIPrompt_promptPassword(title, text, pass, null, {});
  },

  
  
  promptAuth: function promptAuth(aChannel, aLevel, aAuthInfo, aCheckMsg, aCheckState) {
    let res = false;
    
    let defaultUser = aAuthInfo.username;
    if ((aAuthInfo.flags & aAuthInfo.NEED_DOMAIN) && (aAuthInfo.domain.length > 0))
      defaultUser = aAuthInfo.domain + "\\" + defaultUser;
    
    let username = { value: defaultUser };
    let password = { value: aAuthInfo.password };
    
    let message = PromptUtils.makeDialogText(aChannel, aAuthInfo);
    let title = PromptUtils.getLocaleString("PromptUsernameAndPassword2");
    
    if (aAuthInfo.flags & aAuthInfo.ONLY_PASSWORD)
      res = this.promptPassword(title, message, password, aCheckMsg, aCheckState);
    else
      res = this.promptUsernameAndPassword(title, message, username, password, aCheckMsg, aCheckState);
    
    if (res) {
      aAuthInfo.username = username.value;
      aAuthInfo.password = password.value;
    }
    
    return res;
  },
  
  asyncPromptAuth: function asyncPromptAuth(aChannel, aCallback, aContext, aLevel, aAuthInfo, aCheckMsg, aCheckState) {
    
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }
};

let PromptUtils = {
  getLocaleString: function getLocaleString(key) {
    return this.bundle.GetStringFromName(key);
  },
  
  
  makeDialogText: function makeDialogText(aChannel, aAuthInfo) {
    let HostPort = this.getAuthHostPort(aChannel, aAuthInfo);
    let displayHost = HostPort.host;
    let uri = aChannel.URI;
    let scheme = uri.scheme;
    let username = aAuthInfo.username;
    let proxyAuth = (aAuthInfo.flags & aAuthInfo.AUTH_PROXY) != 0;
    let realm = aAuthInfo.realm;
    if (realm.length > 100) { 
      let pref = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
      let ellipsis = pref.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString).data;
      if (!ellipsis)
        ellipsis = "...";
      realm = realm.substring(0, 100) + ellipsis;
    }
    
    if (HostPort.port != -1)
      displayHost += ":" + HostPort.port;
    
    let text = null;
    if (proxyAuth) {
      text = "EnterLoginForProxy";
    } else {
      text = "EnterLoginForRealm";
      displayHost = scheme + "://" + displayHost;
    }
    
    let strings = [realm, displayHost];
    let count = 2;
    if (aAuthInfo.flags & aAuthInfo.ONLY_PASSWORD) {
      text = "EnterPasswordFor";
      strings[0] = username;
    } else if (!proxyAuth && (realm.length == 0)) {
      text = "EnterUserPasswordFor";
      count = 1;
      strings[0] = strings[1];
    }
    
    return this.bundle.formatStringFromName(text, strings, count);
  },
  
  
  getAuthHostPort: function getAuthHostPort(aChannel, aAuthInfo) {
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
  }
};

XPCOMUtils.defineLazyGetter(PromptUtils, "bundle", function () {
  let bundleService = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
  return bundleService.createBundle("chrome://global/locale/commonDialogs.properties");
});

const NSGetFactory = XPCOMUtils.generateNSGetFactory([PromptService]);
