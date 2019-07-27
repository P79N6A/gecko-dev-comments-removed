



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebappOSUtils",
  "resource://gre/modules/WebappOSUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");




this.EXPORTED_SYMBOLS =
  ["AppsUtils", "ManifestHelper", "isAbsoluteURI", "mozIApplication"];

function debug(s) {
  
}

this.isAbsoluteURI = function(aURI) {
  let foo = Services.io.newURI("http://foo", null, null);
  let bar = Services.io.newURI("http://bar", null, null);
  return Services.io.newURI(aURI, null, foo).prePath != foo.prePath ||
         Services.io.newURI(aURI, null, bar).prePath != bar.prePath;
}

this.mozIApplication = function(aApp) {
  _setAppProperties(this, aApp);
}

mozIApplication.prototype = {
  hasPermission: function(aPermission) {
    let uri = Services.io.newURI(this.origin, null, null);
    let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"]
                   .getService(Ci.nsIScriptSecurityManager);
    
    
    
    let principal = secMan.getAppCodebasePrincipal(uri, this.localId,
                                                   false);
    let perm = Services.perms.testExactPermissionFromPrincipal(principal,
                                                               aPermission);
    return (perm === Ci.nsIPermissionManager.ALLOW_ACTION);
  },

  hasWidgetPage: function(aPageURL) {
    let uri = Services.io.newURI(aPageURL, null, null);
    let filepath = AppsUtils.getFilePath(uri.path);
    let eliminatedUri = Services.io.newURI(uri.prePath + filepath, null, null);
    let equalCriterion = aUri => aUri.equals(eliminatedUri);
    return this.widgetPages.find(equalCriterion) !== undefined;
  },

  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.mozIApplication) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
}

function _setAppProperties(aObj, aApp) {
  aObj.name = aApp.name;
  aObj.csp = aApp.csp;
  aObj.installOrigin = aApp.installOrigin;
  aObj.origin = aApp.origin;
#ifdef MOZ_WIDGET_ANDROID
  aObj.apkPackageName = aApp.apkPackageName;
#endif
  aObj.receipts = aApp.receipts ? JSON.parse(JSON.stringify(aApp.receipts)) : null;
  aObj.installTime = aApp.installTime;
  aObj.manifestURL = aApp.manifestURL;
  aObj.appStatus = aApp.appStatus;
  aObj.removable = aApp.removable;
  aObj.id = aApp.id;
  aObj.localId = aApp.localId;
  aObj.basePath = aApp.basePath;
  aObj.progress = aApp.progress || 0.0;
  aObj.installState = aApp.installState || "installed";
  aObj.downloadAvailable = aApp.downloadAvailable;
  aObj.downloading = aApp.downloading;
  aObj.readyToApplyDownload = aApp.readyToApplyDownload;
  aObj.downloadSize = aApp.downloadSize || 0;
  aObj.lastUpdateCheck = aApp.lastUpdateCheck;
  aObj.updateTime = aApp.updateTime;
  aObj.etag = aApp.etag;
  aObj.packageEtag = aApp.packageEtag;
  aObj.manifestHash = aApp.manifestHash;
  aObj.packageHash = aApp.packageHash;
  aObj.staged = aApp.staged;
  aObj.installerAppId = aApp.installerAppId || Ci.nsIScriptSecurityManager.NO_APP_ID;
  aObj.installerIsBrowser = !!aApp.installerIsBrowser;
  aObj.storeId = aApp.storeId || "";
  aObj.storeVersion = aApp.storeVersion || 0;
  aObj.role = aApp.role || "";
  aObj.redirects = aApp.redirects;
  aObj.widgetPages = aApp.widgetPages || [];
  aObj.kind = aApp.kind;
  aObj.enabled = aApp.enabled !== undefined ? aApp.enabled : true;
  aObj.sideloaded = aApp.sideloaded;
}

this.AppsUtils = {
  
  cloneAppObject: function(aApp) {
    let obj = {};
    _setAppProperties(obj, aApp);
    return obj;
  },

  
  createLoadContext: function createLoadContext(aAppId, aIsBrowser) {
    return {
       associatedWindow: null,
       topWindow : null,
       appId: aAppId,
       isInBrowserElement: aIsBrowser,
       usePrivateBrowsing: false,
       isContent: false,

       isAppOfType: function(appType) {
         throw Cr.NS_ERROR_NOT_IMPLEMENTED;
       },

       QueryInterface: XPCOMUtils.generateQI([Ci.nsILoadContext,
                                              Ci.nsIInterfaceRequestor,
                                              Ci.nsISupports]),
       getInterface: function(iid) {
         if (iid.equals(Ci.nsILoadContext))
           return this;
         throw Cr.NS_ERROR_NO_INTERFACE;
       }
     }
  },

  
  
  getFile: function(aRequestChannel, aId, aFileName) {
    let deferred = Promise.defer();

    
    let file = FileUtils.getFile("TmpD", ["webapps", aId, aFileName], true);

    
    let outputStream = Cc["@mozilla.org/network/file-output-stream;1"]
                         .createInstance(Ci.nsIFileOutputStream);
    
    outputStream.init(file, 0x02 | 0x08 | 0x20, parseInt("0664", 8), 0);
    let bufferedOutputStream =
      Cc['@mozilla.org/network/buffered-output-stream;1']
        .createInstance(Ci.nsIBufferedOutputStream);
    bufferedOutputStream.init(outputStream, 1024);

    
    let listener = Cc["@mozilla.org/network/simple-stream-listener;1"]
                     .createInstance(Ci.nsISimpleStreamListener);

    listener.init(bufferedOutputStream, {
      onStartRequest: function(aRequest, aContext) {
        
      },

      onStopRequest: function(aRequest, aContext, aStatusCode) {
        bufferedOutputStream.close();
        outputStream.close();

        if (!Components.isSuccessCode(aStatusCode)) {
          deferred.reject({ msg: "NETWORK_ERROR", downloadAvailable: true});
          return;
        }

        
        
        let responseStatus = aRequestChannel.responseStatus;
        if (responseStatus >= 400 && responseStatus <= 599) {
          
          deferred.reject({ msg: "NETWORK_ERROR", downloadAvailable: false});
          return;
        }

        deferred.resolve(file);
      }
    });
    aRequestChannel.asyncOpen(listener, null);

    return deferred.promise;
  },

  
  getFilePath: function(aPagePath) {
    let urlParser = Cc["@mozilla.org/network/url-parser;1?auth=no"]
                    .getService(Ci.nsIURLParser);
    let uriData = [aPagePath, aPagePath.length, {}, {}, {}, {}, {}, {}];
    urlParser.parsePath.apply(urlParser, uriData);
    let [{value: pathPos}, {value: pathLen}] = uriData.slice(2, 4);
    return aPagePath.substr(pathPos, pathLen);
  },

  getAppByManifestURL: function getAppByManifestURL(aApps, aManifestURL) {
    debug("getAppByManifestURL " + aManifestURL);
    
    
    
    for (let id in aApps) {
      let app = aApps[id];
      if (app.manifestURL == aManifestURL) {
        return new mozIApplication(app);
      }
    }

    return null;
  },

  getManifestFor: function getManifestFor(aManifestURL) {
    debug("getManifestFor(" + aManifestURL + ")");
    return DOMApplicationRegistry.getManifestFor(aManifestURL);
  },

  getAppLocalIdByManifestURL: function getAppLocalIdByManifestURL(aApps, aManifestURL) {
    debug("getAppLocalIdByManifestURL " + aManifestURL);
    for (let id in aApps) {
      if (aApps[id].manifestURL == aManifestURL) {
        return aApps[id].localId;
      }
    }

    return Ci.nsIScriptSecurityManager.NO_APP_ID;
  },

  getAppLocalIdByStoreId: function(aApps, aStoreId) {
    debug("getAppLocalIdByStoreId:" + aStoreId);
    for (let id in aApps) {
      if (aApps[id].storeId == aStoreId) {
        return aApps[id].localId;
      }
    }

    return Ci.nsIScriptSecurityManager.NO_APP_ID;
  },

  getManifestCSPByLocalId: function getManifestCSPByLocalId(aApps, aLocalId) {
    debug("getManifestCSPByLocalId " + aLocalId);
    for (let id in aApps) {
      let app = aApps[id];
      if (app.localId == aLocalId) {
        return ( app.csp || "" );
      }
    }

    return "";
  },

  getDefaultCSPByLocalId: function(aApps, aLocalId) {
    debug("getDefaultCSPByLocalId " + aLocalId);
    for (let id in aApps) {
      let app = aApps[id];
      if (app.localId == aLocalId) {
        
        try {
          switch (app.appStatus) {
            case Ci.nsIPrincipal.APP_STATUS_CERTIFIED:
              return Services.prefs.getCharPref("security.apps.certified.CSP.default");
              break;
            case Ci.nsIPrincipal.APP_STATUS_PRIVILEGED:
              return Services.prefs.getCharPref("security.apps.privileged.CSP.default");
              break;
            case Ci.nsIPrincipal.APP_STATUS_INSTALLED:
              return app.kind == "hosted-trusted"
                ? Services.prefs.getCharPref("security.apps.trusted.CSP.default")
                : "";
              break;
          }
        } catch(e) {}
      }
    }

    return "default-src 'self'; object-src 'none'";
  },

  getAppByLocalId: function getAppByLocalId(aApps, aLocalId) {
    debug("getAppByLocalId " + aLocalId);
    for (let id in aApps) {
      let app = aApps[id];
      if (app.localId == aLocalId) {
        return new mozIApplication(app);
      }
    }

    return null;
  },

  getManifestURLByLocalId: function getManifestURLByLocalId(aApps, aLocalId) {
    debug("getManifestURLByLocalId " + aLocalId);
    for (let id in aApps) {
      let app = aApps[id];
      if (app.localId == aLocalId) {
        return app.manifestURL;
      }
    }

    return "";
  },

  getCoreAppsBasePath: function getCoreAppsBasePath() {
    debug("getCoreAppsBasePath()");
    try {
      return FileUtils.getDir("coreAppsDir", ["webapps"], false).path;
    } catch(e) {
      return null;
    }
  },

  getAppInfo: function getAppInfo(aApps, aAppId) {
    let app = aApps[aAppId];

    if (!app) {
      debug("No webapp for " + aAppId);
      return null;
    }

    
    
    
    let isCoreApp = false;

#ifdef MOZ_WIDGET_GONK
    isCoreApp = app.basePath == this.getCoreAppsBasePath();
#endif
    debug(app.basePath + " isCoreApp: " + isCoreApp);

    
    
    let prefName = "dom.mozApps.auto_confirm_install";
    if (Services.prefs.prefHasUserValue(prefName) &&
        Services.prefs.getBoolPref(prefName)) {
      return { "path": app.basePath + "/" + app.id,
               "isCoreApp": isCoreApp };
    }

    return { "path": WebappOSUtils.getPackagePath(app),
             "isCoreApp": isCoreApp };
  },

  



  sanitizeManifest: function(aManifest) {
    let sanitizer = Cc["@mozilla.org/parserutils;1"]
                      .getService(Ci.nsIParserUtils);
    if (!sanitizer) {
      return;
    }

    function sanitize(aStr) {
      return sanitizer.convertToPlainText(aStr,
               Ci.nsIDocumentEncoder.OutputRaw, 0);
    }

    function sanitizeEntryPoint(aRoot) {
      aRoot.name = sanitize(aRoot.name);

      if (aRoot.description) {
        aRoot.description = sanitize(aRoot.description);
      }

      if (aRoot.developer && aRoot.developer.name) {
        aRoot.developer.name = sanitize(aRoot.developer.name);
      }

      if (aRoot.permissions) {
        for (let permission in aRoot.permissions) {
          if (aRoot.permissions[permission].description) {
            aRoot.permissions[permission].description =
             sanitize(aRoot.permissions[permission].description);
          }
        }
      }
    }

    
    sanitizeEntryPoint(aManifest);

    if (aManifest.entry_points) {
      for (let entry in aManifest.entry_points) {
        sanitizeEntryPoint(aManifest.entry_points[entry]);
      }
    }
  },

  



  checkManifest: function(aManifest, app) {
    if (aManifest.name == undefined)
      return false;

    this.sanitizeManifest(aManifest);

    
    if (aManifest.launch_path && isAbsoluteURI(aManifest.launch_path))
      return false;

    function checkAbsoluteEntryPoints(entryPoints) {
      for (let name in entryPoints) {
        if (entryPoints[name].launch_path && isAbsoluteURI(entryPoints[name].launch_path)) {
          return true;
        }
      }
      return false;
    }

    if (checkAbsoluteEntryPoints(aManifest.entry_points))
      return false;

    for (let localeName in aManifest.locales) {
      if (checkAbsoluteEntryPoints(aManifest.locales[localeName].entry_points)) {
        return false;
      }
    }

    if (aManifest.activities) {
      for (let activityName in aManifest.activities) {
        let activity = aManifest.activities[activityName];
        if (activity.href && isAbsoluteURI(activity.href)) {
          return false;
        }
      }
    }

    
    
    let messages = aManifest.messages;
    if (messages) {
      if (!Array.isArray(messages)) {
        return false;
      }
      for (let item of aManifest.messages) {
        if (typeof item == "object") {
          let keys = Object.keys(item);
          if (keys.length != 1) {
            return false;
          }
          if (isAbsoluteURI(item[keys[0]])) {
            return false;
          }
        }
      }
    }

    
    if (aManifest.size) {
      aManifest.size = parseInt(aManifest.size);
      if (Number.isNaN(aManifest.size) || aManifest.size < 0) {
        return false;
      }
    }

    
    if (aManifest.role && (typeof aManifest.role !== "string")) {
      return false;
    }
    return true;
  },

  checkManifestContentType: function
     checkManifestContentType(aInstallOrigin, aWebappOrigin, aContentType) {
    let hadCharset = { };
    let charset = { };
    let netutil = Cc["@mozilla.org/network/util;1"].getService(Ci.nsINetUtil);
    let contentType = netutil.parseContentType(aContentType, charset, hadCharset);
    if (aInstallOrigin != aWebappOrigin &&
        !(contentType == "application/x-web-app-manifest+json" ||
          contentType == "application/manifest+json")) {
      return false;
    }
    return true;
  },

  allowUnsignedAddons: false, 

  






  checkAppRole: function(aRole, aStatus) {
    if (aRole == "theme" && aStatus !== Ci.nsIPrincipal.APP_STATUS_CERTIFIED) {
      return false;
    }
    if (!this.allowUnsignedAddons &&
        (aRole == "addon" &&
         aStatus !== Ci.nsIPrincipal.APP_STATUS_CERTIFIED &&
         aStatus !== Ci.nsIPrincipal.APP_STATUS_PRIVILEGED)) {
      return false;
    }
    return true;
  },

  



  ensureSameAppName: function ensureSameAppName(aOldManifest, aNewManifest, aApp) {
    
    aNewManifest.name = aApp.name;

    let defaultShortName =
      new ManifestHelper(aOldManifest, aApp.origin, aApp.manifestURL).short_name;
    aNewManifest.short_name = defaultShortName;

    
    if ("locales" in aNewManifest) {
      for (let locale in aNewManifest.locales) {
        let newLocaleEntry = aNewManifest.locales[locale];

        let oldLocaleEntry = aOldManifest && "locales" in aOldManifest &&
            locale in aOldManifest.locales && aOldManifest.locales[locale];

        if (newLocaleEntry.name) {
          
          
          newLocaleEntry.name =
            (oldLocaleEntry && oldLocaleEntry.name) || aApp.name;
        }
        if (newLocaleEntry.short_name) {
          newLocaleEntry.short_name =
            (oldLocaleEntry && oldLocaleEntry.short_name) || defaultShortName;
        }
      }
    }
  },

  





  checkInstallAllowed: function checkInstallAllowed(aManifest, aInstallOrigin) {
    if (!aManifest.installs_allowed_from) {
      return true;
    }

    function cbCheckAllowedOrigin(aOrigin) {
      return aOrigin == "*" || aOrigin == aInstallOrigin;
    }

    return aManifest.installs_allowed_from.some(cbCheckAllowedOrigin);
  },

  





  getAppManifestStatus: function getAppManifestStatus(aManifest) {
    let type = aManifest.type || "web";

    switch(type) {
    case "web":
    case "trusted":
      return Ci.nsIPrincipal.APP_STATUS_INSTALLED;
    case "privileged":
      return Ci.nsIPrincipal.APP_STATUS_PRIVILEGED;
    case "certified":
      return Ci.nsIPrincipal.APP_STATUS_CERTIFIED;
    default:
      throw new Error("Webapps.jsm: Undetermined app manifest type");
    }
  },

  


  isFirstRun: function isFirstRun(aPrefBranch) {
    let savedmstone = null;
    try {
      savedmstone = aPrefBranch.getCharPref("gecko.mstone");
    } catch (e) {}

    let mstone = Services.appinfo.platformVersion;

    let savedBuildID = null;
    try {
      savedBuildID = aPrefBranch.getCharPref("gecko.buildID");
    } catch (e) {}

    let buildID = Services.appinfo.platformBuildID;

    aPrefBranch.setCharPref("gecko.mstone", mstone);
    aPrefBranch.setCharPref("gecko.buildID", buildID);

    if ((mstone != savedmstone) || (buildID != savedBuildID)) {
      aPrefBranch.setBoolPref("dom.apps.reset-permissions", false);
      return true;
    } else {
      return false;
    }
  },

  




  compareManifests: function compareManifests(aManifest1, aManifest2) {
    
    let locales1 = [];
    let locales2 = [];
    if (aManifest1.locales) {
      for (let locale in aManifest1.locales) {
        locales1.push(locale);
      }
    }
    if (aManifest2.locales) {
      for (let locale in aManifest2.locales) {
        locales2.push(locale);
      }
    }
    if (locales1.sort().join() !== locales2.sort().join()) {
      return false;
    }

    
    
    let checkNameAndDev = function(aRoot1, aRoot2) {
      let name1 = aRoot1.name;
      let name2 = aRoot2.name;
      if (name1 !== name2) {
        return false;
      }

      let dev1 = aRoot1.developer;
      let dev2 = aRoot2.developer;
      if ((dev1 && !dev2) || (dev2 && !dev1)) {
        return false;
      }

      return (!dev1 && !dev2) ||
             (dev1.name === dev2.name && dev1.url === dev2.url);
    }

    
    if (!checkNameAndDev(aManifest1, aManifest2)) {
      return false;
    }

    for (let locale in aManifest1.locales) {
      if (!checkNameAndDev(aManifest1.locales[locale],
                           aManifest2.locales[locale])) {
        return false;
      }
    }

    
    return true;
  },

  
  
  loadJSONAsync: function(aPath) {
    let deferred = Promise.defer();

    try {
      let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
      file.initWithPath(aPath);

      let channel = NetUtil.newChannel2(file,
                                        null,
                                        null,
                                        null,      
                                        Services.scriptSecurityManager.getSystemPrincipal(),
                                        null,      
                                        Ci.nsILoadInfo.SEC_NORMAL,
                                        Ci.nsIContentPolicy.TYPE_OTHER);
      channel.contentType = "application/json";

      NetUtil.asyncFetch2(channel, function(aStream, aResult) {
        if (!Components.isSuccessCode(aResult)) {
          deferred.resolve(null);

          if (aResult == Cr.NS_ERROR_FILE_NOT_FOUND) {
            
            
            return;
          }

          Cu.reportError("AppsUtils: Could not read from json file " + aPath);
          return;
        }

        try {
          
          let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                            .createInstance(Ci.nsIScriptableUnicodeConverter);
          converter.charset = "UTF-8";

          
          let data = JSON.parse(converter.ConvertToUnicode(NetUtil.readInputStreamToString(aStream,
                                                            aStream.available()) || ""));
          aStream.close();

          deferred.resolve(data);
        } catch (ex) {
          Cu.reportError("AppsUtils: Could not parse JSON: " +
                         aPath + " " + ex + "\n" + ex.stack);
          deferred.resolve(null);
        }
      });
    } catch (ex) {
      Cu.reportError("AppsUtils: Could not read from " +
                     aPath + " : " + ex + "\n" + ex.stack);
      deferred.resolve(null);
    }

    return deferred.promise;
  },

  
  computeHash: function(aString) {
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                      .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let result = {};
    
    let data = converter.convertToByteArray(aString, result);

    let hasher = Cc["@mozilla.org/security/hash;1"]
                   .createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.MD5);
    hasher.update(data, data.length);
    
    let hash = hasher.finish(false);

    function toHexString(charCode) {
      return ("0" + charCode.toString(16)).slice(-2);
    }

    
    return [toHexString(hash.charCodeAt(i)) for (i in hash)].join("");
  },

  
  computeObjectHash: function(aObject) {
    return this.computeHash(JSON.stringify(aObject));
  },

  getAppManifestURLFromWindow: function(aWindow) {
    let appId = aWindow.document.nodePrincipal.appId;
    if (appId === Ci.nsIScriptSecurityManager.NO_APP_ID) {
      return null;
    }

    return appsService.getManifestURLByLocalId(appId);
  },
}




this.ManifestHelper = function(aManifest, aOrigin, aManifestURL, aLang) {
  
  

  if (!aOrigin || !aManifestURL) {
    throw Error("ManifestHelper needs both origin and manifestURL");
  }

  this._baseURI = Services.io.newURI(
    aOrigin.startsWith("app://") ? aOrigin : aManifestURL, null, null);

  
  
  this._manifestURL = Services.io.newURI(aManifestURL, null, null);

  this._manifest = aManifest;

  let locale = aLang;
  if (!locale) {
    let chrome = Cc["@mozilla.org/chrome/chrome-registry;1"]
                   .getService(Ci.nsIXULChromeRegistry)
                   .QueryInterface(Ci.nsIToolkitChromeRegistry);
    locale = chrome.getSelectedLocale("global").toLowerCase();
  }

  this._localeRoot = this._manifest;

  if (this._manifest.locales && this._manifest.locales[locale]) {
    this._localeRoot = this._manifest.locales[locale];
  }
  else if (this._manifest.locales) {
    
    let lang = locale.split('-')[0];
    if (lang != locale && this._manifest.locales[lang])
      this._localeRoot = this._manifest.locales[lang];
  }
};

ManifestHelper.prototype = {
  _localeProp: function(aProp) {
    if (this._localeRoot[aProp] != undefined)
      return this._localeRoot[aProp];
    return this._manifest[aProp];
  },

  get name() {
    return this._localeProp("name");
  },

  get short_name() {
    return this._localeProp("short_name");
  },

  get description() {
    return this._localeProp("description");
  },

  get type() {
    return this._localeProp("type");
  },

  get version() {
    return this._localeProp("version");
  },

  get launch_path() {
    return this._localeProp("launch_path");
  },

  get developer() {
    
    
    return this._localeProp("developer") || {};
  },

  get icons() {
    return this._localeProp("icons");
  },

  get appcache_path() {
    return this._localeProp("appcache_path");
  },

  get orientation() {
    return this._localeProp("orientation");
  },

  get package_path() {
    return this._localeProp("package_path");
  },

  get widgetPages() {
    return this._localeProp("widgetPages");
  },

  get size() {
    return this._manifest["size"] || 0;
  },

  get permissions() {
    if (this._manifest.permissions) {
      return this._manifest.permissions;
    }
    return {};
  },

  get biggestIconURL() {
    let icons = this._localeProp("icons");
    if (!icons) {
      return null;
    }

    let iconSizes = Object.keys(icons);
    if (iconSizes.length == 0) {
      return null;
    }

    iconSizes.sort((a, b) => a - b);
    let biggestIconSize = iconSizes.pop();
    let biggestIcon = icons[biggestIconSize];
    let biggestIconURL = this._baseURI.resolve(biggestIcon);

    return biggestIconURL;
  },

  iconURLForSize: function(aSize) {
    let icons = this._localeProp("icons");
    if (!icons)
      return null;
    let dist = 100000;
    let icon = null;
    for (let size in icons) {
      let iSize = parseInt(size);
      if (Math.abs(iSize - aSize) < dist) {
        icon = this._baseURI.resolve(icons[size]);
        dist = Math.abs(iSize - aSize);
      }
    }
    return icon;
  },

  fullLaunchPath: function(aStartPoint) {
    
    
    if ((aStartPoint || "") === "") {
      
      if (this._localeProp("start_url")) {
        return this._baseURI.resolve(this._localeProp("start_url") || "/");
      }
      return this._baseURI.resolve(this._localeProp("launch_path") || "/");
    }

    
    let entryPoints = this._localeProp("entry_points");
    if (!entryPoints) {
      return null;
    }

    if (entryPoints[aStartPoint]) {
      return this._baseURI.resolve(entryPoints[aStartPoint].launch_path || "/");
    }

    return null;
  },

  resolveURL: function(aURI) {
    
    if (isAbsoluteURI(aURI)) {
      throw new Error("Webapps.jsm: non-relative URI passed to resolve");
    }
    return this._baseURI.resolve(aURI);
  },

  fullAppcachePath: function() {
    let appcachePath = this._localeProp("appcache_path");
    return this._baseURI.resolve(appcachePath ? appcachePath : "/");
  },

  fullPackagePath: function() {
    let packagePath = this._localeProp("package_path");
    return this._manifestURL.resolve(packagePath ? packagePath : "/");
  },

  get role() {
    return this._manifest.role || "";
  },

  get csp() {
    return this._manifest.csp || "";
  }
}
