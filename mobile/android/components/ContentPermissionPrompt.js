



const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
const Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const kEntities = {
  "contacts": "contacts",
  "desktop-notification": "desktopNotification",
  "device-storage:music": "deviceStorageMusic",
  "device-storage:pictures": "deviceStoragePictures",
  "device-storage:sdcard": "deviceStorageSdcard",
  "device-storage:videos": "deviceStorageVideos",
  "geolocation": "geolocation",
};


const PROMPT_FOR_UNKNOWN = [
  "desktop-notification",
  "geolocation",
];

function ContentPermissionPrompt() {}

ContentPermissionPrompt.prototype = {
  classID: Components.ID("{C6E8C44D-9F39-4AF7-BCC0-76E38A8310F5}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentPermissionPrompt]),

  handleExistingPermission: function handleExistingPermission(request, type, denyUnknown) {
    let result = Services.perms.testExactPermissionFromPrincipal(request.principal, type);
    if (result == Ci.nsIPermissionManager.ALLOW_ACTION) {
      request.allow();
      return true;
    }

    if (result == Ci.nsIPermissionManager.DENY_ACTION) {
      request.cancel();
      return true;
    }

    if (denyUnknown && result == Ci.nsIPermissionManager.UNKNOWN_ACTION) {
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

  prompt: function(request) {
    let isApp = request.principal.appId !== Ci.nsIScriptSecurityManager.NO_APP_ID && request.principal.appId !== Ci.nsIScriptSecurityManager.UNKNOWN_APP_ID;

    
    let types = request.types.QueryInterface(Ci.nsIArray);
    if (types.length != 1) {
      request.cancel();
      return;
    }
    let perm = types.queryElementAt(0, Ci.nsIContentPermissionType);

    
    let access = (perm.access && perm.access !== "unused") ?
                 (perm.type + "-" + perm.access) : perm.type;
    if (this.handleExistingPermission(request, access,
           isApp || PROMPT_FOR_UNKNOWN.indexOf(perm.type) < 0))
       return;

    let chromeWin = this.getChromeForRequest(request);
    let tab = chromeWin.BrowserApp.getTabForWindow(request.window.top);
    if (!tab)
      return;

    let browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
    let entityName = kEntities[perm.type];

    let buttons = [{
      label: browserBundle.GetStringFromName(entityName + ".dontAllow"),
      callback: function(aChecked) {
        
        if (aChecked)
          Services.perms.addFromPrincipal(request.principal, access, Ci.nsIPermissionManager.DENY_ACTION);

        request.cancel();
      }
    },
    {
      label: browserBundle.GetStringFromName(entityName + ".allow"),
      callback: function(aChecked) {
        
        if (aChecked) {
          Services.perms.addFromPrincipal(request.principal, access, Ci.nsIPermissionManager.ALLOW_ACTION);
        } else if (isApp || entityName == "desktopNotification") {
          
          Services.perms.addFromPrincipal(request.principal, access, Ci.nsIPermissionManager.ALLOW_ACTION, Ci.nsIPermissionManager.EXPIRE_SESSION);
        }

        request.allow();
      },
      positive: true
    }];

    let requestor = chromeWin.BrowserApp.manifest ? "'" + chromeWin.BrowserApp.manifest.name + "'" : request.principal.URI.host;
    let message = browserBundle.formatStringFromName(entityName + ".ask", [requestor], 1);
    let options = { checkbox: browserBundle.GetStringFromName(entityName + ".dontAskAgain") };

    chromeWin.NativeWindow.doorhanger.show(message, entityName + request.principal.URI.host, buttons, tab.id, options, entityName.toUpperCase());
  }
};



this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentPermissionPrompt]);
