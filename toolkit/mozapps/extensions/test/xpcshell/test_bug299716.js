






































Services.prefs.setBoolPref("extensions.checkUpdateSecurity", false);


const checkListener = {
  pendingCount: 0,

  onUpdateAvailable: function onUpdateAvailable(aAddon, aInstall) {
    for (var i = 0; i < ADDONS.length; i++) {
      if (ADDONS[i].id == aAddon.id) {
        ADDONS[i].newInstall = aInstall;
        return;
      }
    }
  },

  onUpdateFinished: function onUpdateFinished() {
    if (--this.pendingCount == 0)
      next_test();
  }
}


do_load_httpd_js();
var testserver;

var ADDONS = [
  
  {
    id: "bug299716-a@tests.mozilla.org",
    addon: "test_bug299716_a_1",
    installed: true,
    item: null,
    newInstall: null
  },

  
  {
    id: "bug299716-b@tests.mozilla.org",
    addon: "test_bug299716_b_1",
    installed: true,
    item: null,
    newInstall: null
  },

  
  {
    id: "bug299716-c@tests.mozilla.org",
    addon: "test_bug299716_c_1",
    installed: true,
    item: null,
    newInstall: null
  },

  
  {
    id: "bug299716-d@tests.mozilla.org",
    addon: "test_bug299716_d_1",
    installed: true,
    item: null,
    newInstall: null
  },

  
  {
    id: "bug299716-e@tests.mozilla.org",
    addon: "test_bug299716_e_1",
    installed: false,
    item: null,
    newInstall: null,
    failedAppName: "XPCShell"
  },

  
  {
    id: "bug299716-f@tests.mozilla.org",
    addon: "test_bug299716_f_1",
    installed: false,
    item: null,
    newInstall: null,
    failedAppName: "XPCShell"
  },

  
  {
    id: "bug299716-g@tests.mozilla.org",
    addon: "test_bug299716_g_1",
    installed: false,
    item: null,
    newInstall: null,
    failedAppName: "Toolkit"
  },
];

var next_test = function() {};

function do_check_item(aItem, aVersion, aAddonsEntry) {
  if (aAddonsEntry.installed) {
    if (aItem == null)
      do_throw("Addon " + aAddonsEntry.id + " wasn't detected");
    if (aItem.version != aVersion)
      do_throw("Addon " + aAddonsEntry.id + " was version " + aItem.version + " instead of " + aVersion);
  } else {
    if (aItem != null)
      do_throw("Addon " + aAddonsEntry.id + " was detected");
  }
}




function run_test() {
  do_test_pending();

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "5", "1.9");

  const dataDir = do_get_file("data");
  const addonsDir = do_get_addon(ADDONS[0].addon).parent;

  
  const xpiFile = addonsDir.clone();
  xpiFile.append("test_bug299716_a_2.xpi");
  do_check_true(xpiFile.exists());

  
  testserver = new nsHttpServer();
  testserver.registerDirectory("/addons/", addonsDir);
  testserver.registerDirectory("/data/", dataDir);
  testserver.start(4444);

  
  const Ci = Components.interfaces;
  const xhr = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
                        .createInstance(Ci.nsIXMLHttpRequest)
  xhr.open("GET", "http://localhost:4444/addons/test_bug299716_a_2.xpi", false);
  xhr.send(null);
  do_check_true(xhr.status == 200);

  xhr.open("GET", "http://localhost:4444/data/test_bug299716.rdf", false);
  xhr.send(null);
  do_check_true(xhr.status == 200);

  
  startupManager();
  dump("\n\n*** INSTALLING NEW ITEMS\n\n");

  installAllFiles([do_get_addon(a.addon) for each (a in ADDONS)], run_test_pt2,
                  true);
}




function run_test_pt2() {
  dump("\n\n*** DONE INSTALLING NEW ITEMS\n\n");
  dump("\n\n*** RESTARTING EXTENSION MANAGER\n\n");
  restartManager();

  AddonManager.getAddonsByIDs([a.id for each (a in ADDONS)], function(items) {
    dump("\n\n*** REQUESTING UPDATE\n\n");
    
    next_test = run_test_pt3;

    
    for (var i = 0; i < ADDONS.length; i++) {
      var item = items[i];
      do_check_item(item, "0.1", ADDONS[i]);

      if (item) {
        checkListener.pendingCount++;
        ADDONS[i].item = item;
        item.findUpdates(checkListener, AddonManager.UPDATE_WHEN_USER_REQUESTED);
      }
    }
  });
}




function run_test_pt3() {
  
  dump("\n\n*** UPDATING ITEMS\n\n");
  completeAllInstalls([a.newInstall for each(a in ADDONS) if (a.newInstall)],
                      run_test_pt4);
}




function run_test_pt4() {
  dump("\n\n*** RESTARTING EXTENSION MANAGER\n\n");
  restartManager();

  dump("\n\n*** FINAL CHECKS\n\n");
  AddonManager.getAddonsByIDs([a.id for each (a in ADDONS)], function(items) {
    for (var i = 0; i < ADDONS.length; i++) {
      var item = items[i];
      do_check_item(item, "0.2", ADDONS[i]);
    }

    testserver.stop(do_test_finished);
  });
}
