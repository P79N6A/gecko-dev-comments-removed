






































gPrefs.setBoolPref("extensions.checkUpdateSecurity", false);


gPrefs.setCharPref("extensions.update.url", "http://localhost:4444/");


do_load_httpd_js();
var testserver;

var next_state = null;
var needs_compatibility = false;
var next_test = null;


var promptService = {
  alert: function(aParent, aDialogTitle, aText) {
  },
  
  alertCheck: function(aParent, aDialogTitle, aText, aCheckMsg, aCheckState) {
  },
  
  confirm: function(aParent, aDialogTitle, aText) {
  },
  
  confirmCheck: function(aParent, aDialogTitle, aText, aCheckMsg, aCheckState) {
  },
  
  confirmEx: function(aParent, aDialogTitle, aText, aButtonFlags, aButton0Title, aButton1Title, aButton2Title, aCheckMsg, aCheckState) {
  },
  
  prompt: function(aParent, aDialogTitle, aText, aValue, aCheckMsg, aCheckState) {
  },
  
  promptUsernameAndPassword: function(aParent, aDialogTitle, aText, aUsername, aPassword, aCheckMsg, aCheckState) {
  },

  promptPassword: function(aParent, aDialogTitle, aText, aPassword, aCheckMsg, aCheckState) {
  },
  
  select: function(aParent, aDialogTitle, aText, aCount, aSelectList, aOutSelection) {
  },
  
  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIPromptService)
     || iid.equals(Components.interfaces.nsISupports))
      return this;
  
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

var PromptServiceFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return promptService.QueryInterface(iid);
  }
};
var registrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{6cc9c9fe-bc0b-432b-a410-253ef8bcc699}"), "PromptService",
                          "@mozilla.org/embedcomp/prompt-service;1", PromptServiceFactory);


var updateListener = {
  onUpdateStarted: function onUpdateStarted() {
  },

  onUpdateEnded: function onUpdateEnded() {
  },

  onAddonUpdateStarted: function onAddonUpdateStarted(aAddon) {
  },

  onAddonUpdateEnded: function onAddonUpdateEnded(aAddon, aStatus) {
    if (next_test)
      next_test(aAddon, aStatus);
  }
}


var installListener = {
  onDownloadStarted: function(aAddon) {
    do_check_eq(next_state, "onDownloadStarted");
    dump(next_state+"\n");

    next_state = "onDownloadEnded";
  },

  onDownloadEnded: function(aAddon) {
    do_check_eq(next_state, "onDownloadEnded");
    dump(next_state+"\n");

    next_state = "onInstallStarted";
  },

  onInstallStarted: function(aAddon) {
    do_check_eq(next_state, "onInstallStarted");
    dump(next_state+"\n");

    if (needs_compatibility)
      next_state = "onCompatibilityCheckStarted";
    else
      next_state = "onInstallEnded";
  },

  onCompatibilityCheckStarted: function(aAddon) {
    do_check_eq(next_state, "onCompatibilityCheckStarted");
    dump(next_state+"\n");

    next_state = "onCompatibilityCheckEnded";
  },

  onCompatibilityCheckEnded: function(aAddon, aStatus) {
    do_check_eq(next_state, "onCompatibilityCheckEnded");
    dump(next_state+"\n");

    next_state = "onInstallEnded";
  },

  onInstallEnded: function(aAddon, aStatus) {
    do_check_eq(next_state, "onInstallEnded");
    dump(next_state+"\n");

    next_state = "onInstallsCompleted";
  },

  onInstallsCompleted: function() {
    do_check_eq(next_state, "onInstallsCompleted");
    dump(next_state+"\n");

    next_state = null;
    if (next_test)
      next_test();
  },

  onDownloadProgress: function onProgress(aAddon, aValue, aMaxValue) {
  }
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "1.9");

  const dataDir = do_get_file("data");
  const addonsDir = do_get_addon("test_bug396129_a_1").parent;

  
  testserver = new nsHttpServer();
  testserver.registerDirectory("/addons/", addonsDir);
  testserver.registerDirectory("/data/", dataDir);
  testserver.start(4444);

  startupEM();

  gEM.addInstallListener(installListener);

  run_install_test();
}

function run_install_test() {
  
  needs_compatibility = false;

  var addons = ["test_bug396129_a_1", "test_bug396129_b_1", "test_bug396129_c_1", "test_bug396129_d_1"];
  for (var i = 0; i < addons.length; i++) {
    next_state = "onInstallStarted";
    gEM.installItemFromFile(do_get_addon(addons[i]), NS_INSTALL_LOCATION_APPPROFILE);
    do_check_eq(next_state, null);
  }

  restartEM();

  var item = gEM.getItemForID("bug396129_a@tests.mozilla.org");
  do_check_neq(item, null);

  next_test = updatecheck_a;
  gEM.update([item], 1, Components.interfaces.nsIExtensionManager.UPDATE_CHECK_NEWVERSION, updateListener);

  do_test_pending();
}

function updatecheck_a(addon, status) {
  do_check_eq(status, Components.interfaces.nsIAddonUpdateCheckListener.STATUS_UPDATE);
  do_check_eq(addon.version, 2);

  next_test = updated_a;
  needs_compatibility = true;
  next_state = "onDownloadStarted";
  gEM.addDownloads([addon], 1, null);
}

function updated_a() {
  do_check_eq(next_state, null);

  var item = gEM.getItemForID("bug396129_d@tests.mozilla.org");
  do_check_neq(item, null);

  next_test = updatecheck_d;
  gEM.update([item], 1, Components.interfaces.nsIExtensionManager.UPDATE_CHECK_NEWVERSION, updateListener);
}

function updatecheck_d(addon, status) {
  do_check_eq(status, Components.interfaces.nsIAddonUpdateCheckListener.STATUS_UPDATE);
  do_check_eq(addon.version, 2);

  next_test = updated_d;
  needs_compatibility = false;
  next_state = "onDownloadStarted";
  gEM.addDownloads([addon], 1, null);
}

function updated_d() {
  do_check_eq(next_state, null);
  restartEM();
  gEM.uninstallItem("bug396129_a@tests.mozilla.org");
  gEM.uninstallItem("bug396129_b@tests.mozilla.org");
  gEM.uninstallItem("bug396129_c@tests.mozilla.org");
  gEM.uninstallItem("bug396129_d@tests.mozilla.org");
  restartEM();
  do_check_eq(gEM.getItemForID("bug396129_a@tests.mozilla.org"), null);
  do_check_eq(gEM.getItemForID("bug396129_b@tests.mozilla.org"), null);
  do_check_eq(gEM.getItemForID("bug396129_c@tests.mozilla.org"), null);
  do_check_eq(gEM.getItemForID("bug396129_d@tests.mozilla.org"), null);

  next_state = "onInstallStarted";
  needs_compatibility = true;
  next_test = installed_a;
  gEM.installItemFromFile(do_get_addon("test_bug396129_a_2"), NS_INSTALL_LOCATION_APPPROFILE);
}

function installed_a() {
  do_check_eq(next_state, null);
  restartEM();
  
  do_check_neq(gEM.getItemForID("bug396129_a@tests.mozilla.org"), null);
  gEM.uninstallItem("bug396129_a@tests.mozilla.org");
  restartEM();
  do_check_eq(gEM.getItemForID("bug396129_a@tests.mozilla.org"), null);

  next_state = "onInstallStarted";
  needs_compatibility = true;
  next_test = installed_b;
  gEM.installItemFromFile(do_get_addon("test_bug396129_b_2"), NS_INSTALL_LOCATION_APPPROFILE);
}

function installed_b() {
  do_check_eq(next_state, null);
  restartEM();
  
  do_check_eq(gEM.getItemForID("bug396129_b@tests.mozilla.org"), null);

  next_state = "onInstallStarted";
  needs_compatibility = true;
  next_test = installed_c;
  gEM.installItemFromFile(do_get_addon("test_bug396129_c_2"), NS_INSTALL_LOCATION_APPPROFILE);
}

function installed_c() {
  do_check_eq(next_state, null);

  restartEM();
  
  do_check_eq(gEM.getItemForID("bug396129_c@tests.mozilla.org"), null);

  var item = Components.classes["@mozilla.org/updates/item;1"]
                       .createInstance(Components.interfaces.nsIUpdateItem);
  item.init("bug396129_b@tests.mozilla.org", "", NS_INSTALL_LOCATION_APPPROFILE,
            "", "", "", " http://localhost:4444/addons/test_bug396129_b_2.xpi",
            "", "", "", "", 2, "");
  next_state = "onDownloadStarted";
  needs_compatibility = true;
  next_test = downloaded_b;
  gEM.addDownloads([item], 1, null);
}

function downloaded_b() {
  do_check_eq(next_state, null);

  restartEM();
  
  do_check_eq(gEM.getItemForID("bug396129_b@tests.mozilla.org"), null);

  var item = Components.classes["@mozilla.org/updates/item;1"]
                       .createInstance(Components.interfaces.nsIUpdateItem);
  item.init("bug396129_c@tests.mozilla.org", "", NS_INSTALL_LOCATION_APPPROFILE,
            "", "", "", " http://localhost:4444/addons/test_bug396129_c_2.xpi",
            "", "", "", "", 2, "");
  next_state = "onDownloadStarted";
  needs_compatibility = true;
  next_test = downloaded_c;
  gEM.addDownloads([item], 1, null);
}

function downloaded_c() {
  do_check_eq(next_state, null);

  restartEM();
  
  do_check_eq(gEM.getItemForID("bug396129_c@tests.mozilla.org"), null);

  test_complete();
}

function test_complete() {
  testserver.stop();
  do_test_finished();
}

