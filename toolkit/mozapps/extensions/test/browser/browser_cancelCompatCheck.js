







const URI_EXTENSION_UPDATE_DIALOG = "chrome://mozapps/content/extensions/update.xul";

const PREF_GETADDONS_BYIDS            = "extensions.getAddons.get.url";
const PREF_MIN_PLATFORM_COMPAT        = "extensions.minCompatiblePlatformVersion";
const PREF_METADATA_LASTUPDATE        = "extensions.getAddons.cache.lastUpdate";

let repo = {};
Components.utils.import("resource://gre/modules/addons/AddonRepository.jsm", repo);


















let ao1 = { file: "browser_bug557956_1", id: "addon1@tests.mozilla.org"};
let ao2 = { file: "browser_bug557956_2", id: "addon2@tests.mozilla.org"};
let ao3 = { file: "browser_bug557956_3", id: "addon3@tests.mozilla.org"};
let ao4 = { file: "browser_bug557956_4", id: "addon4@tests.mozilla.org"};
let ao5 = { file: "browser_bug557956_5", id: "addon5@tests.mozilla.org"};
let ao6 = { file: "browser_bug557956_6", id: "addon6@tests.mozilla.org"};
let ao7 = { file: "browser_bug557956_7", id: "addon7@tests.mozilla.org"};
let ao8 = { file: "browser_bug557956_8_1", id: "addon8@tests.mozilla.org"};
let ao9 = { file: "browser_bug557956_9_1", id: "addon9@tests.mozilla.org"};
let ao10 = { file: "browser_bug557956_10", id: "addon10@tests.mozilla.org"};


function delayMS(aDelay) {
  let deferred = Promise.defer();
  setTimeout(deferred.resolve, aDelay);
  return deferred.promise;
}


function promise_observer(aTopic) {
  let deferred = Promise.defer();
  Services.obs.addObserver(function observe(aSubject, aObsTopic, aData) {
    Services.obs.removeObserver(arguments.callee, aObsTopic);
    deferred.resolve([aSubject, aData]);
  }, aTopic, false);
  return deferred.promise;
}




function promise_install_test_addons(aAddonList, aUpdateURL) {
  info("Starting add-on installs");
  var installs = [];
  let deferred = Promise.defer();

  
  Services.prefs.setCharPref(PREF_UPDATEURL, TESTROOT + "missing.rdf");

  for (let addon of aAddonList) {
    AddonManager.getInstallForURL(TESTROOT + "addons/" + addon.file + ".xpi", function(aInstall) {
      installs.push(aInstall);
    }, "application/x-xpinstall");
  }

  var listener = {
    installCount: 0,

    onInstallEnded: function() {
      this.installCount++;
      if (this.installCount == installs.length) {
        info("Done add-on installs");
        
        Services.prefs.setCharPref(PREF_UPDATEURL, aUpdateURL);
        deferred.resolve();
      }
    }
  };

  for (let install of installs) {
    install.addListener(listener);
    install.install();
  }

  return deferred.promise;
}

function promise_addons_by_ids(aAddonIDs) {
  info("promise_addons_by_ids " + aAddonIDs.toSource());
  let deferred = Promise.defer();
  AddonManager.getAddonsByIDs(aAddonIDs, deferred.resolve);
  return deferred.promise;
}

function* promise_uninstall_test_addons() {
  info("Starting add-on uninstalls");
  let addons = yield promise_addons_by_ids([ao1.id, ao2.id, ao3.id, ao4.id, ao5.id,
                                            ao6.id, ao7.id, ao8.id, ao9.id, ao10.id]);
  let deferred = Promise.defer();
  let uninstallCount = addons.length;
  let listener = {
    onUninstalled: function(aAddon) {
      if (aAddon) {
        info("Finished uninstalling " + aAddon.id);
      }
      if (--uninstallCount == 0) {
        info("Done add-on uninstalls");
        AddonManager.removeAddonListener(listener);
        deferred.resolve();
      }
    }};
  AddonManager.addAddonListener(listener);
  for (let addon of addons) {
    if (addon)
      addon.uninstall();
    else
      listener.onUninstalled(null);
  }
  yield deferred.promise;
}



function promise_open_compatibility_window(aInactiveAddonIds) {
  let deferred = Promise.defer();
  
  
  
  requestLongerTimeout(2);

  var variant = Cc["@mozilla.org/variant;1"].
                createInstance(Ci.nsIWritableVariant);
  variant.setFromVariant(aInactiveAddonIds);

  
  
  var features = "chrome,centerscreen,dialog,titlebar";
  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  var win = ww.openWindow(null, URI_EXTENSION_UPDATE_DIALOG, "", features, variant);

  win.addEventListener("load", function() {
    function page_shown(aEvent) {
      if (aEvent.target.pageid)
        info("Page " + aEvent.target.pageid + " shown");
    }

    win.removeEventListener("load", arguments.callee, false);

    info("Compatibility dialog opened");

    win.addEventListener("pageshow", page_shown, false);
    win.addEventListener("unload", function() {
      win.removeEventListener("unload", arguments.callee, false);
      win.removeEventListener("pageshow", page_shown, false);
      dump("Compatibility dialog closed\n");
    }, false);

    deferred.resolve(win);
  }, false);
  return deferred.promise;
}

function promise_window_close(aWindow) {
  let deferred = Promise.defer();
  aWindow.addEventListener("unload", function() {
    aWindow.removeEventListener("unload", arguments.callee, false);
    deferred.resolve(aWindow);
  }, false);
  return deferred.promise;
}

function promise_page(aWindow, aPageId) {
  let deferred = Promise.defer();
  var page = aWindow.document.getElementById(aPageId);
  if (aWindow.document.getElementById("updateWizard").currentPage === page) {
    deferred.resolve(aWindow);
  } else {
    page.addEventListener("pageshow", function() {
      page.removeEventListener("pageshow", arguments.callee, false);
      executeSoon(function() {
        deferred.resolve(aWindow);
      });
    }, false);
  }
  return deferred.promise;
}

function get_list_names(aList) {
  var items = [];
  for (let listItem of aList.childNodes)
    items.push(listItem.label);
  items.sort();
  return items;
}


let inactiveAddonIds = [
  ao5.id,
  ao6.id,
  ao7.id,
  ao8.id,
  ao9.id
];


function* check_addons_uninstalled(aAddonList) {
  let foundList = yield promise_addons_by_ids([addon.id for (addon of aAddonList)]);
  for (let i = 0; i < aAddonList.length; i++) {
    ok(!foundList[i], "Addon " + aAddonList[i].id + " is not installed");
  }
  info("Add-on uninstall check complete");
  yield true;
}





add_task(function cancel_during_repopulate() {
  let a5, a8, a9, a10;

  Services.prefs.setBoolPref(PREF_STRICT_COMPAT, true);
  Services.prefs.setCharPref(PREF_MIN_PLATFORM_COMPAT, "0");
  Services.prefs.setCharPref(PREF_UPDATEURL, TESTROOT + "missing.rdf");

  let installsDone = promise_observer("TEST:all-updates-done");

  
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, false);
  
  
  
  let addonList = [ao5, ao8, ao9, ao10];
  yield promise_install_test_addons(addonList,
                                    TESTROOT + "cancelCompatCheck.sjs?500");

  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);
  Services.prefs.setCharPref(PREF_GETADDONS_BYIDS, TESTROOT + "browser_bug557956.xml");

  [a5, a8, a9] = yield promise_addons_by_ids([ao5.id, ao8.id, ao9.id]);
  ok(!a5.isCompatible, "addon5 should not be compatible");
  ok(!a8.isCompatible, "addon8 should not be compatible");
  ok(!a9.isCompatible, "addon9 should not be compatible");

  let compatWindow = yield promise_open_compatibility_window([ao5.id, ao8.id]);
  var doc = compatWindow.document;
  yield promise_page(compatWindow, "versioninfo");

  
  
  yield delayMS(50);

  info("Cancel the compatibility check dialog");
  var button = doc.documentElement.getButton("cancel");
  EventUtils.synthesizeMouse(button, 2, 2, { }, compatWindow);

  info("Waiting for installs to complete");
  yield installsDone;
  ok(!repo.AddonRepository.isSearching, "Background installs are done");

  
  let getInstalls = Promise.defer();
  AddonManager.getAllInstalls(getInstalls.resolve);
  let installs = yield getInstalls.promise;
  is (installs.length, 0, "There should be no active installs after background installs are done");

  
  
  [a5, a8, a9, a10] = yield promise_addons_by_ids([ao5.id, ao8.id, ao9.id, ao10.id]);
  ok(a5.isCompatible, "addon5 should be compatible");
  ok(a8.isCompatible, "addon8 should have been upgraded");
  ok(!a9.isCompatible, "addon9 should not have been upgraded");
  ok(!a10.isCompatible, "addon10 should not be compatible");

  info("Updates done");
  yield promise_uninstall_test_addons();
  info("done uninstalling add-ons");
});





add_task(function cancel_during_findUpdates() {
  let a5, a8, a9;

  Services.prefs.setBoolPref(PREF_STRICT_COMPAT, true);
  Services.prefs.setCharPref(PREF_MIN_PLATFORM_COMPAT, "0");

  
  Services.prefs.clearUserPref(PREF_METADATA_LASTUPDATE);
  let observeUpdateDone = promise_observer("TEST:addon-repository-data-updated");
  let installsDone = promise_observer("TEST:all-updates-done");

  
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, false);
  
  let addonList = [ao3, ao5, ao6, ao7, ao8, ao9];
  yield promise_install_test_addons(addonList,
                                    TESTROOT + "cancelCompatCheck.sjs");

  [a8] = yield promise_addons_by_ids([ao8.id]);
  a8.applyBackgroundUpdates = AddonManager.AUTOUPDATE_DISABLE;

  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);
  let compatWindow = yield promise_open_compatibility_window(inactiveAddonIds);
  var doc = compatWindow.document;
  yield promise_page(compatWindow, "versioninfo");

  info("Waiting for repository-data-updated");
  yield observeUpdateDone;

  
  yield delayMS(5);

  info("Cancel the compatibility check dialog");
  var button = doc.documentElement.getButton("cancel");
  EventUtils.synthesizeMouse(button, 2, 2, { }, compatWindow);

  info("Waiting for installs to complete 2");
  yield installsDone;
  ok(!repo.AddonRepository.isSearching, "Background installs are done 2");

  
  
  [a5, a8, a9] = yield promise_addons_by_ids([ao5.id, ao8.id, ao9.id]);
  ok(a5.isCompatible, "addon5 should be compatible");
  ok(!a8.isCompatible, "addon8 should not have been upgraded");
  ok(a9.isCompatible, "addon9 should have been upgraded");

  let getInstalls = Promise.defer();
  AddonManager.getAllInstalls(getInstalls.resolve);
  let installs = yield getInstalls.promise;
  is (installs.length, 0, "There should be no active installs after the dialog is cancelled 2");

  info("findUpdates done");
  yield promise_uninstall_test_addons();
});





add_task(function cancel_mismatch() {
  let a3, a5, a7, a8, a9;

  Services.prefs.setBoolPref(PREF_STRICT_COMPAT, true);
  Services.prefs.setCharPref(PREF_MIN_PLATFORM_COMPAT, "0");

  
  Services.prefs.clearUserPref(PREF_METADATA_LASTUPDATE);
  let installsDone = promise_observer("TEST:all-updates-done");

  
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, false);
  
  let addonList = [ao3, ao5, ao6, ao7, ao8, ao9];
  yield promise_install_test_addons(addonList,
                                    TESTROOT + "cancelCompatCheck.sjs");

  [a8] = yield promise_addons_by_ids([ao8.id]);
  a8.applyBackgroundUpdates = AddonManager.AUTOUPDATE_DISABLE;

  
  [a3, a7, a8, a9] = yield promise_addons_by_ids([ao3.id, ao7.id, ao8.id, ao9.id]);
  ok(!a3.isCompatible, "addon3 should not be compatible");
  ok(!a7.isCompatible, "addon7 should not be compatible");
  ok(!a8.isCompatible, "addon8 should not be compatible");
  ok(!a9.isCompatible, "addon9 should not be compatible");

  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);
  let compatWindow = yield promise_open_compatibility_window(inactiveAddonIds);
  var doc = compatWindow.document;
  info("Wait for mismatch page");
  yield promise_page(compatWindow, "mismatch");
  info("Click the Don't Check button");
  var button = doc.documentElement.getButton("cancel");
  EventUtils.synthesizeMouse(button, 2, 2, { }, compatWindow);

  yield promise_window_close(compatWindow);
  info("Waiting for installs to complete in cancel_mismatch");
  yield installsDone;

  
  
  [a5, a8, a9] = yield promise_addons_by_ids([ao5.id, ao8.id, ao9.id]);
  ok(a5.isCompatible, "addon5 should be compatible");
  ok(!a8.isCompatible, "addon8 should not have been upgraded");
  ok(a9.isCompatible, "addon9 should have been upgraded");

  
  let pInstalls = Promise.defer();
  AddonManager.getAllInstalls(pInstalls.resolve);
  let installs = yield pInstalls.promise;
  ok(installs.length == 0, "No remaining add-on installs (" + installs.toSource() + ")");

  yield promise_uninstall_test_addons();
  yield check_addons_uninstalled(addonList);
});



add_task(function cancel_mismatch_no_updates() {
  let a3, a5, a6

  Services.prefs.setBoolPref(PREF_STRICT_COMPAT, true);
  Services.prefs.setCharPref(PREF_MIN_PLATFORM_COMPAT, "0");

  
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, false);
  
  let addonList = [ao3, ao5, ao6];
  yield promise_install_test_addons(addonList,
                                    TESTROOT + "cancelCompatCheck.sjs");

  
  [a3, a5, a6] = yield promise_addons_by_ids([ao3.id, ao5.id, ao6.id]);
  ok(!a3.isCompatible, "addon3 should not be compatible");
  ok(!a5.isCompatible, "addon5 should not be compatible");
  ok(!a6.isCompatible, "addon6 should not be compatible");

  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);
  let compatWindow = yield promise_open_compatibility_window([ao3.id, ao5.id, ao6.id]);
  var doc = compatWindow.document;
  info("Wait for mismatch page");
  yield promise_page(compatWindow, "mismatch");
  info("Click the Don't Check button");
  var button = doc.documentElement.getButton("cancel");
  EventUtils.synthesizeMouse(button, 2, 2, { }, compatWindow);

  yield promise_window_close(compatWindow);

  [a3, a5, a6] = yield promise_addons_by_ids([ao3.id, ao5.id, ao6.id]);
  ok(!a3.isCompatible, "addon3 should not be compatible");
  ok(a5.isCompatible, "addon5 should have become compatible");
  ok(a6.isCompatible, "addon6 should have become compatible");

  
  let pInstalls = Promise.defer();
  AddonManager.getAllInstalls(pInstalls.resolve);
  let installs = yield pInstalls.promise;
  ok(installs.length == 0, "No remaining add-on installs (" + installs.toSource() + ")");

  yield promise_uninstall_test_addons();
  yield check_addons_uninstalled(addonList);
});
