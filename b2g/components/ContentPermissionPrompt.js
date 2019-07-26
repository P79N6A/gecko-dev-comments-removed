



const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
const Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");

function ContentPermissionPrompt() {}

ContentPermissionPrompt.prototype = {

  handleExistingPermission: function handleExistingPermission(request) {
    let result = Services.perms.testExactPermissionFromPrincipal(request.principal, request.type);
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

  _id: 0,
  prompt: function(request) {
    
    if (this.handleExistingPermission(request))
       return;

    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    let content = browser.getContentWindow();
    if (!content)
      return;

    let requestId = this._id++;
    content.addEventListener("mozContentEvent", function contentEvent(evt) {
      if (evt.detail.id != requestId)
        return;
      evt.target.removeEventListener(evt.type, contentEvent);

      if (evt.detail.type == "permission-allow") {

        if (evt.detail.remember) {
          Services.perms.addFromPrincipal(request.principal, request.type,
                                          Ci.nsIPermissionManager.ALLOW_ACTION);
        }

        request.allow();
        return;
      }

      if (evt.detail.remember) {
        Services.perms.addFromPrincipal(request.principal, request.type,
                                        Ci.nsIPermissionManager.DENY_ACTION);
      }

      request.cancel();
    });

    let principal = request.principal;
    let isApp = principal.appStatus != Ci.nsIPrincipal.APP_STATUS_NOT_INSTALLED;

    let details = {
      type: "permission-prompt",
      permission: request.type,
      id: requestId,
      origin: principal.origin,
      isApp: isApp
    };

    if (!isApp) {
      browser.shell.sendChromeEvent(details);
      return;
    }

    
    let app = DOMApplicationRegistry.getAppByLocalId(principal.appId);
    DOMApplicationRegistry.getManifestFor(app.origin, function getManifest(aManifest) {
      let helper = new ManifestHelper(aManifest, app.origin);
      details.appName = helper.name;
      browser.shell.sendChromeEvent(details);
    });
  },

  classID: Components.ID("{8c719f03-afe0-4aac-91ff-6c215895d467}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentPermissionPrompt])
};



const NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentPermissionPrompt]);
