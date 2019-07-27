


Components.utils.import("resource://gre/modules/osfile.jsm");

const kDefaultenginenamePref = "browser.search.defaultenginename";
const kSelectedEnginePref = "browser.search.selectedEngine";

const kTestEngineName = "Test search engine";

function getDefaultEngineName() {
  const nsIPLS = Ci.nsIPrefLocalizedString;
  return Services.prefs.getComplexValue(kDefaultenginenamePref, nsIPLS).data;
}

function waitForNotification(aExpectedData) {
  let deferred = Promise.defer();

  const SEARCH_SERVICE_TOPIC = "browser-search-service";
  Services.obs.addObserver(function observer(aSubject, aTopic, aData) {
    if (aData != aExpectedData)
      return;

    Services.obs.removeObserver(observer, SEARCH_SERVICE_TOPIC);
    deferred.resolve();
  }, SEARCH_SERVICE_TOPIC, false);

  return deferred.promise;
}

function asyncInit() {
  let deferred = Promise.defer();

  Services.search.init(function() {
    do_check_true(Services.search.isInitialized);
    deferred.resolve();
  });

  return deferred.promise;
}

function asyncReInit() {
  let promise = waitForNotification("reinit-complete");

  Services.search.QueryInterface(Ci.nsIObserver)
          .observe(null, "nsPref:changed", "general.useragent.locale");

  return promise;
}


add_task(function* test_defaultEngine() {
  yield asyncInit();

  do_check_eq(Services.search.currentEngine.name, getDefaultEngineName());
});


add_task(function* test_selectedEngine() {
  let defaultEngineName = getDefaultEngineName();
  
  Services.prefs.setCharPref(kSelectedEnginePref, kTestEngineName);

  yield asyncReInit();
  do_check_eq(Services.search.currentEngine.name, defaultEngineName);

  Services.prefs.clearUserPref(kSelectedEnginePref);

  
  Services.prefs.setCharPref(kDefaultenginenamePref, kTestEngineName);

  yield asyncReInit();
  do_check_eq(Services.search.currentEngine.name, defaultEngineName);

  Services.prefs.clearUserPref(kDefaultenginenamePref);
});


add_task(function* test_persistAcrossRestarts() {
  
  Services.search.currentEngine = Services.search.getEngineByName(kTestEngineName);
  do_check_eq(Services.search.currentEngine.name, kTestEngineName);
  yield waitForNotification("write-metadata-to-disk-complete");

  
  let path = OS.Path.join(OS.Constants.Path.profileDir, "search-metadata.json");
  let bytes = yield OS.File.read(path);
  let json = JSON.parse(new TextDecoder().decode(bytes));
  do_check_eq(json["[global]"].hash.length, 44);

  
  yield asyncReInit();
  do_check_eq(Services.search.currentEngine.name, kTestEngineName);

  
  Services.search.currentEngine = Services.search.defaultEngine;
  do_check_eq(Services.search.currentEngine.name, getDefaultEngineName());
});


add_task(function* test_ignoreInvalidHash() {
  
  Services.search.currentEngine = Services.search.getEngineByName(kTestEngineName);
  do_check_eq(Services.search.currentEngine.name, kTestEngineName);
  yield waitForNotification("write-metadata-to-disk-complete");

  
  let path = OS.Path.join(OS.Constants.Path.profileDir, "search-metadata.json");
  let bytes = yield OS.File.read(path);
  let json = JSON.parse(new TextDecoder().decode(bytes));

  
  json["[global]"].hash = "invalid";

  let data = new TextEncoder().encode(JSON.stringify(json));
  let promise = OS.File.writeAtomic(path, data);

  
  yield asyncReInit();
  do_check_eq(Services.search.currentEngine.name, getDefaultEngineName());
});


add_task(function* test_settingToDefault() {
  
  Services.search.currentEngine = Services.search.getEngineByName(kTestEngineName);
  do_check_eq(Services.search.currentEngine.name, kTestEngineName);
  yield waitForNotification("write-metadata-to-disk-complete");

  
  let path = OS.Path.join(OS.Constants.Path.profileDir, "search-metadata.json");
  let bytes = yield OS.File.read(path);
  let json = JSON.parse(new TextDecoder().decode(bytes));
  do_check_eq(json["[global]"].current, kTestEngineName);

  
  Services.search.currentEngine =
    Services.search.getEngineByName(getDefaultEngineName());
  yield waitForNotification("write-metadata-to-disk-complete");

  
  bytes = yield OS.File.read(path);
  json = JSON.parse(new TextDecoder().decode(bytes));
  do_check_eq(json["[global]"].current, "");
});


function run_test() {
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

  run_next_test();
}
