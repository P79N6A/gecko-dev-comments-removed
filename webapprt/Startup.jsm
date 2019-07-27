








this.EXPORTED_SYMBOLS = ["startup"];

const Ci = Components.interfaces;
const Cu = Components.utils;






Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm");

Cu.import("resource://webapprt/modules/WebappRT.jsm");




XPCOMUtils.defineLazyModuleGetter(this, "PermissionsInstaller",
  "resource://gre/modules/PermissionsInstaller.jsm");

const PROFILE_DIR = OS.Constants.Path.profileDir;

function isFirstRunOrUpdate() {
  let savedBuildID = null;
  try {
    savedBuildID = Services.prefs.getCharPref("webapprt.buildID");
  } catch (e) {}

  let ourBuildID = Services.appinfo.platformBuildID;

  if (ourBuildID != savedBuildID) {
    Services.prefs.setCharPref("webapprt.buildID", ourBuildID);
    return true;
  }

  return false;
}

function writeFile(aPath, aData) {
  return Task.spawn(function() {
    let data = new TextEncoder().encode(aData);
    yield OS.File.writeAtomic(aPath, data, { tmpPath: aPath + ".tmp" });
  });
}

function createBrandingFiles() {
  return Task.spawn(function() {
    let manifest = WebappRT.localeManifest;
    let name = WebappRT.localeManifest.name;
    let developer = " ";
    if (WebappRT.localeManifest.developer) {
      developer = WebappRT.localeManifest.developer.name;
    }

    let brandDTDContent = '<!ENTITY brandShortName "' + name + '">\n\
  <!ENTITY brandFullName "' + name + '">\n\
  <!ENTITY vendorShortName "' + developer + '">\n\
  <!ENTITY trademarkInfo.part1 " ">';

    yield writeFile(OS.Path.join(PROFILE_DIR, "brand.dtd"), brandDTDContent);

    let brandPropertiesContent = 'brandShortName=' + name + '\n\
  brandFullName=' + name + '\n\
  vendorShortName=' + developer;

    yield writeFile(OS.Path.join(PROFILE_DIR, "brand.properties"),
                    brandPropertiesContent);
  });
}



this.startup = function(window) {
  return Task.spawn(function () {
    
    
    let deferredWindowLoad = Promise.defer();
    if (window.document && window.document.getElementById("content")) {
      deferredWindowLoad.resolve();
    } else {
      window.addEventListener("DOMContentLoaded", function onLoad() {
        window.removeEventListener("DOMContentLoaded", onLoad, false);
        deferredWindowLoad.resolve();
      });
    }

    let appUpdated = false;
    let updatePending = yield WebappRT.isUpdatePending();
    if (updatePending) {
      appUpdated = yield WebappRT.applyUpdate();
    }

    yield WebappRT.configPromise;

    let appData = WebappRT.config.app;

    
    Cu.import("resource://gre/modules/Webapps.jsm");
    
    Cu.import("resource://webapprt/modules/WebappManager.jsm");

    
    yield DOMApplicationRegistry.registryStarted;
    
    yield DOMApplicationRegistry.addInstalledApp(appData, appData.manifest,
                                                 appData.updateManifest);

    let manifestURL = appData.manifestURL;
    if (manifestURL) {
      
      
      if (isFirstRunOrUpdate(Services.prefs) || appUpdated) {
        PermissionsInstaller.installPermissions(appData, true);
        yield createBrandingFiles();
      }
    }

    
    let aliasFile = Components.classes["@mozilla.org/file/local;1"]
                              .createInstance(Ci.nsIFile);
    aliasFile.initWithPath(PROFILE_DIR);

    let aliasURI = Services.io.newFileURI(aliasFile);

    Services.io.getProtocolHandler("resource")
               .QueryInterface(Ci.nsIResProtocolHandler)
               .setSubstitution("webappbranding", aliasURI);

    
    yield deferredWindowLoad.promise;

    
    
    
    Components.manager.QueryInterface(Ci.nsIComponentRegistrar)
              .registerFactory(Components.ID("{1b4c85df-cbdd-4bb6-b04e-613caece083c}"),
                               "", "@mozilla.org/transfer;1", null);

    
    
    Cu.import("resource://gre/modules/Payment.jsm");
    Cu.import("resource://gre/modules/AlarmService.jsm");
    Cu.import("resource://webapprt/modules/WebRTCHandler.jsm");
    Cu.import("resource://webapprt/modules/DownloadView.jsm");

    
    let appBrowser = window.document.getElementById("content");

    
    appBrowser.docShell.setIsApp(WebappRT.appID);
    appBrowser.setAttribute("src", WebappRT.launchURI);

    if (appData.manifest.fullscreen) {
      appBrowser.addEventListener("load", function onLoad() {
        appBrowser.removeEventListener("load", onLoad, true);
        appBrowser.contentDocument.
          documentElement.mozRequestFullScreen();
      }, true);
    }

    WebappRT.startUpdateService();
  });
}
