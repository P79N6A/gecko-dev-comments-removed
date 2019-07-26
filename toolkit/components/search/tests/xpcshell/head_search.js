


Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");

Components.utils.import("resource://testing-common/AppInfo.jsm");

const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");

const BROWSER_SEARCH_PREF = "browser.search.";
const NS_APP_SEARCH_DIR = "SrchPlugns";

const MODE_RDONLY = FileUtils.MODE_RDONLY;
const MODE_WRONLY = FileUtils.MODE_WRONLY;
const MODE_CREATE = FileUtils.MODE_CREATE;
const MODE_TRUNCATE = FileUtils.MODE_TRUNCATE;

var gXULAppInfo = null;












function createAppInfo(id, name, version, platformVersion)
{
  gXULAppInfo = {
    vendor: "Mozilla",
    name: name,
    ID: id,
    version: version,
    appBuildID: "2007010101",
    platformVersion: platformVersion,
    platformBuildID: "2007010101",
    inSafeMode: false,
    logConsoleErrors: true,
    OS: "XPCShell",
    XPCOMABI: "noarch-spidermonkey",
    invalidateCachesOnRestart: function invalidateCachesOnRestart() {},

    QueryInterface: function QueryInterface(iid) {
      if (iid.equals(Components.interfaces.nsIXULAppInfo)
       || iid.equals(Components.interfaces.nsIXULRuntime)
       || iid.equals(Components.interfaces.nsISupports))
        return this;

      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  };

  var XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return gXULAppInfo.QueryInterface(iid);
    }
  };
  var registrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}


var gProfD = do_get_profile();

function dumpn(text)
{
  dump("search test: " + text + "\n");
}




function removeMetadata()
{
  let file = gProfD.clone();
  file.append("search-metadata.json");
  if (file.exists()) {
    file.remove(false);
  }

  file = gProfD.clone();
  file.append("search.sqlite");
  if (file.exists()) {
    file.remove(false);
  }
}

function removeCacheFile()
{
  let file = gProfD.clone();
  file.append("search.json");
  if (file.exists()) {
    file.remove(false);
  }
}




function afterCommit(callback)
{
  let obs = function(result, topic, verb) {
    if (verb == "write-metadata-to-disk-complete") {
      Services.obs.removeObserver(obs, topic);
      callback(result);
    } else {
      dump("TOPIC: " + topic+ "\n");
    }
  }
  Services.obs.addObserver(obs, "browser-search-service", false);
}

function  parseJsonFromStream(aInputStream) {
  const json = Cc["@mozilla.org/dom/json;1"].createInstance(Components.interfaces.nsIJSON);
  const data = json.decodeFromStream(aInputStream, aInputStream.available());
  return data;
}




function readJSONFile(aFile) {
  let stream = Cc["@mozilla.org/network/file-input-stream;1"].
               createInstance(Ci.nsIFileInputStream);
  try {
    stream.init(aFile, MODE_RDONLY, FileUtils.PERMS_FILE, 0);
    return parseJsonFromStream(stream, stream.available());
  } catch(ex) {
    dumpn("readJSONFile: Error reading JSON file: " + ex);
  } finally {
    stream.close();
  }
  return false;
}





function isSubObjectOf(expectedObj, actualObj) {
  for (let prop in expectedObj) {
    if (expectedObj[prop] instanceof Object) {
      do_check_eq(expectedObj[prop].length, actualObj[prop].length);
      isSubObjectOf(expectedObj[prop], actualObj[prop]);
    } else {
      do_check_eq(expectedObj[prop], actualObj[prop]);
    }
  }
}


Services.prefs.setBoolPref("browser.search.log", true);

