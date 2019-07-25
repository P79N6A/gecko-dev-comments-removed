































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function promptService() {
  let bundleService = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
  this._bundle = bundleService.createBundle("chrome://global/locale/commonDialogs.properties");
}

promptService.prototype = {
  classDescription: "Mobile Prompt Service",
  contractID: "@mozilla.org/embedcomp/prompt-service;1",
  classID: Components.ID("{9a61149b-2276-4a0a-b79c-be994ad106cf}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPromptService, Ci.nsIPromptService2]),
 
  
  getDocument: function() {
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
    return wm.getMostRecentWindow("navigator:browser").document;
  },
 
  
  
  sizeElement: function(id, percent) {
    let elem = this.getDocument().getElementById(id);
    let screenW = this.getDocument().getElementById("main-window").getBoundingClientRect().width;
    elem.style.width = screenW * percent / 100 + "px"
  },
  
  
  
  sizeScrollableMsg: function(id, percent) {
    let screenH = this.getDocument().getElementById("main-window").getBoundingClientRect().height;
    let maxHeight = screenH * percent / 100;
    
    let elem = this.getDocument().getElementById(id);
    let height = elem.getBoundingClientRect().height;
    if (height > maxHeight)
      height = maxHeight;
    elem.parentNode.style.height = height + "px";
  },
  
  openDialog: function(parent, src, params) {
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
    let browser = wm.getMostRecentWindow("navigator:browser");
    return browser.importDialog(parent, src, params);
  },
  
  alert: function(aParent, aTitle, aText) {
    let dialog = this.openDialog(aParent, "chrome://browser/content/prompt/alert.xul", null);
    let doc = this.getDocument();
    doc.getElementById("prompt-alert-title").value = aTitle;
    doc.getElementById("prompt-alert-message").appendChild(doc.createTextNode(aText));
    this.sizeElement("prompt-alert-message", 80);
    this.sizeScrollableMsg("prompt-alert-message", 25);
    
    dialog.waitForClose();
  },
  
  alertCheck: function(aParent, aTitle, aText, aCheckMsg, aCheckState) {
    let dialog = this.openDialog(aParent, "chrome://browser/content/prompt/alert.xul", aCheckState);
    let doc = this.getDocument();
    doc.getElementById("prompt-alert-title").value = aTitle;
    doc.getElementById("prompt-alert-message").appendChild(doc.createTextNode(aText));
    this.sizeElement("prompt-alert-message", 80);
    this.sizeScrollableMsg("prompt-alert-message", 25);
    
    doc.getElementById("prompt-alert-checkbox").checked = aCheckState.value;
    this.setLabelForNode(doc.getElementById("prompt-alert-checkbox-msg"), aCheckMsg);
    this.sizeElement("prompt-alert-checkbox-msg", 50);
    doc.getElementById("prompt-alert-checkbox-box").removeAttribute("collapsed");
    
    dialog.waitForClose();
  },
  
  confirm: function(aParent, aTitle, aText) {
    var params = new Object();
    params.result = false;
    let doc = this.getDocument();
    let dialog = this.openDialog(aParent, "chrome://browser/content/prompt/confirm.xul", params);
    doc.getElementById("prompt-confirm-title").value = aTitle;
    doc.getElementById("prompt-confirm-message").appendChild(doc.createTextNode(aText));
    this.sizeElement("prompt-confirm-message", 80);
    this.sizeScrollableMsg("prompt-confirm-message", 25);
    
    dialog.waitForClose();
    return params.result;
  },
  
  confirmCheck: function(aParent, aTitle, aText, aCheckMsg, aCheckState) {
    var params = new Object();
    params.result = false;
    params.checkbox = aCheckState;
    let doc = this.getDocument();
    let dialog = this.openDialog(aParent, "chrome://browser/content/prompt/confirm.xul", params);
    doc.getElementById("prompt-confirm-title").value = aTitle;
    doc.getElementById("prompt-confirm-message").appendChild(doc.createTextNode(aText));
    this.sizeElement("prompt-confirm-message", 80);
    this.sizeScrollableMsg("prompt-confirm-message", 25);

    doc.getElementById("prompt-confirm-checkbox").checked = aCheckState.value;
    this.setLabelForNode(doc.getElementById("prompt-confirm-checkbox-msg"), aCheckMsg);
    this.sizeElement("prompt-confirm-checkbox-msg", 50);
    doc.getElementById("prompt-confirm-checkbox-box").removeAttribute("collapsed");
    
    dialog.waitForClose();
    return params.result;
  },
  
  getLocaleString: function(key) {
    return this._bundle.GetStringFromName(key);
  },
  
  
  
  
  setLabelForNode: function(aNode, aLabel) {
    
    
    
    
    
    
  
    
    
 
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

  confirmEx: function(aParent, aTitle, aText, aButtonFlags, aButton0,
            aButton1, aButton2, aCheckMsg, aCheckState) {
    let numButtons = 0;
    let titles = [aButton0, aButton1, aButton2];
    
    var params = new Object();
    params.result = false;
    params.checkbox = aCheckState;
    let doc = this.getDocument();
    let dialog = this.openDialog(aParent, "chrome://browser/content/prompt/confirm.xul", params);
    doc.getElementById("prompt-confirm-title").value = aTitle;
    doc.getElementById("prompt-confirm-message").appendChild(doc.createTextNode(aText));
    this.sizeElement("prompt-confirm-message", 80);
    this.sizeScrollableMsg("prompt-confirm-message", 25);

    doc.getElementById("prompt-confirm-checkbox").checked = aCheckState.value;
    this.setLabelForNode(doc.getElementById("prompt-confirm-checkbox-msg"), aCheckMsg);
    this.sizeElement("prompt-confirm-checkbox-msg", 50);
    if (aCheckMsg) {
      doc.getElementById("prompt-confirm-checkbox-box").removeAttribute("collapsed");
    }
    
    let bbox = doc.getElementById("prompt-confirm-button-box");
    while (bbox.lastChild) {
      bbox.removeChild(bbox.lastChild);
    }
    
    for (let i = 0; i < 3; i++) {
      let bTitle = null;
      switch (aButtonFlags & 0xff) {
        case Ci.nsIPromptService.BUTTON_TITLE_OK :
          bTitle = this.getLocaleString("OK");
        break;
        case Ci.nsIPromptService.BUTTON_TITLE_CANCEL :
          bTitle = this.getLocaleString("Cancel");
        break;
        case Ci.nsIPromptService.BUTTON_TITLE_YES :
          bTitle = this.getLocaleString("Yes");
        break;
        case Ci.nsIPromptService.BUTTON_TITLE_NO :
          bTitle = this.getLocaleString("No");
        break;
        case Ci.nsIPromptService.BUTTON_TITLE_SAVE :
          bTitle = this.getLocaleString("Save");
        break;
        case Ci.nsIPromptService.BUTTON_TITLE_DONT_SAVE :
          bTitle = this.getLocaleString("DontSave");
        break;
        case Ci.nsIPromptService.BUTTON_TITLE_REVERT :
          bTitle = this.getLocaleString("Revert");
        break;
        case Ci.nsIPromptService.BUTTON_TITLE_IS_STRING :
          bTitle = titles[i];
        break;
      }
      
      if (bTitle) {
        let button = doc.createElement("button");
        this.setLabelForNode(button, bTitle);
        button.setAttribute("class", "button-dark");
        button.setAttribute("oncommand",
          "document.getElementById('prompt-confirm-dialog').PromptHelper.closeConfirm(" + i + ")");
        bbox.appendChild(button);
      }
      
      aButtonFlags >>= 8;
    }
    
    dialog.waitForClose();
    return params.result;
  },
  
  commonPrompt : function(aParent, aTitle, aText, aValue, aCheckMsg, aCheckState, isPassword) {
    var params = new Object();
    params.result = false;
    params.checkbox = aCheckState;
    params.value = aValue;
    let dialog = this.openDialog(aParent, "chrome://browser/content/prompt/prompt.xul", params);
    let doc = this.getDocument();
    doc.getElementById("prompt-prompt-title").value = aTitle;
    doc.getElementById("prompt-prompt-message").appendChild(doc.createTextNode(aText));
    this.sizeElement("prompt-prompt-message", 80);
    this.sizeScrollableMsg("prompt-prompt-message", 25);

    doc.getElementById("prompt-prompt-checkbox").checked = aCheckState.value;
    this.setLabelForNode(doc.getElementById("prompt-prompt-checkbox-msg"), aCheckMsg);
    this.sizeElement("prompt-prompt-checkbox-msg", 50);
    doc.getElementById("prompt-prompt-textbox").value = aValue.value;
    if (aCheckMsg) {
      doc.getElementById("prompt-prompt-checkbox-box").removeAttribute("collapsed");
    }
    if (isPassword) {
      doc.getElementById("prompt-prompt-textbox").type = "password";
    }
    
    dialog.waitForClose();
    return params.result;
  },
  
  prompt : function(aParent, aTitle, aText, aValue, aCheckMsg, aCheckState) {
    return this.commonPrompt(aParent, aTitle, aText, aValue, aCheckMsg, aCheckState, false);
  },
  
  promptPassword: function(aParent, aTitle, aText, aPassword, aCheckMsg, aCheckState) {
    return this.commonPrompt(aParent, aTitle, aText, aPassword, aCheckMsg, aCheckState, true);
  },
  
  promptUsernameAndPassword: function(aParent, aTitle, aText, aUsername, aPassword, aCheckMsg, aCheckState) {
    var params = new Object();
    params.result = false;
    params.checkbox = aCheckState;
    params.user = aUsername;
    params.password = aPassword;
    let dialog = this.openDialog(aParent, "chrome://browser/content/prompt/promptPassword.xul", params);
    let doc = this.getDocument();
    doc.getElementById("prompt-password-title").value = aTitle;
    doc.getElementById("prompt-password-message").appendChild(doc.createTextNode(aText));
    this.sizeElement("prompt-password-message", 80);
    this.sizeScrollableMsg("prompt-password-message", 25);
    doc.getElementById("prompt-password-checkbox").checked = aCheckState.value;
    
    doc.getElementById("prompt-password-user").value = aUsername.value;
    doc.getElementById("prompt-password-password").value = aPassword.value;
    if (aCheckMsg) {
      doc.getElementById("prompt-password-checkbox-box").removeAttribute("collapsed");
      this.setLabelForNode(doc.getElementById("prompt-password-checkbox-msg"), aCheckMsg);
      this.sizeElement("prompt-password-checkbox-msg", 50);
      this.sizeElement("prompt-password-checkbox-box", 50);
    }
    
    dialog.waitForClose();
    return params.result;
  },
  
  
  
  
  getAuthHostPort: function(aChannel, aAuthInfo) {
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
  
  
  
  
  makeDialogText: function(aChannel, aAuthInfo) {
    let bundleService = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
    let bundle = bundleService.createBundle("chrome://global/locale/prompts.properties");
    
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
    
    return bundle.formatStringFromName(text, strings, count);
  },
  
  promptAuth: function(aParent, aChannel, aLevel, aAuthInfo, aCheckMsg, aCheckState) {
    let res = false;
    
    let defaultUser = aAuthInfo.username;
    if ((aAuthInfo.flags & aAuthInfo.NEED_DOMAIN) && (aAuthInfo.domain.length > 0))
      defaultUser = aAuthInfo.domain + "\\" + defaultUser;
    
    let username = { value: defaultUser };
    let password = { value: aAuthInfo.password };
    
    let message = this.makeDialogText(aChannel, aAuthInfo);
    let title = this.getLocaleString("PromptUsernameAndPassword2");
    
    if (aAuthInfo.flags & aAuthInfo.ONLY_PASSWORD) {
      res = this.promptPassword(aParent, title, message, password, aCheckMsg, aCheckState);
    } else {
      res = this.promptUsernameAndPassword(aParent, title, message, username, password, aCheckMsg, aCheckState);
    }
    
    if (res) {
      aAuthInfo.username = username.value;
      aAuthInfo.password = password.value;
    }
    
    return res;
  },
  
  asyncPromptAuth: function(aParent, aChannel, aCallback, aContext, aLevel, aAuthInfo, aCheckMsg, aCheckState) {
    
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  
  select: function(aParent, aTitle, aText, aCount, aSelectList, aOutSelection) {
    var params = new Object();
    params.result = false;
    params.selection = aOutSelection;
    let dialog = this.openDialog(aParent, "chrome://browser/content/prompt/select.xul", params);
    let doc = this.getDocument();
    doc.getElementById("prompt-select-title").value = aTitle;
    doc.getElementById("prompt-select-message").appendChild(doc.createTextNode(aText));
    this.sizeElement("prompt-select-message", 80);
    this.sizeScrollableMsg("prompt-select-message", 25);
    
    let list = doc.getElementById("prompt-select-list");
    for (let i = 0; i < aCount; i++)
      list.appendItem(aSelectList[i], null, null);
      
    
    list.selectedIndex = 0;
    
    dialog.waitForClose();
    return params.result;
  }
};


function NSGetModule(aCompMgr, aFileSpec) {
  return XPCOMUtils.generateModule([promptService]);
}
