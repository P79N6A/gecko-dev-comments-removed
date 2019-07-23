





































const PREF_GETADDONS_BROWSEADDONS        = "extensions.getAddons.browseAddons";
const PREF_GETADDONS_BROWSERECOMMENDED   = "extensions.getAddons.recommended.browseURL";
const PREF_GETADDONS_GETRECOMMENDED      = "extensions.getAddons.recommended.url";
const PREF_GETADDONS_BROWSESEARCHRESULTS = "extensions.getAddons.search.browseURL";
const PREF_GETADDONS_GETSEARCHRESULTS    = "extensions.getAddons.search.url";

const BROWSE      = "http://localhost:4444/";
const RECOMMENDED = "http://localhost:4444/recommended.html";
const SEARCH      = "http://localhost:4444/search.html?q=";

var BROWSE_SEARCH_URLS = [
  ["test",                              SEARCH + "test" ],
  ["SEARCH",                            SEARCH + "SEARCH" ],
  ["test search",                       SEARCH + "test%20search" ],
  ["odd=search:with&weird\"characters", SEARCH + "odd%3Dsearch%3Awith%26weird%22characters" ]
];

do_load_httpd_js();
var server;
var addonRepo;

var RESULTS = [
{
  id:           "test5@tests.mozilla.org",
  name:         "PASS",
  version:      "1.0",
  summary:      "This should work fine",
  description:  "Test description",
  rating:       -1,
  iconURL:      null,
  thumbnailURL: null,
  homepageURL:  "https://addons.mozilla.org/addon/5992",
  eula:         null,
  type:         Ci.nsIUpdateItem.TYPE_EXTENSION,
  xpiURL:       "http://localhost:4444/test.xpi",
  xpiHash:      "sha1:c26f0b0d62e5dcddcda95074d3f3fedb9bbc26e3"
},
{
  id:           "test6@tests.mozilla.org",
  name:         "PASS",
  version:      "1.0",
  summary:      "Specific OS should work fine",
  description:  null,
  rating:       4,
  iconURL:      "http://localhost:4444/test_bug404024/icon.png",
  thumbnailURL: "http://localhost:4444/test_bug404024/thumbnail.png",
  homepageURL:  null,
  eula:         "EULA should be confirmed",
  type:         Ci.nsIUpdateItem.TYPE_THEME,
  xpiURL:       "http://localhost:4444/test.xpi",
  xpiHash:      null
}
];

function checkResults(addons, length) {
  do_check_eq(addons.length, RESULTS.length);
  do_check_eq(addons.length, length);

  for (var i = 0; i < addons.length; i++) {
    if (addons[i].name == "FAIL")
      do_throw(addons[i].id + " - " + addons[i].summary);
    if (addons[i].name != "PASS")
      do_throw(addons[i].id + " - " + "invalid add-on name " + addons[i].name);
  }

  for (var i = 0; i < addons.length; i++) {
    for (var p in RESULTS[i]) {
      if (addons[i][p] != RESULTS[i][p])
        do_throw("Failed on property " + p + " on add-on " + addons[i].id +
                 addons[i][p] + " == " + RESULTS[i][p]);
    }
  }
}

var RecommendedCallback = {
  searchSucceeded: function(addons, length, total) {
    
    do_check_false(addonRepo.isSearching);
    checkResults(addons, length);

    
    addonRepo.searchAddons("bug404024", 10, SearchCallback);
    
    do_check_true(addonRepo.isSearching);
    addonRepo.searchAddons("test search", 10, FailCallback);
  },

  searchFailed: function() {
    server.stop(do_test_finished);
    do_throw("Recommended results failed");
  }
};

var SearchCallback = {
  searchSucceeded: function(addons, length, total) {
    do_check_false(addonRepo.isSearching);
    checkResults(addons, length);

    server.stop(do_test_finished);
  },

  searchFailed: function() {
    server.stop(do_test_finished);
    do_throw("Search results failed");
  }
};

var FailCallback = {
  searchSucceeded: function(addons, length, total) {
    server.stop(do_test_finished);
    do_throw("Should not be called");
  },

  searchFailed: function() {
    server.stop(do_test_finished);
    do_throw("Should not be called");
  }
};

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  
  startupEM();
  gEM.installItemFromFile(do_get_addon("test_bug397778"), NS_INSTALL_LOCATION_APPPROFILE);
  restartEM();

  server = new nsHttpServer();
  server.registerDirectory("/", do_get_file("data"));
  server.start(4444);

  
  gPrefs.setCharPref(PREF_GETADDONS_BROWSEADDONS, BROWSE);
  gPrefs.setCharPref(PREF_GETADDONS_BROWSERECOMMENDED, RECOMMENDED);
  gPrefs.setCharPref(PREF_GETADDONS_GETRECOMMENDED, "http://localhost:4444/test_bug404024.xml");
  gPrefs.setCharPref(PREF_GETADDONS_BROWSESEARCHRESULTS, SEARCH + "%TERMS%");
  gPrefs.setCharPref(PREF_GETADDONS_GETSEARCHRESULTS, "http://localhost:4444/test_%TERMS%.xml");
  
  addonRepo = Components.classes["@mozilla.org/extensions/addon-repository;1"]
                        .getService(Components.interfaces.nsIAddonRepository);

  do_check_neq(addonRepo, null);
  
  do_check_eq(addonRepo.homepageURL, BROWSE);
  do_check_eq(addonRepo.getRecommendedURL(), RECOMMENDED);

  
  for (var i = 0; i < BROWSE_SEARCH_URLS.length; i++) {
    var url = addonRepo.getSearchURL(BROWSE_SEARCH_URLS[i][0]);
    if (url != BROWSE_SEARCH_URLS[i][1])
      do_throw("BROWSE_SEARCH_URL[" + i + "] returned " + url);
  }

  do_test_pending();
  
  addonRepo.retrieveRecommendedAddons(10, FailCallback);
  addonRepo.cancelSearch();
  
  addonRepo.retrieveRecommendedAddons(10, RecommendedCallback);
  
  do_check_true(addonRepo.isSearching);
  addonRepo.retrieveRecommendedAddons(10, FailCallback);
}

