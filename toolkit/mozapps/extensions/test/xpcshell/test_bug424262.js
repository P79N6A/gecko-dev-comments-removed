




































Components.utils.import("resource://gre/modules/AddonRepository.jsm");

const PREF_GETADDONS_GETRECOMMENDED      = "extensions.getAddons.recommended.url";

do_load_httpd_js();
var server;
var RESULTS = [
  null,
  null,
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
      if (addons[i].averageRating != RESULTS[i])
        do_throw("Rating for " + addons[i].id + " was " + addons[i].averageRating + ", should have been " + RESULTS[i]);
    }
    server.stop(do_test_finished);
  },

  searchFailed: function() {
    server.stop(do_test_finished);
    do_throw("Recommended results failed");
  }
};

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");
  startupManager();

  server = new nsHttpServer();
  server.registerDirectory("/", do_get_file("data"));
  server.start(4444);

  
  Services.prefs.setCharPref(PREF_GETADDONS_GETRECOMMENDED, "http://localhost:4444/test_bug424262.xml");
  
  do_check_neq(AddonRepository, null);

  do_test_pending();
  
  AddonRepository.retrieveRecommendedAddons(RESULTS.length, RecommendedCallback);
}

