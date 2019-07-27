



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/AppConstants.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");





const NS_APP_CACHE_PARENT_DIR = "cachePDir";
const NS_APP_SEARCH_DIR       = "SrchPlugns";
const NS_APP_SEARCH_DIR_LIST  = "SrchPluginsDL";
const NS_APP_USER_SEARCH_DIR  = "UsrSrchPlugns";
const NS_XPCOM_CURRENT_PROCESS_DIR = "XCurProcD";
const XRE_APP_DISTRIBUTION_DIR = "XREAppDist";
const XRE_UPDATE_ROOT_DIR     = "UpdRootD";
const ENVVAR_UPDATE_DIR       = "UPDATES_DIRECTORY";
const WEBAPPS_DIR             = "webappsDir";
const DOWNLOAD_DIR            = "DfltDwnld";

const SYSTEM_DIST_PATH = `/system/${AppConstants.ANDROID_PACKAGE_NAME}/distribution`;

function DirectoryProvider() {}

DirectoryProvider.prototype = {
  classID: Components.ID("{ef0f7a87-c1ee-45a8-8d67-26f586e46a4b}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider,
                                         Ci.nsIDirectoryServiceProvider2]),

  getFile: function(prop, persistent) {
    if (prop == NS_APP_CACHE_PARENT_DIR) {
      let dirsvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
      let profile = dirsvc.get("ProfD", Ci.nsIFile);
      return profile;
    } else if (prop == WEBAPPS_DIR) {
      
      
      
      let dirsvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
      let profile = dirsvc.get("ProfD", Ci.nsIFile);
      return profile.parent;
    } else if (prop == XRE_APP_DISTRIBUTION_DIR) {
      
      let dataDist = FileUtils.getDir(NS_XPCOM_CURRENT_PROCESS_DIR, ["distribution"], false);
      if (!dataDist.exists()) {
        
        let systemDist = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
        systemDist.initWithPath(SYSTEM_DIST_PATH);
        
        if (systemDist.exists()) {
          return systemDist;
        }
      }
      return dataDist;
    } else if (prop == XRE_UPDATE_ROOT_DIR) {
      let env = Cc["@mozilla.org/process/environment;1"].getService(Ci.nsIEnvironment);
      if (env.exists(ENVVAR_UPDATE_DIR)) {
        let path = env.get(ENVVAR_UPDATE_DIR);
        if (path) {
          return new FileUtils.File(path);
        }
      }
      return new FileUtils.File(env.get("DOWNLOADS_DIRECTORY"));
    } else if (prop == DOWNLOAD_DIR) {
      
      
      
      let env = Cc["@mozilla.org/process/environment;1"].getService(Ci.nsIEnvironment);
      return new FileUtils.File(env.get("DOWNLOADS_DIRECTORY"));
    }

    
    
    
    return null;
  },

  















  _appendDistroSearchDirs: function(array) {
    let distro = this.getFile(XRE_APP_DISTRIBUTION_DIR);
    if (!distro.exists())
      return;

    let searchPlugins = distro.clone();
    searchPlugins.append("searchplugins");
    if (!searchPlugins.exists())
      return;

    let commonPlugins = searchPlugins.clone();
    commonPlugins.append("common");
    if (commonPlugins.exists())
      array.push(commonPlugins);

    let localePlugins = searchPlugins.clone();
    localePlugins.append("locale");
    if (!localePlugins.exists())
      return;

    let curLocale = Services.prefs.getCharPref("general.useragent.locale");
    let curLocalePlugins = localePlugins.clone();
    curLocalePlugins.append(curLocale);
    if (curLocalePlugins.exists()) {
      array.push(curLocalePlugins);
      return;
    }

    
    let defLocale = Services.prefs.getCharPref("distribution.searchplugins.defaultLocale");
    let defLocalePlugins = localePlugins.clone();
    if (defLocalePlugins.exists())
      array.push(defLocalePlugins);
  },

  getFiles: function(prop) {
    if (prop != NS_APP_SEARCH_DIR_LIST)
      return;

    let result = [];

    






    this._appendDistroSearchDirs(result);

    let appUserSearchDir = FileUtils.getDir(NS_APP_USER_SEARCH_DIR, [], false);
    if (appUserSearchDir.exists())
      result.push(appUserSearchDir);

    let appSearchDir = FileUtils.getDir(NS_APP_SEARCH_DIR, [], false);
    if (appSearchDir.exists())
      result.push(appSearchDir);

    return {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsISimpleEnumerator]),
      hasMoreElements: function() {
        return result.length > 0;
      },
      getNext: function() {
        return result.shift();
      }
    };
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DirectoryProvider]);
