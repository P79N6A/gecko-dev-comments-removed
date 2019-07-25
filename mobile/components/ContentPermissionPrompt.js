




































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

  getChromeForRequest: function getChromeForRequest(request) {
    if (request.window) {
      let requestingWindow = request.window.top;
      return this.getChromeWindow(requestingWindow).wrappedJSObject;
    }
    return request.element.ownerDocument.defaultView;
  },

  getTabForRequest: function getTabForRequest(request) {
    let chromeWin = this.getChromeForRequest(request);
    if (request.window) {
      let browser = chromeWin.BrowserApp.getBrowserForWindow(request.window);
      let tabID = chromeWin.BrowserApp.getTabForBrowser(browser).id;
      return tabID;
    }
    
    return null;
  },

  prompt: function(request) {
    
    if (this.handleExistingPermission(request))
       return;

    let pm = Services.perms;
    let browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");

    let entityName = kEntities[request.type];

    let tabID = this.getTabForRequest(request);
    let chromeWin = this.getChromeForRequest(request);

    let buttons = [{
      label: browserBundle.GetStringFromName(entityName + ".allow"),
      accessKey: null,
      callback: function(notification) {
        setPagePermission(request.type, request.uri, true);
        request.allow();
      }
    },
    {
      label: browserBundle.GetStringFromName(entityName + ".dontAllow"),
      accessKey: null,
      callback: function(notification) {
        setPagePermission(request.type, request.uri, false);
        request.cancel();
      }
    }];

    let message = browserBundle.formatStringFromName(entityName + ".wantsTo",
                                                     [request.uri.host], 1);

    chromeWin.NativeWindow.doorhanger.show(message, buttons, tabID);
  }
};



const NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentPermissionPrompt]);
