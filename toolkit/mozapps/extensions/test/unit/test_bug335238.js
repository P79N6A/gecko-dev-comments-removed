





































const PREF_MATCH_OS_LOCALE = "intl.locale.matchOS";
const PREF_SELECTED_LOCALE = "general.useragent.locale";


gPrefs.setBoolPref("extensions.checkUpdateSecurity", false);

do_load_httpd_js();


var EXPECTED = [
  {
    id: "bug335238_1@tests.mozilla.org",
    version: "1.3.4",
    maxAppVersion: "5",
    status: "userEnabled",
    appId: "xpcshell@tests.mozilla.org",
    appVersion: "1",
    appOs: "XPCShell",
    appAbi: "noarch-spidermonkey",
    locale: "en-US",
    reqVersion: "2"
  },
  {
    id: "bug335238_2@tests.mozilla.org",
    version: "28at",
    maxAppVersion: "7",
    status: "userDisabled,needsDependencies",
    appId: "xpcshell@tests.mozilla.org",
    appVersion: "1",
    appOs: "XPCShell",
    appAbi: "noarch-spidermonkey",
    locale: "en-US",
    reqVersion: "2"
  }
];

var ADDONS = [
  {id: "bug335238_1@tests.mozilla.org",
   addon: "test_bug335238_1"},
  {id: "bug335238_2@tests.mozilla.org",
   addon: "test_bug335238_2"}
];

var server;

var updateListener = {
  onUpdateStarted: function()
  {
  },
  
  onUpdateEnded: function()
  {
    server.stop(do_test_finished);
  },
  
  onAddonUpdateStarted: function(addon)
  {
  },
  
  onAddonUpdateEnded: function(addon, status)
  {
    
    do_check_eq(status, Ci.nsIAddonUpdateCheckListener.STATUS_FAILURE);
  }
}

var requestHandler = {
  handle: function(metadata, response)
  {
    var expected = EXPECTED[metadata.path.substring(1)];
    var params = metadata.queryString.split("&");
    do_check_eq(params.length, 10);
    for (var k in params) {
      var pair = params[k].split("=");
      var name = decodeURIComponent(pair[0]);
      var value = decodeURIComponent(pair[1]);
      do_check_eq(expected[name], value);
    }
    response.setStatusLine(metadata.httpVersion, 404, "Not Found");
  }
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");
  
  gPrefs.setBoolPref(PREF_MATCH_OS_LOCALE, false);
  gPrefs.setCharPref(PREF_SELECTED_LOCALE, "en-US");

  startupEM();
  for (var k in ADDONS)
    gEM.installItemFromFile(do_get_addon(ADDONS[k].addon),
                            NS_INSTALL_LOCATION_APPPROFILE);

  restartEM();
  gEM.disableItem(ADDONS[1].id);
  restartEM();

  var updates = [];
  for (var k in ADDONS) {
    do_check_neq(gEM.getInstallLocation(ADDONS[k].id), null);
    var addon = gEM.getItemForID(ADDONS[k].id);
    updates.push(addon);
  }

  server = new nsHttpServer();
  server.registerPathHandler("/0", requestHandler);
  server.registerPathHandler("/1", requestHandler);
  server.start(4444);
  
  gEM.update(updates, updates.length, false, updateListener);

  do_test_pending();
}
