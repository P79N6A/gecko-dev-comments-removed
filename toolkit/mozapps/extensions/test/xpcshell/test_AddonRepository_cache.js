





Components.utils.import("resource://gre/modules/addons/AddonRepository.jsm");

Components.utils.import("resource://testing-common/httpd.js");
let gServer;

const PORT      = 4444;
const BASE_URL  = "http://localhost:" + PORT;

const PREF_GETADDONS_CACHE_ENABLED = "extensions.getAddons.cache.enabled";
const PREF_GETADDONS_CACHE_TYPES   = "extensions.getAddons.cache.types";
const GETADDONS_RESULTS            = BASE_URL + "/data/test_AddonRepository_cache.xml";
const GETADDONS_EMPTY              = BASE_URL + "/data/test_AddonRepository_empty.xml";
const GETADDONS_FAILED             = BASE_URL + "/data/test_AddonRepository_failed.xml";

const FILE_DATABASE = "addons.json";
const ADDON_NAMES = ["test_AddonRepository_1",
                     "test_AddonRepository_2",
                     "test_AddonRepository_3"];
const ADDON_IDS = ADDON_NAMES.map(function(aName) aName + "@tests.mozilla.org");
const ADDON_FILES = ADDON_NAMES.map(do_get_addon);

const PREF_ADDON0_CACHE_ENABLED = "extensions." + ADDON_IDS[0] + ".getAddons.cache.enabled";
const PREF_ADDON1_CACHE_ENABLED = "extensions." + ADDON_IDS[1] + ".getAddons.cache.enabled";



const ADDON_PROPERTIES = ["id", "type", "name", "version", "creator",
                          "developers", "translators", "contributors",
                          "description", "fullDescription",
                          "developerComments", "eula", "iconURL", "icons",
                          "screenshots", "homepageURL", "supportURL",
                          "optionsURL", "aboutURL", "contributionURL",
                          "contributionAmount", "averageRating", "reviewCount",
                          "reviewURL", "totalDownloads", "weeklyDownloads",
                          "dailyUsers", "sourceURI", "repositoryStatus",
                          "compatibilityOverrides"];





const REPOSITORY_SIZE       = 9;
const REPOSITORY_UPDATEDATE = 9;



function get_subfile_uri(aId, aFilename) {
  let file = gProfD.clone();
  file.append("extensions");
  return do_get_addon_root_uri(file, aId) + aFilename;
}



const REPOSITORY_ADDONS = [{
  id:                     ADDON_IDS[0],
  type:                   "extension",
  name:                   "Repo Add-on 1",
  version:                "2.1",
  creator:                {
                            name: "Repo Add-on 1 - Creator",
                            url:  BASE_URL + "/repo/1/creator.html"
                          },
  developers:             [{
                            name: "Repo Add-on 1 - First Developer",
                            url:  BASE_URL + "/repo/1/firstDeveloper.html"
                          }, {
                            name: "Repo Add-on 1 - Second Developer",
                            url:  BASE_URL + "/repo/1/secondDeveloper.html"
                          }],
  description:            "Repo Add-on 1 - Description\nSecond line",
  fullDescription:        "Repo Add-on 1 - Full Description & some extra",
  developerComments:      "Repo Add-on 1\nDeveloper Comments",
  eula:                   "Repo Add-on 1 - EULA",
  iconURL:                BASE_URL + "/repo/1/icon.png",
  icons:                  { "32": BASE_URL + "/repo/1/icon.png" },
  homepageURL:            BASE_URL + "/repo/1/homepage.html",
  supportURL:             BASE_URL + "/repo/1/support.html",
  contributionURL:        BASE_URL + "/repo/1/meetDevelopers.html",
  contributionAmount:     "$11.11",
  averageRating:          1,
  reviewCount:            1111,
  reviewURL:              BASE_URL + "/repo/1/review.html",
  totalDownloads:         2221,
  weeklyDownloads:        3331,
  dailyUsers:             4441,
  sourceURI:              BASE_URL + "/repo/1/install.xpi",
  repositoryStatus:       4,
  compatibilityOverrides: [{
                            type: "incompatible",
                            minVersion: 0.1,
                            maxVersion: 0.2,
                            appID: "xpcshell@tests.mozilla.org",
                            appMinVersion: 3.0,
                            appMaxVersion: 4.0
                          }, {
                            type: "incompatible",
                            minVersion: 0.2,
                            maxVersion: 0.3,
                            appID: "xpcshell@tests.mozilla.org",
                            appMinVersion: 5.0,
                            appMaxVersion: 6.0
                          }]
}, {
  id:                     ADDON_IDS[1],
  type:                   "theme",
  name:                   "Repo Add-on 2",
  version:                "2.2",
  creator:                {
                            name: "Repo Add-on 2 - Creator",
                            url:  BASE_URL + "/repo/2/creator.html"
                          },
  developers:             [{
                            name: "Repo Add-on 2 - First Developer",
                            url:  BASE_URL + "/repo/2/firstDeveloper.html"
                          }, {
                            name: "Repo Add-on 2 - Second Developer",
                            url:  BASE_URL + "/repo/2/secondDeveloper.html"
                          }],
  description:            "Repo Add-on 2 - Description",
  fullDescription:        "Repo Add-on 2 - Full Description",
  developerComments:      "Repo Add-on 2 - Developer Comments",
  eula:                   "Repo Add-on 2 - EULA",
  iconURL:                BASE_URL + "/repo/2/icon.png",
  icons:                  { "32": BASE_URL + "/repo/2/icon.png" },
  screenshots:            [{
                            url:          BASE_URL + "/repo/2/firstFull.png",
                            thumbnailURL: BASE_URL + "/repo/2/firstThumbnail.png",
                            caption:      "Repo Add-on 2 - First Caption"
                          } , {
                            url:          BASE_URL + "/repo/2/secondFull.png",
                            thumbnailURL: BASE_URL + "/repo/2/secondThumbnail.png",
                            caption:      "Repo Add-on 2 - Second Caption"
                          }],
  homepageURL:            BASE_URL + "/repo/2/homepage.html",
  supportURL:             BASE_URL + "/repo/2/support.html",
  contributionURL:        BASE_URL + "/repo/2/meetDevelopers.html",
  contributionAmount:     null,
  averageRating:          2,
  reviewCount:            1112,
  reviewURL:              BASE_URL + "/repo/2/review.html",
  totalDownloads:         2222,
  weeklyDownloads:        3332,
  dailyUsers:             4442,
  sourceURI:              BASE_URL + "/repo/2/install.xpi",
  repositoryStatus:       9
}, {
  id:                     ADDON_IDS[2],
  name:                   "Repo Add-on 3",
  version:                "2.3",
  iconURL:                BASE_URL + "/repo/3/icon.png",
  icons:                  { "32": BASE_URL + "/repo/3/icon.png" },
  screenshots:            [{
                            url:          BASE_URL + "/repo/3/firstFull.png",
                            thumbnailURL: BASE_URL + "/repo/3/firstThumbnail.png",
                            caption:      "Repo Add-on 3 - First Caption"
                          } , {
                            url:          BASE_URL + "/repo/3/secondFull.png",
                            thumbnailURL: BASE_URL + "/repo/3/secondThumbnail.png",
                            caption:      "Repo Add-on 3 - Second Caption"
                          }]
}];



const WITHOUT_CACHE = [{
  id:                     ADDON_IDS[0],
  type:                   "extension",
  name:                   "XPI Add-on 1",
  version:                "1.1",
  creator:                { name: "XPI Add-on 1 - Creator" },
  developers:             [{ name: "XPI Add-on 1 - First Developer" },
                           { name: "XPI Add-on 1 - Second Developer" }],
  translators:            [{ name: "XPI Add-on 1 - First Translator" },
                           { name: "XPI Add-on 1 - Second Translator" }],
  contributors:           [{ name: "XPI Add-on 1 - First Contributor" },
                           { name: "XPI Add-on 1 - Second Contributor" }],
  description:            "XPI Add-on 1 - Description",
  iconURL:                BASE_URL + "/xpi/1/icon.png",
  icons:                  { "32": BASE_URL + "/xpi/1/icon.png" },
  homepageURL:            BASE_URL + "/xpi/1/homepage.html",
  optionsURL:             BASE_URL + "/xpi/1/options.html",
  aboutURL:               BASE_URL + "/xpi/1/about.html",
  sourceURI:              NetUtil.newURI(ADDON_FILES[0]).spec
}, {
  id:                     ADDON_IDS[1],
  type:                   "theme",
  name:                   "XPI Add-on 2",
  version:                "1.2",
  sourceURI:              NetUtil.newURI(ADDON_FILES[1]).spec,
  icons:                  {}
}, {
  id:                     ADDON_IDS[2],
  type:                   "theme",
  name:                   "XPI Add-on 3",
  version:                "1.3",
  get iconURL () {
    return get_subfile_uri(ADDON_IDS[2], "icon.png");
  },
  get icons () {
    return { "32": get_subfile_uri(ADDON_IDS[2], "icon.png") };
  },
  screenshots:            [{ get url () { return get_subfile_uri(ADDON_IDS[2], "preview.png"); } }],
  sourceURI:              NetUtil.newURI(ADDON_FILES[2]).spec
}];



const WITH_CACHE = [{
  id:                     ADDON_IDS[0],
  type:                   "extension",
  name:                   "XPI Add-on 1",
  version:                "1.1",
  creator:                {
                            name: "Repo Add-on 1 - Creator",
                            url:  BASE_URL + "/repo/1/creator.html"
                          },
  developers:             [{ name: "XPI Add-on 1 - First Developer" },
                           { name: "XPI Add-on 1 - Second Developer" }],
  translators:            [{ name: "XPI Add-on 1 - First Translator" },
                           { name: "XPI Add-on 1 - Second Translator" }],
  contributors:           [{ name: "XPI Add-on 1 - First Contributor" },
                           { name: "XPI Add-on 1 - Second Contributor" }],
  description:            "XPI Add-on 1 - Description",
  fullDescription:        "Repo Add-on 1 - Full Description & some extra",
  developerComments:      "Repo Add-on 1\nDeveloper Comments",
  eula:                   "Repo Add-on 1 - EULA",
  iconURL:                BASE_URL + "/xpi/1/icon.png",
  icons:                  { "32": BASE_URL + "/xpi/1/icon.png" },
  homepageURL:            BASE_URL + "/xpi/1/homepage.html",
  supportURL:             BASE_URL + "/repo/1/support.html",
  optionsURL:             BASE_URL + "/xpi/1/options.html",
  aboutURL:               BASE_URL + "/xpi/1/about.html",
  contributionURL:        BASE_URL + "/repo/1/meetDevelopers.html",
  contributionAmount:     "$11.11",
  averageRating:          1,
  reviewCount:            1111,
  reviewURL:              BASE_URL + "/repo/1/review.html",
  totalDownloads:         2221,
  weeklyDownloads:        3331,
  dailyUsers:             4441,
  sourceURI:              NetUtil.newURI(ADDON_FILES[0]).spec,
  repositoryStatus:       4,
  compatibilityOverrides: [{
                            type: "incompatible",
                            minVersion: 0.1,
                            maxVersion: 0.2,
                            appID: "xpcshell@tests.mozilla.org",
                            appMinVersion: 3.0,
                            appMaxVersion: 4.0
                          }, {
                            type: "incompatible",
                            minVersion: 0.2,
                            maxVersion: 0.3,
                            appID: "xpcshell@tests.mozilla.org",
                            appMinVersion: 5.0,
                            appMaxVersion: 6.0
                          }]
}, {
  id:                     ADDON_IDS[1],
  type:                   "theme",
  name:                   "XPI Add-on 2",
  version:                "1.2",
  creator:                {
                            name: "Repo Add-on 2 - Creator",
                            url:  BASE_URL + "/repo/2/creator.html"
                          },
  developers:             [{
                            name: "Repo Add-on 2 - First Developer",
                            url:  BASE_URL + "/repo/2/firstDeveloper.html"
                          }, {
                            name: "Repo Add-on 2 - Second Developer",
                            url:  BASE_URL + "/repo/2/secondDeveloper.html"
                          }],
  description:            "Repo Add-on 2 - Description",
  fullDescription:        "Repo Add-on 2 - Full Description",
  developerComments:      "Repo Add-on 2 - Developer Comments",
  eula:                   "Repo Add-on 2 - EULA",
  iconURL:                BASE_URL + "/repo/2/icon.png",
  icons:                  { "32": BASE_URL + "/repo/2/icon.png" },
  screenshots:            [{
                            url:          BASE_URL + "/repo/2/firstFull.png",
                            thumbnailURL: BASE_URL + "/repo/2/firstThumbnail.png",
                            caption:      "Repo Add-on 2 - First Caption"
                          } , {
                            url:          BASE_URL + "/repo/2/secondFull.png",
                            thumbnailURL: BASE_URL + "/repo/2/secondThumbnail.png",
                            caption:      "Repo Add-on 2 - Second Caption"
                          }],
  homepageURL:            BASE_URL + "/repo/2/homepage.html",
  supportURL:             BASE_URL + "/repo/2/support.html",
  contributionURL:        BASE_URL + "/repo/2/meetDevelopers.html",
  contributionAmount:     null,
  averageRating:          2,
  reviewCount:            1112,
  reviewURL:              BASE_URL + "/repo/2/review.html",
  totalDownloads:         2222,
  weeklyDownloads:        3332,
  dailyUsers:             4442,
  sourceURI:              NetUtil.newURI(ADDON_FILES[1]).spec,
  repositoryStatus:       9
}, {
  id:                     ADDON_IDS[2],
  type:                   "theme",
  name:                   "XPI Add-on 3",
  version:                "1.3",
  get iconURL () {
    return get_subfile_uri(ADDON_IDS[2], "icon.png");
  },
  get icons () {
    return { "32": get_subfile_uri(ADDON_IDS[2], "icon.png") };
  },
  screenshots:            [{
                            url:          BASE_URL + "/repo/3/firstFull.png",
                            thumbnailURL: BASE_URL + "/repo/3/firstThumbnail.png",
                            caption:      "Repo Add-on 3 - First Caption"
                          } , {
                            url:          BASE_URL + "/repo/3/secondFull.png",
                            thumbnailURL: BASE_URL + "/repo/3/secondThumbnail.png",
                            caption:      "Repo Add-on 3 - Second Caption"
                          }],
  sourceURI:              NetUtil.newURI(ADDON_FILES[2]).spec
}];


const WITH_EXTENSION_CACHE = [{
  id:                     ADDON_IDS[0],
  type:                   "extension",
  name:                   "XPI Add-on 1",
  version:                "1.1",
  creator:                {
                            name: "Repo Add-on 1 - Creator",
                            url:  BASE_URL + "/repo/1/creator.html"
                          },
  developers:             [{ name: "XPI Add-on 1 - First Developer" },
                           { name: "XPI Add-on 1 - Second Developer" }],
  translators:            [{ name: "XPI Add-on 1 - First Translator" },
                           { name: "XPI Add-on 1 - Second Translator" }],
  contributors:           [{ name: "XPI Add-on 1 - First Contributor" },
                           { name: "XPI Add-on 1 - Second Contributor" }],
  description:            "XPI Add-on 1 - Description",
  fullDescription:        "Repo Add-on 1 - Full Description & some extra",
  developerComments:      "Repo Add-on 1\nDeveloper Comments",
  eula:                   "Repo Add-on 1 - EULA",
  iconURL:                BASE_URL + "/xpi/1/icon.png",
  icons:                  { "32": BASE_URL + "/xpi/1/icon.png" },
  homepageURL:            BASE_URL + "/xpi/1/homepage.html",
  supportURL:             BASE_URL + "/repo/1/support.html",
  optionsURL:             BASE_URL + "/xpi/1/options.html",
  aboutURL:               BASE_URL + "/xpi/1/about.html",
  contributionURL:        BASE_URL + "/repo/1/meetDevelopers.html",
  contributionAmount:     "$11.11",
  averageRating:          1,
  reviewCount:            1111,
  reviewURL:              BASE_URL + "/repo/1/review.html",
  totalDownloads:         2221,
  weeklyDownloads:        3331,
  dailyUsers:             4441,
  sourceURI:              NetUtil.newURI(ADDON_FILES[0]).spec,
  repositoryStatus:       4,
  compatibilityOverrides: [{
                            type: "incompatible",
                            minVersion: 0.1,
                            maxVersion: 0.2,
                            appID: "xpcshell@tests.mozilla.org",
                            appMinVersion: 3.0,
                            appMaxVersion: 4.0
                          }, {
                            type: "incompatible",
                            minVersion: 0.2,
                            maxVersion: 0.3,
                            appID: "xpcshell@tests.mozilla.org",
                            appMinVersion: 5.0,
                            appMaxVersion: 6.0
                          }]
}, {
  id:                     ADDON_IDS[1],
  type:                   "theme",
  name:                   "XPI Add-on 2",
  version:                "1.2",
  sourceURI:              NetUtil.newURI(ADDON_FILES[1]).spec,
  icons:                  {}
}, {
  id:                     ADDON_IDS[2],
  type:                   "theme",
  name:                   "XPI Add-on 3",
  version:                "1.3",
  get iconURL () {
    return get_subfile_uri(ADDON_IDS[2], "icon.png");
  },
  get icons () {
    return { "32": get_subfile_uri(ADDON_IDS[2], "icon.png") };
  },
  screenshots:            [{ get url () { return get_subfile_uri(ADDON_IDS[2], "preview.png"); } }],
  sourceURI:              NetUtil.newURI(ADDON_FILES[2]).spec
}];

let gDBFile = gProfD.clone();
gDBFile.append(FILE_DATABASE);












function check_results(aActualAddons, aExpectedAddons, aFromRepository) {
  aFromRepository = !!aFromRepository;

  do_check_addons(aActualAddons, aExpectedAddons, ADDON_PROPERTIES);

  
  
  aActualAddons.forEach(function(aActualAddon) {
    if (aActualAddon.size)
      do_check_eq(aActualAddon.size === REPOSITORY_SIZE, aFromRepository);

    if (aActualAddon.updateDate) {
      let time = aActualAddon.updateDate.getTime();
      do_check_eq(time === 1000 * REPOSITORY_UPDATEDATE, aFromRepository);
    }
  });
}















function check_cache(aExpectedToFind, aExpectedImmediately) {
  do_check_eq(aExpectedToFind.length, REPOSITORY_ADDONS.length);

  let lookups = [];

  for (let i = 0 ; i < REPOSITORY_ADDONS.length ; i++) {
    lookups.push(new Promise((resolve, reject) => {
      let immediatelyFound = true;
      let expected = aExpectedToFind[i] ? REPOSITORY_ADDONS[i] : null;
      
      
      AddonRepository.getCachedAddonByID(REPOSITORY_ADDONS[i].id, function(aAddon) {
        do_check_eq(immediatelyFound, aExpectedImmediately);
        if (expected == null)
          do_check_eq(aAddon, null);
        else
          check_results([aAddon], [expected], true);
        resolve();
      });
      immediatelyFound = false;
    }));
  }
  return Promise.all(lookups);
}










function* check_initialized_cache(aExpectedToFind) {
  yield check_cache(aExpectedToFind, true);
  yield promiseRestartManager();

  
  let cacheEnabled = Services.prefs.getBoolPref(PREF_GETADDONS_CACHE_ENABLED);
  yield check_cache(aExpectedToFind, !cacheEnabled);
}

function run_test() {
  run_next_test();
}

add_task(function* setup() {
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  startupManager();

  
  yield promiseInstallAllFiles(ADDON_FILES);
  yield promiseRestartManager();

  gServer = new HttpServer();
  gServer.registerDirectory("/data/", do_get_file("data"));
  gServer.start(PORT);
});


add_task(function* run_test_1() {
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, false);
  do_check_false(AddonRepository.cacheEnabled);
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);
  do_check_true(AddonRepository.cacheEnabled);
});


add_task(function* run_test_2() {
  do_check_false(gDBFile.exists());
  yield check_cache([false, false, false], false);
  yield AddonRepository.flush();
});


add_task(function* run_test_3() {
  do_check_true(gDBFile.exists());
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS, GETADDONS_FAILED);

  yield AddonRepository.repopulateCache();
  yield check_initialized_cache([false, false, false]);
});


add_task(function* run_test_4() {
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS, GETADDONS_EMPTY);

  yield AddonRepository.repopulateCache();
  yield check_initialized_cache([false, false, false]);
});


add_task(function* run_test_5() {
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS, GETADDONS_RESULTS);

  yield AddonRepository.repopulateCache();
  yield check_initialized_cache([true, true, true]);
});


add_task(function* run_test_5_1() {
  Services.prefs.setBoolPref(PREF_ADDON0_CACHE_ENABLED, false);

  yield AddonRepository.repopulateCache();

  
  Services.prefs.setBoolPref(PREF_ADDON0_CACHE_ENABLED, true);

  yield check_initialized_cache([false, true, true]);
});


add_task(function* run_test_6() {
  do_check_true(gDBFile.exists());
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, false);

  yield AddonRepository.repopulateCache();
  
  do_check_false(gDBFile.exists());

  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);
  yield check_cache([false, false, false], false);
  yield AddonRepository.flush();
});


add_task(function* run_test_7() {
  do_check_true(gDBFile.exists());
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS, GETADDONS_FAILED);

  yield new Promise((resolve, reject) =>
    AddonRepository.cacheAddons(ADDON_IDS, resolve));
  yield check_initialized_cache([false, false, false]);
});


add_task(function* run_test_8() {
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS, GETADDONS_EMPTY);

  yield new Promise((resolve, reject) =>
    AddonRepository.cacheAddons(ADDON_IDS, resolve));
  yield check_initialized_cache([false, false, false]);
});


add_task(function* run_test_9() {
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS, GETADDONS_RESULTS);

  yield new Promise((resolve, reject) =>
    AddonRepository.cacheAddons([ADDON_IDS[0]], resolve));
  yield check_initialized_cache([true, false, false]);
});


add_task(function* run_test_9_1() {
  Services.prefs.setBoolPref(PREF_ADDON1_CACHE_ENABLED, false);

  yield new Promise((resolve, reject) =>
    AddonRepository.cacheAddons(ADDON_IDS, resolve));

  
  Services.prefs.setBoolPref(PREF_ADDON1_CACHE_ENABLED, true);

  yield check_initialized_cache([true, false, true]);
});


add_task(function* run_test_10() {
  yield new Promise((resolve, reject) =>
    AddonRepository.cacheAddons(ADDON_IDS, resolve));
  yield check_initialized_cache([true, true, true]);
});


add_task(function* run_test_11() {
  do_check_true(gDBFile.exists());
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, false);

  yield new Promise((resolve, reject) =>
    AddonRepository.cacheAddons(ADDON_IDS, resolve));
  do_check_true(gDBFile.exists());

  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);
  yield check_initialized_cache([true, true, true]);
});



add_task(function* run_test_12() {
  do_check_true(gDBFile.exists());
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, false);
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS, GETADDONS_RESULTS);

  let aAddons = yield promiseAddonsByIDs(ADDON_IDS);
  check_results(aAddons, WITHOUT_CACHE);
});



add_task(function* run_test_13() {
  do_check_true(gDBFile.exists());
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS_PERFORMANCE, GETADDONS_EMPTY);

  yield AddonManagerInternal.backgroundUpdateCheck();
  
  do_check_false(gDBFile.exists());

  let aAddons = yield promiseAddonsByIDs(ADDON_IDS);
  check_results(aAddons, WITHOUT_CACHE);
});



add_task(function* run_test_14() {
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);

  yield AddonManagerInternal.backgroundUpdateCheck();
  yield AddonRepository.flush();
  do_check_true(gDBFile.exists());

  let aAddons = yield promiseAddonsByIDs(ADDON_IDS);
  check_results(aAddons, WITHOUT_CACHE);
});



add_task(function* run_test_15() {
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS_PERFORMANCE, GETADDONS_RESULTS);

  yield AddonManagerInternal.backgroundUpdateCheck();
  let aAddons = yield promiseAddonsByIDs(ADDON_IDS);
  check_results(aAddons, WITH_CACHE);
});




add_task(function* run_test_16() {
  yield promiseRestartManager();

  let aAddons = yield promiseAddonsByIDs(ADDON_IDS);
  check_results(aAddons, WITH_CACHE);
});


add_task(function* run_test_17() {
  Services.prefs.setCharPref(PREF_GETADDONS_CACHE_TYPES, "foo,bar,extension,baz");

  yield AddonManagerInternal.backgroundUpdateCheck();
  let aAddons = yield promiseAddonsByIDs(ADDON_IDS);
  check_results(aAddons, WITH_EXTENSION_CACHE);
});

add_task(function* end_test() {
  yield new Promise((resolve, reject) => gServer.stop(resolve));
});
