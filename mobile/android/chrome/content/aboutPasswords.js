



let Ci = Components.interfaces, Cc = Components.classes, Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm")
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(window, "gChromeWin", function()
  window.QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIWebNavigation)
    .QueryInterface(Ci.nsIDocShellTreeItem)
    .rootTreeItem
    .QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIDOMWindow)
    .QueryInterface(Ci.nsIDOMChromeWindow));

let debug = Cu.import("resource:

let gStringBundle = Services.strings.createBundle("chrome://browser/locale/aboutPasswords.properties");

function copyStringAndToast(string, notifyString) {
  try {
    let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper);
    clipboard.copyString(string);
    gChromeWin.NativeWindow.toast.show(notifyString, "short");
  } catch (e) {
    debug("Error copying from about:passwords");
    gChromeWin.NativeWindow.toast.show(gStringBundle.GetStringFromName("passwordsDetails.copyFailed"), "short");
  }
}

let Passwords = {
  init: function () {
    window.addEventListener("popstate", this , false);

    Services.obs.addObserver(this, "passwordmgr-storage-changed", false);

    this._loadList();

    document.getElementById("copyusername-btn").addEventListener("click", this._copyUsername.bind(this), false);
    document.getElementById("copypassword-btn").addEventListener("click", this._copyPassword.bind(this), false);
    document.getElementById("details-header").addEventListener("click", this._openLink.bind(this), false);

    this._showList();
  },

  uninit: function () {
    Services.obs.removeObserver(this, "passwordmgr-storage-changed");
    window.removeEventListener("popstate", this, false);
  },

  _loadList: function () {
    let logins;
    try {
      logins = Services.logins.getAllLogins();
    } catch(e) {
      
      debug("Master password permissions error: " + e);
      return;
    }

    logins.forEach(login => login.QueryInterface(Ci.nsILoginMetaInfo));

    logins.sort((a, b) => a.hostname.localeCompare(b.hostname));

    
    let list = document.getElementById("logins-list");
    list.innerHTML = "";
    logins.forEach(login => {
      let item = this._createItemForLogin(login);
      list.appendChild(item);
    });
  },

  _showList: function () {
    
    let details = document.getElementById("login-details");
    details.setAttribute("hidden", "true");
    let list = document.getElementById("logins-list");
    list.removeAttribute("hidden");
  },

  _onPopState: function (event) {
    
    if (event.state) {
      
      this._showDetails(this._getElementForLogin(event.state.id));
    } else {
      
      let detailItem = document.querySelector("#login-details > .login-item");
      detailItem.login = null;
      this._showList();
    }
  },

  _createItemForLogin: function (login) {
    let loginItem = document.createElement("div");

    loginItem.setAttribute("loginID", login.guid);
    loginItem.className = "login-item list-item";
    loginItem.addEventListener("click", () => {
      this._showDetails(loginItem);
      history.pushState({ id: login.guid }, document.title);
    }, true);

    
    let img = document.createElement("img");
    img.className = "icon";
    img.setAttribute("src", login.hostname + "/favicon.ico");
    loginItem.appendChild(img);

    
    let inner = document.createElement("div");
    inner.className = "inner";

    let details = document.createElement("div");
    details.className = "details";
    inner.appendChild(details);

    let titlePart = document.createElement("div");
    titlePart.className = "hostname";
    titlePart.textContent = login.hostname;
    details.appendChild(titlePart);

    let versionPart = document.createElement("div");
    versionPart.textContent = login.httpRealm;
    versionPart.className = "realm";
    details.appendChild(versionPart);

    let descPart = document.createElement("div");
    descPart.textContent = login.username;
    descPart.className = "username";
    inner.appendChild(descPart);

    loginItem.appendChild(inner);
    loginItem.login = login;
    return loginItem;
  },

  _getElementForLogin: function (login) {
    let list = document.getElementById("logins-list");
    let element = list.querySelector("div[loginID=" + login.quote() + "]");
    return element;
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "popstate": {
        this._onPopState(event);
        break;
      }
    }
  },

  observe: function (subject, topic, data) {
    switch(topic) {
      case "passwordmgr-storage-changed": {
        
        this._loadList();
        break;
      }
    }
  },

  _showDetails: function (listItem) {
    let detailItem = document.querySelector("#login-details > .login-item");
    let login = detailItem.login = listItem.login;
    let favicon = detailItem.querySelector(".icon");
    favicon.setAttribute("src", login.hostname + "/favicon.ico");

    document.getElementById("details-header").setAttribute("link", login.hostname);

    document.getElementById("detail-hostname").textContent = login.hostname;
    document.getElementById("detail-realm").textContent = login.httpRealm;
    document.getElementById("detail-username").textContent = login.username;

    
    let matchedURL = login.hostname.match(/^((?:[a-z]+:\/\/)?(?:[^\/]+@)?)(.+?)(?::\d+)?(?:\/|$)/);

    let userInputs = [];
    if (matchedURL) {
      let [, , domain] = matchedURL;
      userInputs = domain.split(".").filter(part => part.length > 3);
    }

    let lastChanged = new Date(login.timePasswordChanged);
    let days = Math.round((Date.now() - lastChanged) / 1000 / 60 / 60/ 24);
    document.getElementById("detail-age").textContent = gStringBundle.formatStringFromName("passwordsDetails.age", [days], 1);

    let list = document.getElementById("logins-list");
    list.setAttribute("hidden", "true");

    let loginDetails = document.getElementById("login-details");
    loginDetails.removeAttribute("hidden");

    
    let loadEvent = document.createEvent("Events");
    loadEvent.initEvent("PasswordsDetailsLoad", true, false);
    window.dispatchEvent(loadEvent);
  },

  _copyUsername: function() {
    let detailItem = document.querySelector("#login-details > .login-item");
    let login = detailItem.login;
    copyStringAndToast(login.username, gStringBundle.GetStringFromName("passwordsDetails.usernameCopied"));
  },

  _copyPassword: function() {
    let detailItem = document.querySelector("#login-details > .login-item");
    let login = detailItem.login;
    copyStringAndToast(login.password, gStringBundle.GetStringFromName("passwordsDetails.passwordCopied"));
  },

  _openLink: function (clickEvent) {
    let url = clickEvent.currentTarget.getAttribute("link");
    let BrowserApp = gChromeWin.BrowserApp;
    BrowserApp.addTab(url, { selected: true, parentId: BrowserApp.selectedTab.id });
  }
};

window.addEventListener("load", Passwords.init.bind(Passwords), false);
window.addEventListener("unload", Passwords.uninit.bind(Passwords), false);
