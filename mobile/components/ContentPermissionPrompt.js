




































const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
const Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const kCountBeforeWeRemember = 5;

function dump(a) {
    Cc["@mozilla.org/consoleservice;1"]
        .getService(Ci.nsIConsoleService)
        .logStringMessage(a);
}

function setPagePermission(type, uri, allow) {
  let pm = Services.perms;
  let contentPrefs = Services.contentPrefs;
  let contentPrefName = type + ".request.remember";

  if (!contentPrefs.hasPref(uri, contentPrefName))
      contentPrefs.setPref(uri, contentPrefName, 0);

  let count = contentPrefs.getPref(uri, contentPrefName);

  if (allow == false)
    count--;
  else
    count++;
    
  contentPrefs.setPref(uri, contentPrefName, count);
  if (count == kCountBeforeWeRemember)
    pm.add(uri, type, Ci.nsIPermissionManager.ALLOW_ACTION);
  else if (count == -kCountBeforeWeRemember)
    pm.add(uri, type, Ci.nsIPermissionManager.DENY_ACTION);
}

const kEntities = { "geolocation": "geolocation", "desktop-notification": "desktopNotification",
                    "indexedDB": "offlineApps", "indexedDBQuota": "indexedDBQuota",
                    "openWebappsManage": "openWebappsManage" };

function ContentPermissionPrompt() {}

ContentPermissionPrompt.prototype = {
  classID: Components.ID("{C6E8C44D-9F39-4AF7-BCC0-76E38A8310F5}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentPermissionPrompt]),

  _promptId : 0,
  _callbackId : 0,
  _callbacks : [],

  handleExistingPermission: function handleExistingPermission(request) {
    let result = Services.perms.testExactPermission(request.uri, request.type);
    if (result == Ci.nsIPermissionManager.ALLOW_ACTION) {
      request.allow();
      return true;
    }
    if (result == Ci.nsIPermissionManager.DENY_ACTION) {
      request.cancel();
      return true;
    }
    return false;
  },

  getChromeWindow: function getChromeWindow(aWindow) {
     let chromeWin = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIWebNavigation)
                            .QueryInterface(Ci.nsIDocShellTreeItem)
                            .rootTreeItem
                            .QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindow)
                            .QueryInterface(Ci.nsIDOMChromeWindow);
     return chromeWin;
  },

  getTabForRequest: function getTabForRequest(request) {
    if (request.window) {
      let requestingWindow = request.window.top;
      let chromeWin = this.getChromeWindow(requestingWindow).wrappedJSObject;
      let windowID = chromeWin.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
      let browser = chromeWin.BrowserApp.getBrowserForWindow(request.window);
      let tabID = chromeWin.BrowserApp.getTabForBrowser(browser).id;
      return tabID;
    }
    let chromeWin = request.element.ownerDocument.defaultView;
    let browser = chromeWin.Browser;
    let tabID = chromeWin.BrowserApp.getTabForBrowser(browser).id;
    return tabID;
  },

  prompt: function(request) {
    
    if (this.handleExistingPermission(request))
       return;

    let pm = Services.perms;
    let browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");

    let entityName = kEntities[request.type];

    let tabID = this.getTabForRequest(request);

    this._promptId++;
    this._callbackId++;
    let allowCallback = {
      cb : function(notification) {
        setPagePermission(request.type, request.uri, true);
        request.allow();
      },
      callbackId  : this._callbackId,
      promptId : this._promptId
    };
    this._callbackId++;
    let denyCallback = {
      cb : function(notification) {
        setPagePermission(request.type, request.uri, false);
        request.cancel();
      },
      callbackId : this._callbackId,
      promptId : this._promptId
    };
    this._callbacks.push(allowCallback);
    this._callbacks.push(denyCallback);

    let buttons = [{
      label: browserBundle.GetStringFromName(entityName + ".allow"),
      accessKey: null,
      callback: allowCallback.callbackId
    },
    {
      label: browserBundle.GetStringFromName(entityName + ".dontAllow"),
      accessKey: null,
      callback: denyCallback.callbackId
    }];

    let message = browserBundle.formatStringFromName(entityName + ".wantsTo",
                                                     [request.uri.host], 1);

    let DoorhangerEventListener = {
      _contentPermission: this,
      init: function(owner) {
        Services.obs.addObserver(this, "Doorhanger:Reply", false);
      },
      observe: function(aSubject, aTopic, aData) {
        let cpo = this._contentPermission;
        if (aTopic == "Doorhanger:Reply") {
          let cbId = parseInt(aData);
          let promptId = -1;
          let keepStack = [];
          
          for (i = 0; i < cpo._callbacks.length; i++) {
            if (cpo._callbacks[i].callbackId == cbId) {
              promptId = cpo._callbacks[i].promptId;
              cpo._callbacks[i].cb();
              break;
            }
          }
          
          
          for (i = 0; i < cpo._callbacks.length; i++) {
            if (cpo._callbacks[i].promptId != promptId) {
              keepStack.push(cpo._callbacks[i]);
            }
          }
          
          cpo._callbacks = keepStack;
          if (cpo._callbacks.length == 0) {
            
            Services.obs.removeObserver(this, "Doorhanger:Reply");
          }
        }
      }
    };
    DoorhangerEventListener.init(this);

    let json = {
      gecko: {
        type: "Doorhanger:Add",
        message: message,
        severity: "PRIORITY_WARNING_MEDIUM",
        buttons: buttons,
        tabID: tabID
      }
    };

    Cc["@mozilla.org/android/bridge;1"]
          .getService(Ci.nsIAndroidBridge)
          .handleGeckoMessage(JSON.stringify(json));
  }
};



const NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentPermissionPrompt]);
