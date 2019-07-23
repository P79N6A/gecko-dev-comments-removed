






































gPrefs.setBoolPref("extensions.checkUpdateSecurity", false);


var promptService = {
  
  alert: function alert(aParent,
                        aDialogTitle,
                        aText) {
    const title = "Bug 299716 test ";
    var keyChar = aText.charAt(title.length).toLowerCase();
    var id = "bug299716-" + keyChar + "@tests.mozilla.org";
    for (var i = 0; i < ADDONS.length; i++) {
      if (ADDONS[i].id != id) {
        continue;
      }

      do_check_false(ADDONS[i].installed);
      break;
    }
  },

  
  alertCheck: function alertCheck(aParent,
                                  aDialogTitle,
                                  aText,
                                  aCheckMsg,
                                  aCheckState) {
    do_throw("Unexpected call to alertCheck!");
  },

  
  confirm: function confirm(aParent,
                            aDialogTitle,
                            aText) {
    do_throw("Unexpected call to confirm!");
  },

  
  confirmCheck: function confirmCheck(aParent,
                                      aDialogTitle,
                                      aText,
                                      aCheckMsg,
                                      aCheckState) {
    do_throw("Unexpected call to confirmCheck!");
  },

  
  confirmEx: function confirmEx(aParent,
                                aDialogTitle,
                                aText,
                                aButtonFlags,
                                aButton0Title,
                                aButton1Title,
                                aButton2Title,
                                aCheckMsg,
                                aCheckState) {
    do_throw("Unexpected call to confirmEx!");
  },

  
  prompt: function prompt(aParent,
                          aDialogTitle,
                          aText,
                          aValue,
                          aCheckMsg,
                          aCheckState) {
    do_throw("Unexpected call to prompt!");
  },

  
  promptUsernameAndPassword:
  function promptUsernameAndPassword(aParent,
                                     aDialogTitle,
                                     aText,
                                     aUsername,
                                     aPassword,
                                     aCheckMsg,
                                     aCheckState) {
    do_throw("Unexpected call to promptUsernameAndPassword!");
  },

  
  promptPassword: function promptPassword(aParent,
                                          aDialogTitle,
                                          aText,
                                          aPassword,
                                          aCheckMsg,
                                          aCheckState) {
    do_throw("Unexpected call to promptPassword!");
  },

  
  select: function select(aParent,
                          aDialogTitle,
                          aText,
                          aCount,
                          aSelectList,
                          aOutSelection) {
    do_throw("Unexpected call to select!");
  },

  
  QueryInterface: function QueryInterface(iid) {
    if (iid.equals(Components.interfaces.nsIPromptService)
     || iid.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

var PromptServiceFactory = {
  createInstance: function createInstance(outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return promptService.QueryInterface(iid);
  }
};
const nsIComponentRegistrar = Components.interfaces.nsIComponentRegistrar;
var registrar = Components.manager.QueryInterface(nsIComponentRegistrar);
const psID = Components.ID("{6cc9c9fe-bc0b-432b-a410-253ef8bcc699}");
registrar.registerFactory(psID,
                          "PromptService",
                          "@mozilla.org/embedcomp/prompt-service;1",
                          PromptServiceFactory);

const installListener = {
  
  onDownloadStarted: function(aAddon) {
    
  },

  onDownloadEnded: function(aAddon) {
    
  },

  onInstallStarted: function(aAddon) {
    
  },

  onCompatibilityCheckStarted: function(aAddon) {
    
  },

  onCompatibilityCheckEnded: function(aAddon, aStatus) {
    
  },

  onInstallEnded: function(aAddon, aStatus) {
    
  },

  onInstallsCompleted: function() {
    next_test();
  },

  onDownloadProgress: function onProgress(aAddon, aValue, aMaxValue) {
    
  }
};


const checkListener = {
  
  onUpdateStarted: function onUpdateStarted() {
    
  },

  
  onUpdateEnded: function onUpdateEnded() {
    next_test();
  },

  
  onAddonUpdateStarted: function onAddonUpdateStarted(aAddon) {
    
  },

  
  onAddonUpdateEnded: function onAddonUpdateEnded(aAddon, aStatus) {
    for (var i = 0; i < ADDONS.length; i++) {
      if (ADDONS[i].id == aAddon.id) {
        ADDONS[i].newItem = aAddon;
        return;
      }
    }
  }
}


do_load_httpd_js();
var testserver;
var updateItems = [];


const DELAY = 2000;

var ADDONS = [
  
  {
    id: "bug299716-a@tests.mozilla.org",
    addon: "test_bug299716_a_1",
    installed: true,
    item: null,
    newItem: null
  },

  
  {
    id: "bug299716-b@tests.mozilla.org",
    addon: "test_bug299716_b_1",
    installed: true,
    item: null,
    newItem: null
  },

  
  {
    id: "bug299716-c@tests.mozilla.org",
    addon: "test_bug299716_c_1",
    installed: true,
    item: null,
    newItem: null
  },

  
  {
    id: "bug299716-d@tests.mozilla.org",
    addon: "test_bug299716_d_1",
    installed: true,
    item: null,
    newItem: null
  },

  
  {
    id: "bug299716-e@tests.mozilla.org",
    addon: "test_bug299716_e_1",
    installed: false,
    item: null,
    newItem: null,
    failedAppName: "XPCShell"
  },

  
  {
    id: "bug299716-f@tests.mozilla.org",
    addon: "test_bug299716_f_1",
    installed: false,
    item: null,
    newItem: null,
    failedAppName: "XPCShell"
  },

  
  {
    id: "bug299716-g@tests.mozilla.org",
    addon: "test_bug299716_g_1",
    installed: false,
    item: null,
    newItem: null,
    failedAppName: "Toolkit"
  },
];

var currentAddonObj = null;
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

  
  startupEM();
  dump("\n\n*** INSTALLING NEW ITEMS\n\n");

  for (var i = 0; i < ADDONS.length; i++) {
    gEM.installItemFromFile(do_get_addon(ADDONS[i].addon),
                            NS_INSTALL_LOCATION_APPPROFILE);
  }
  dump("\n\n*** DONE INSTALLING NEW ITEMS\n\n");

  do_test_pending();

  
  do_timeout(DELAY, run_test_pt2);
}




function run_test_pt2() {
  dump("\n\n*** RESTARTING EXTENSION MANAGER\n\n");
  restartEM();

  
  for (var i = 0; i < ADDONS.length; i++) {
    var item = gEM.getItemForID(ADDONS[i].id);
    do_check_item(item, "0.1", ADDONS[i]);
    ADDONS[i].item = item;
    updateItems[updateItems.length] = item;
  }

  dump("\n\n*** REQUESTING UPDATE\n\n");
  
  next_test = run_test_pt3;
  try {
    gEM.update(updateItems,
               updateItems.length,
               Components.interfaces.nsIExtensionManager.UPDATE_CHECK_NEWVERSION,
               checkListener);
    do_throw("Shouldn't reach here!");
  } catch (e if (e instanceof Components.interfaces.nsIException &&
                 e.result == Components.results.NS_ERROR_ILLEGAL_VALUE)) {
    
  }

  var addonsArray = [];
  for (var i = 0; i < ADDONS.length; i++) {
    if (ADDONS[i].item) {
      addonsArray.push(ADDONS[i].item);
    }
  }
  gEM.update(addonsArray,
             addonsArray.length,
             Components.interfaces.nsIExtensionManager.UPDATE_CHECK_NEWVERSION,
             checkListener);
}




function run_test_pt3() {
  
  var addonsArray = [];
  for (var i = 0; i < ADDONS.length; i++) {
    addonsArray.push(ADDONS[i].newItem);
  }
  dump("\n\n*** UPDATING " + addonsArray.length + " ITEMS\n\n");

  
  next_test = run_test_pt4;

  
  try {
    gEM.addDownloads(addonsArray, addonsArray.length, null);
    do_throw("Shouldn't reach here!");
  } catch (e if (e instanceof Components.interfaces.nsIException &&
                 e.result == Components.results.NS_ERROR_ILLEGAL_VALUE)) {
    
  }

  for (i = addonsArray.length - 1; i >= 0; i--) {
    if (!addonsArray[i]) {
      addonsArray.splice(i, 1);
    }
  }

  gEM.addInstallListener(installListener);

  do_check_true(addonsArray.length > 0);
  gEM.addDownloads(addonsArray, addonsArray.length, null);
}




function run_test_pt4() {
  dump("\n\n*** RESTARTING EXTENSION MANAGER\n\n");
  restartEM();

  dump("\n\n*** FINAL CHECKS\n\n");
  for (var i = 0; i < ADDONS.length; i++) {
    var item = gEM.getItemForID(ADDONS[i].id);
    do_check_item(item, "0.2", ADDONS[i]);
  }

  testserver.stop(do_test_finished);
}
