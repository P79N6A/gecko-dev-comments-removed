





































const PREF_GETADDONS_GETRECOMMENDED      = "extensions.getAddons.recommended.url";

do_load_httpd_js();
var server;
var addonRepo;

var RESULTS = [
  -1,
  -1,
  0,
  2,
  4,
  5,
  5,
  5
];

var RecommendedCallback = {
  searchSucceeded: function(addons, length, total) {
    dump("loaded");
    
    do_check_eq(length, RESULTS.length);

    for (var i = 0; i < length; i++) {
      if (addons[i].rating != RESULTS[i])
        do_throw("Rating for " + addons[i].id + " was " + addons[i].rating + ", should have been " + RESULTS[i]);
    }
    do_test_finished();
    server.stop();
  },

  searchFailed: function() {
    do_test_finished();
    server.stop();
    do_throw("Recommended results failed");
  }
};

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");
  startupEM();

  server = new nsHttpServer();
  server.registerDirectory("/", do_get_file("data"));
  server.start(4444);

  
  gPrefs.setCharPref(PREF_GETADDONS_GETRECOMMENDED, "http://localhost:4444/test_bug424262.xml");
  
  addonRepo = Components.classes["@mozilla.org/extensions/addon-repository;1"]
                        .getService(Components.interfaces.nsIAddonRepository);

  do_check_neq(addonRepo, null);

  do_test_pending();
  
  addonRepo.retrieveRecommendedAddons(RESULTS.length, RecommendedCallback);
}

