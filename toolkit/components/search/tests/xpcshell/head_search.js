


const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://testing-common/AppInfo.jsm");
Cu.import("resource://testing-common/httpd.js");

const BROWSER_SEARCH_PREF = "browser.search.";
const NS_APP_SEARCH_DIR = "SrchPlugns";

const MODE_RDONLY = FileUtils.MODE_RDONLY;
const MODE_WRONLY = FileUtils.MODE_WRONLY;
const MODE_CREATE = FileUtils.MODE_CREATE;
const MODE_TRUNCATE = FileUtils.MODE_TRUNCATE;


var XULRuntime = Components.classesByID["{95d89e3e-a169-41a3-8e56-719978e15b12}"]
                           .getService(Ci.nsIXULRuntime);

var XULAppInfo = {
  vendor: "Mozilla",
  name: "XPCShell",
  ID: "xpcshell@test.mozilla.org",
  version: "5",
  appBuildID: "2007010101",
  platformVersion: "1.9",
  platformBuildID: "2007010101",
  inSafeMode: false,
  logConsoleErrors: true,
  
  OS: XULRuntime.OS,
  XPCOMABI: "noarch-spidermonkey",
  
  processType: XULRuntime.processType,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIXULAppInfo, Ci.nsIXULRuntime,
                                         Ci.nsISupports])
};

var XULAppInfoFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return XULAppInfo.QueryInterface(iid);
  }
};

var isChild = XULRuntime.processType == XULRuntime.PROCESS_TYPE_CONTENT;

Components.manager.QueryInterface(Ci.nsIComponentRegistrar)
          .registerFactory(Components.ID("{ecff8849-cee8-40a7-bd4a-3f4fdfeddb5c}"),
                           "XULAppInfo", "@mozilla.org/xre/app-info;1",
                           XULAppInfoFactory);

var gProfD;
if (!isChild) {
  
  gProfD = do_get_profile();
}

function dumpn(text)
{
  dump("search test: " + text + "\n");
}






function configureToLoadJarEngines(loadFromJars = true)
{
  let defaultBranch = Services.prefs.getDefaultBranch(null);

  let url = "chrome://testsearchplugin/locale/searchplugins/";
  defaultBranch.setCharPref("browser.search.jarURIs", url);

  defaultBranch.setBoolPref("browser.search.loadFromJars", loadFromJars);

  
  
  Services.prefs.setBoolPref("browser.search.loadFromJars", !loadFromJars)

  
  let dir = Services.dirsvc.get(NS_APP_SEARCH_DIR, Ci.nsIFile);
  if (!dir.exists())
    dir.create(dir.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
  do_get_file("data/engine-app.xml").copyTo(dir, "app.xml");
}





function installAddonEngine(name = "engine-addon")
{
  const XRE_EXTENSIONS_DIR_LIST = "XREExtDL";
  const gProfD = do_get_profile().QueryInterface(Ci.nsILocalFile);

  let dir = gProfD.clone();
  dir.append("extensions");
  if (!dir.exists())
    dir.create(dir.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  dir.append("search-engine@tests.mozilla.org");
  dir.create(dir.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  do_get_file("data/install.rdf").copyTo(dir, "install.rdf");
  let addonDir = dir.clone();
  dir.append("searchplugins");
  dir.create(dir.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
  do_get_file("data/" + name + ".xml").copyTo(dir, "bug645970.xml");

  Services.dirsvc.registerProvider({
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider,
                                           Ci.nsIDirectoryServiceProvider2]),

    getFile: function (prop, persistant) {
      throw Cr.NS_ERROR_FAILURE;
    },

    getFiles: function (prop) {
      let result = [];

      switch (prop) {
      case XRE_EXTENSIONS_DIR_LIST:
        result.push(addonDir);
        break;
      default:
        throw Cr.NS_ERROR_FAILURE;
      }

      return {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsISimpleEnumerator]),
        hasMoreElements: () => result.length > 0,
        getNext: () => result.shift()
      };
    }
  });
}





function installDistributionEngine()
{
  const XRE_APP_DISTRIBUTION_DIR = "XREAppDist";

  const gProfD = do_get_profile().QueryInterface(Ci.nsILocalFile);

  let dir = gProfD.clone();
  dir.append("distribution");
  dir.create(dir.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
  let distDir = dir.clone();

  dir.append("searchplugins");
  dir.create(dir.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  dir.append("common");
  dir.create(dir.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  do_get_file("data/engine-override.xml").copyTo(dir, "bug645970.xml");

  Services.dirsvc.registerProvider({
    getFile: function(aProp, aPersistent) {
      aPersistent.value = true;
      if (aProp == XRE_APP_DISTRIBUTION_DIR)
        return distDir.clone();
      return null;
    }
  });
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

function getSearchMetadata()
{
  
  let metadata = gProfD.clone();
  metadata.append("search-metadata.json");
  do_check_true(metadata.exists());

  do_print("Parsing metadata");
  return readJSONFile(metadata);
}

function removeCacheFile()
{
  let file = gProfD.clone();
  file.append("search.json");
  if (file.exists()) {
    file.remove(false);
  }
}




function removeCache()
{
  let file = gProfD.clone();
  file.append("search.json");
  if (file.exists()) {
    file.remove(false);
  }

}




function isUSTimezone() {
  
  

  
  

  
  

  
  

  let UTCOffset = (new Date()).getTimezoneOffset();
  return UTCOffset >= 150 && UTCOffset <= 600;
}





function promiseAfterCommit() {
  return waitForSearchNotification("write-metadata-to-disk-complete");
}





function promiseAfterCache() {
  return waitForSearchNotification("write-cache-to-disk-complete");
}

function parseJsonFromStream(aInputStream) {
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



if (!isChild) {
  
  Services.prefs.setBoolPref("browser.search.log", true);

  
  
  Services.prefs.setBoolPref("browser.search.geoSpecificDefaults", true);
  Services.prefs.setIntPref("browser.search.geoip.timeout", 2000);
  
  Services.prefs.setCharPref("browser.search.geoip.url", "");
}





let gDataUrl;






function useHttpServer() {
  let httpServer = new HttpServer();
  httpServer.start(-1);
  httpServer.registerDirectory("/", do_get_cwd());
  gDataUrl = "http://localhost:" + httpServer.identity.primaryPort + "/data/";
  do_register_cleanup(() => httpServer.stop(() => {}));
  return httpServer;
}

















let addTestEngines = Task.async(function* (aItems) {
  if (!gDataUrl) {
    do_throw("useHttpServer must be called before addTestEngines.");
  }

  let engines = [];

  for (let item of aItems) {
    do_print("Adding engine: " + item.name);
    yield new Promise((resolve, reject) => {
      Services.obs.addObserver(function obs(subject, topic, data) {
        try {
          let engine = subject.QueryInterface(Ci.nsISearchEngine);
          do_print("Observed " + data + " for " + engine.name);
          if (data != "engine-added" || engine.name != item.name) {
            return;
          }

          Services.obs.removeObserver(obs, "browser-search-engine-modified");
          engines.push(engine);
          resolve();
        } catch (ex) {
          reject(ex);
        }
      }, "browser-search-engine-modified", false);

      if (item.xmlFileName) {
        Services.search.addEngine(gDataUrl + item.xmlFileName,
                                  Ci.nsISearchEngine.DATA_XML, null, false);
      } else if (item.srcFileName) {
        Services.search.addEngine(gDataUrl + item.srcFileName,
                                  Ci.nsISearchEngine.DATA_TEXT,
                                  gDataUrl + item.iconFileName, false);
      } else {
        Services.search.addEngineWithDetails(item.name, ...item.details);
      }
    });
  }

  return engines;
});




function installTestEngine() {
  removeMetadata();
  removeCacheFile();

  do_check_false(Services.search.isInitialized);

  let engineDummyFile = gProfD.clone();
  engineDummyFile.append("searchplugins");
  engineDummyFile.append("test-search-engine.xml");
  let engineDir = engineDummyFile.parent;
  engineDir.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  do_get_file("data/engine.xml").copyTo(engineDir, "engine.xml");

  do_register_cleanup(function() {
    removeMetadata();
    removeCacheFile();
  });
}






function setLocalizedPref(aPrefName, aValue) {
  const nsIPLS = Ci.nsIPrefLocalizedString;
  try {
    var pls = Components.classes["@mozilla.org/pref-localizedstring;1"]
                        .createInstance(Ci.nsIPrefLocalizedString);
    pls.data = aValue;
    Services.prefs.setComplexValue(aPrefName, nsIPLS, pls);
  } catch (ex) {}
}





function setUpGeoDefaults() {
  removeMetadata();
  removeCacheFile();

  do_check_false(Services.search.isInitialized);

  let engineDummyFile = gProfD.clone();
  engineDummyFile.append("searchplugins");
  engineDummyFile.append("test-search-engine.xml");
  let engineDir = engineDummyFile.parent;
  engineDir.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  do_get_file("data/engine.xml").copyTo(engineDir, "engine.xml");

  engineDummyFile = gProfD.clone();
  engineDummyFile.append("searchplugins");
  engineDummyFile.append("test-search-engine2.xml");

  do_get_file("data/engine2.xml").copyTo(engineDir, "engine2.xml");

  setLocalizedPref("browser.search.defaultenginename",    "Test search engine");
  setLocalizedPref("browser.search.defaultenginename.US", "A second test engine");

  do_register_cleanup(function() {
    removeMetadata();
    removeCacheFile();
  });
}









function waitForSearchNotification(aExpectedData) {
  return new Promise(resolve => {
    const SEARCH_SERVICE_TOPIC = "browser-search-service";
    Services.obs.addObserver(function observer(aSubject, aTopic, aData) {
      if (aData != aExpectedData)
        return;

      Services.obs.removeObserver(observer, SEARCH_SERVICE_TOPIC);
      resolve(aSubject);
    }, SEARCH_SERVICE_TOPIC, false);
  });
}


const TELEMETRY_RESULT_ENUM = {
  SUCCESS: 0,
  SUCCESS_WITHOUT_DATA: 1,
  XHRTIMEOUT: 2,
  ERROR: 3,
};









function checkCountryResultTelemetry(aExpectedValue) {
  let histogram = Services.telemetry.getHistogramById("SEARCH_SERVICE_COUNTRY_FETCH_RESULT");
  let snapshot = histogram.snapshot();
  
  let expectedCounts = [0,0,0,0,0,0,0,0,0];
  if (aExpectedValue != null) {
    expectedCounts[aExpectedValue] = 1;
  }
  deepEqual(snapshot.counts, expectedCounts);
}
