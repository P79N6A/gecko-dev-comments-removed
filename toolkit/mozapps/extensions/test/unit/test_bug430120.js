





































const BLOCKLIST_TIMER                 = "blocklist-background-update-timer";
const PREF_BLOCKLIST_URL              = "extensions.blocklist.url";
const PREF_BLOCKLIST_ENABLED          = "extensions.blocklist.enabled";
const PREF_APP_DISTRIBUTION           = "distribution.id";
const PREF_APP_DISTRIBUTION_VERSION   = "distribution.version";
const PREF_APP_UPDATE_CHANNEL         = "app.update.channel";
const PREF_GENERAL_USERAGENT_LOCALE   = "general.useragent.locale";
const CATEGORY_UPDATE_TIMER           = "update-timer";


do_load_httpd_js();
var testserver;
var gOSVersion;
var gBlocklist;


var timerService = {

  hasTimer: function(id) {
    var catMan = Components.classes["@mozilla.org/categorymanager;1"]
                           .getService(Components.interfaces.nsICategoryManager);
    var entries = catMan.enumerateCategory(CATEGORY_UPDATE_TIMER);
    while (entries.hasMoreElements()) {
      var entry = entries.getNext().QueryInterface(Components.interfaces.nsISupportsCString).data;
      var value = catMan.getCategoryEntry(CATEGORY_UPDATE_TIMER, entry);
      var timerID = value.split(",")[2];
      if (id == timerID) {
        return true;
      }
    }
    return false;
  },

  fireTimer: function(id) {
    gBlocklist.QueryInterface(Components.interfaces.nsITimerCallback).notify(null);
  },

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIUpdateTimerManager)
     || iid.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

var TimerServiceFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return timerService.QueryInterface(iid);
  }
};
var registrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{61189e7a-6b1b-44b8-ac81-f180a6105085}"), "TimerService",
                          "@mozilla.org/updates/timer-manager;1", TimerServiceFactory);

function failHandler(metadata, response) {
  do_throw("Should not have attempted to retrieve the blocklist when it is disabled");
}

function pathHandler(metadata, response) {
  do_check_eq(metadata.queryString,
              "xpcshell@tests.mozilla.org&1&XPCShell&1&2007010101&" +
              "XPCShell_noarch-spidermonkey&locale&updatechannel&" + 
              gOSVersion + "&1.9&distribution&distribution-version");
  gBlocklist.observe(null, "quit-application", "");
  gBlocklist.observe(null, "xpcom-shutdown", "");
  testserver.stop(do_test_finished);
}

function run_test() {
  var osVersion;
  var sysInfo = Components.classes["@mozilla.org/system-info;1"]
                          .getService(Components.interfaces.nsIPropertyBag2);
  try {
    osVersion = sysInfo.getProperty("name") + " " + sysInfo.getProperty("version");
    if (osVersion) {
      try {
        osVersion += " (" + sysInfo.getProperty("secondaryLibrary") + ")";
      }
      catch (e) {
      }
      gOSVersion = encodeURIComponent(osVersion);
    }
  }
  catch (e) {
  }

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  testserver = new nsHttpServer();
  testserver.registerPathHandler("/1", failHandler);
  testserver.registerPathHandler("/2", pathHandler);
  testserver.start(4444);

  
  gBlocklist = Components.classes["@mozilla.org/extensions/blocklist;1"]
                         .getService(Components.interfaces.nsIBlocklistService)
                         .QueryInterface(Components.interfaces.nsIObserver);
  gBlocklist.observe(null, "profile-after-change", "");

  do_check_true(timerService.hasTimer(BLOCKLIST_TIMER));

  do_test_pending();

  
  gPrefs.setCharPref(PREF_BLOCKLIST_URL, "http://localhost:4444/1");
  gPrefs.setBoolPref(PREF_BLOCKLIST_ENABLED, false);
  timerService.fireTimer(BLOCKLIST_TIMER);

  
  var defaults = gPrefs.QueryInterface(Components.interfaces.nsIPrefService)
                       .getDefaultBranch(null);
  defaults.setCharPref(PREF_APP_UPDATE_CHANNEL, "updatechannel");
  defaults.setCharPref(PREF_APP_DISTRIBUTION, "distribution");
  defaults.setCharPref(PREF_APP_DISTRIBUTION_VERSION, "distribution-version");
  defaults.setCharPref(PREF_GENERAL_USERAGENT_LOCALE, "locale");

  
  gPrefs.setCharPref(PREF_BLOCKLIST_URL, "http://localhost:4444/2?" + 
                     "%APP_ID%&%APP_VERSION%&%PRODUCT%&%VERSION%&%BUILD_ID%&" +
                     "%BUILD_TARGET%&%LOCALE%&%CHANNEL%&" + 
                     "%OS_VERSION%&%PLATFORM_VERSION%&%DISTRIBUTION%&%DISTRIBUTION_VERSION%");
  gPrefs.setBoolPref(PREF_BLOCKLIST_ENABLED, true);
  timerService.fireTimer(BLOCKLIST_TIMER);
}
