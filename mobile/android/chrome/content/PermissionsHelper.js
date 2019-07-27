


"use strict";

var PermissionsHelper = {
  _permissonTypes: ["password", "geolocation", "popup", "indexedDB",
                    "offline-app", "desktop-notification", "plugins", "native-intent"],
  _permissionStrings: {
    "password": {
      label: "password.savePassword",
      allowed: "password.save",
      denied: "password.dontSave"
    },
    "geolocation": {
      label: "geolocation.shareLocation",
      allowed: "geolocation.allow",
      denied: "geolocation.dontAllow"
    },
    "popup": {
      label: "blockPopups.label",
      allowed: "popup.show",
      denied: "popup.dontShow"
    },
    "indexedDB": {
      label: "offlineApps.storeOfflineData",
      allowed: "offlineApps.allow",
      denied: "offlineApps.dontAllow2"
    },
    "offline-app": {
      label: "offlineApps.storeOfflineData",
      allowed: "offlineApps.allow",
      denied: "offlineApps.dontAllow2"
    },
    "desktop-notification": {
      label: "desktopNotification.useNotifications",
      allowed: "desktopNotification.allow",
      denied: "desktopNotification.dontAllow"
    },
    "plugins": {
      label: "clickToPlayPlugins.activatePlugins",
      allowed: "clickToPlayPlugins.activate",
      denied: "clickToPlayPlugins.dontActivate"
    },
    "native-intent": {
      label: "helperapps.openWithList2",
      allowed: "helperapps.always",
      denied: "helperapps.never"
    }
  },

  observe: function observe(aSubject, aTopic, aData) {
    let uri = BrowserApp.selectedBrowser.currentURI;

    switch (aTopic) {
      case "Permissions:Get":
        let permissions = [];
        for (let i = 0; i < this._permissonTypes.length; i++) {
          let type = this._permissonTypes[i];
          let value = this.getPermission(uri, type);

          
          if (value == Services.perms.UNKNOWN_ACTION)
            continue;

          
          let typeStrings = this._permissionStrings[type];
          let label = Strings.browser.GetStringFromName(typeStrings["label"]);

          
          let valueKey = value == Services.perms.ALLOW_ACTION ?
                         "allowed" : "denied";
          let valueString = Strings.browser.GetStringFromName(typeStrings[valueKey]);

          permissions.push({
            type: type,
            setting: label,
            value: valueString
          });
        }

        
        this._currentPermissions = permissions;

        let host;
        try {
          host = uri.host;
        } catch(e) {
          host = uri.spec;
        }
        Messaging.sendRequest({
          type: "Permissions:Data",
          host: host,
          permissions: permissions
        });
        break;
 
      case "Permissions:Clear":
        
        let permissionsToClear = JSON.parse(aData);
        let privacyContext = BrowserApp.selectedBrowser.docShell
                               .QueryInterface(Ci.nsILoadContext);

        for (let i = 0; i < permissionsToClear.length; i++) {
          let indexToClear = permissionsToClear[i];
          let permissionType = this._currentPermissions[indexToClear]["type"];
          this.clearPermission(uri, permissionType, privacyContext);
        }
        break;
    }
  },

  








  getPermission: function getPermission(aURI, aType) {
    
    
    if (aType == "password") {
      
      
      if (!Services.logins.getLoginSavingEnabled(aURI.prePath))
        return Services.perms.DENY_ACTION;

      
      if (Services.logins.countLogins(aURI.prePath, "", ""))
        return Services.perms.ALLOW_ACTION;

      return Services.perms.UNKNOWN_ACTION;
    }

    
    if (aType == "geolocation")
      return Services.perms.testExactPermission(aURI, aType);

    return Services.perms.testPermission(aURI, aType);
  },

  






  clearPermission: function clearPermission(aURI, aType, aContext) {
    
    
    if (aType == "password") {
      
      let logins = Services.logins.findLogins({}, aURI.prePath, "", "");
      for (let i = 0; i < logins.length; i++) {
        Services.logins.removeLogin(logins[i]);
      }
      
      Services.logins.setLoginSavingEnabled(aURI.prePath, true);
    } else {
      Services.perms.remove(aURI.host, aType);
      
      Cc["@mozilla.org/content-pref/service;1"]
        .getService(Ci.nsIContentPrefService2)
        .removeByDomainAndName(aURI.spec, aType + ".request.remember", aContext);
    }
  }
};
