


var OfflineApps = {
  offlineAppRequested: function(aRequest, aTarget) {
    if (!Services.prefs.getBoolPref("browser.offline-apps.notify"))
      return;

    let currentURI = Services.io.newURI(aRequest.location, aRequest.charset, null);

    
    if (Services.perms.testExactPermission(currentURI, "offline-app") != Ci.nsIPermissionManager.UNKNOWN_ACTION)
      return;

    try {
      if (Services.prefs.getBoolPref("offline-apps.allow_by_default")) {
        
        return;
      }
    } catch(e) {
      
    }

    let host = currentURI.asciiHost;
    let notificationID = "offline-app-requested-" + host;
    let notificationBox = Browser.getNotificationBox();

    let notification = notificationBox.getNotificationWithValue(notificationID);
    let strings = Strings.browser;
    if (notification) {
      notification.documents.push(aRequest);
    } else {
      let buttons = [{
        label: strings.GetStringFromName("offlineApps.allow"),
        accessKey: null,
        callback: function() {
          for (let i = 0; i < notification.documents.length; i++)
            OfflineApps.allowSite(notification.documents[i], aTarget);
        }
      },{
        label: strings.GetStringFromName("offlineApps.never"),
        accessKey: null,
        callback: function() {
          for (let i = 0; i < notification.documents.length; i++)
            OfflineApps.disallowSite(notification.documents[i]);
        }
      },{
        label: strings.GetStringFromName("offlineApps.notNow"),
        accessKey: null,
        callback: function() {  }
      }];

      const priority = notificationBox.PRIORITY_INFO_LOW;
      let message = strings.formatStringFromName("offlineApps.available2", [host], 1);
      notification = notificationBox.appendNotification(message, notificationID, "", priority, buttons);
      notification.documents = [aRequest];
    }
  },

  allowSite: function(aRequest, aTarget) {
    let currentURI = Services.io.newURI(aRequest.location, aRequest.charset, null);
    Services.perms.add(currentURI, "offline-app", Ci.nsIPermissionManager.ALLOW_ACTION);

    
    
    
    aTarget.messageManager.sendAsyncMessage("Browser:MozApplicationCache:Fetch", aRequest);
  },

  disallowSite: function(aRequest) {
    let currentURI = Services.io.newURI(aRequest.location, aRequest.charset, null);
    Services.perms.add(currentURI, "offline-app", Ci.nsIPermissionManager.DENY_ACTION);
  },

  receiveMessage: function receiveMessage(aMessage) {
    if (aMessage.name == "Browser:MozApplicationManifest") {
      this.offlineAppRequested(aMessage.json, aMessage.target);
    }
  }
};

