



this.EXPORTED_SYMBOLS = ["WebappRT"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, 'NativeApp',
  'resource://gre/modules/NativeApp.jsm');

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                  "@mozilla.org/AppsService;1",
                                  "nsIAppsService");

this.WebappRT = {
  _configPromise: null,

  get configPromise() {
    if (!this._configPromise) {
      this._configPromise = Task.spawn(function*() {
        let webappJson = OS.Path.join(Services.dirsvc.get("AppRegD", Ci.nsIFile).path,
                                      "webapp.json");

        WebappRT.config = yield AppsUtils.loadJSONAsync(webappJson);
      });
    }

    return this._configPromise;
  },

  get launchURI() {
    return this.localeManifest.fullLaunchPath();
  },

  get localeManifest() {
    return new ManifestHelper(this.config.app.manifest,
                              this.config.app.origin);
  },

  get appID() {
    let manifestURL = this.config.app.manifestURL;
    if (!manifestURL) {
      return Ci.nsIScriptSecurityManager.NO_APP_ID;
    }

    return appsService.getAppLocalIdByManifestURL(manifestURL);
  },

  isUpdatePending: Task.async(function*() {
    let webappJson = OS.Path.join(Services.dirsvc.get("AppRegD", Ci.nsIFile).path,
                                  "update", "webapp.json");

    if (!(yield OS.File.exists(webappJson))) {
      return false;
    }

    return true;
  }),

  applyUpdate: Task.async(function*() {
    let webappJson = OS.Path.join(Services.dirsvc.get("AppRegD", Ci.nsIFile).path,
                                  "update", "webapp.json");
    let config = yield AppsUtils.loadJSONAsync(webappJson);

    let nativeApp = new NativeApp(config.app, config.app.manifest,
                                  config.app.categories,
                                  config.registryDir);
    try {
      yield nativeApp.applyUpdate(config.app);
    } catch (ex) {
      return false;
    }

    
    
    this.config = config;
    this._configPromise = Promise.resolve();

    return true;
  }),

  startUpdateService: function() {
    let manifestURL = this.config.app.manifestURL;
    
    
    if (!manifestURL) {
      return;
    }

    
    let timerManager = Cc["@mozilla.org/updates/timer-manager;1"].
                       getService(Ci.nsIUpdateTimerManager);
    timerManager.registerTimer("updateTimer", () => {
      let window = Services.wm.getMostRecentWindow("webapprt:webapp");
      window.navigator.mozApps.mgmt.getAll().onsuccess = function() {
        let thisApp = null;
        for (let app of this.result) {
          if (app.manifestURL == manifestURL) {
            thisApp = app;
            break;
          }
        }

        
        if (!thisApp) {
          Cu.reportError("Couldn't find the app in the webapps registry");
          return;
        }

        thisApp.ondownloadavailable = () => {
          
          thisApp.download();
        };

        thisApp.ondownloadsuccess = () => {
          
          window.navigator.mozApps.mgmt.applyDownload(thisApp);
        };

        thisApp.ondownloadapplied = () => {
          
        };

        thisApp.checkForUpdate();
      }
    }, Services.prefs.getIntPref("webapprt.app_update_interval"));
  },
};
