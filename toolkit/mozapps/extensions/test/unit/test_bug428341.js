






































gPrefs.setCharPref("extensions.update.url", "http://localhost:4444/");

do_load_httpd_js();
var testserver;


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

var gNextState = null;
var gIndex = null;
var ADDONS = [
{
  id: null,
  addon: "test_bug428341_1",
  error: -8,
  checksCompatibility: false
},
{
  id: null,
  addon: "test_bug428341_2",
  error: -8,
  checksCompatibility: false
},
{
  id: null,
  addon: "test_bug428341_3",
  error: -8,
  checksCompatibility: false
},
{
  id: "{invalid-guid}",
  addon: "test_bug428341_4",
  error: -2,
  checksCompatibility: false
},
{
  id: "bug428341_5@tests.mozilla.org",
  addon: "test_bug428341_5",
  error: -5,
  checksCompatibility: false
},
{
  id: "bug428341_6@tests.mozilla.org",
  addon: "test_bug428341_6",
  error: -7,
  checksCompatibility: false
},
{
  id: "bug428341_7@tests.mozilla.org",
  addon: "test_bug428341_7",
  error: -3,
  checksCompatibility: false
},
{
  id: "bug428341_8@tests.mozilla.org",
  addon: "test_bug428341_8",
  error: -3,
  checksCompatibility: true
},
{
  id: "bug428341_9@tests.mozilla.org",
  addon: "test_bug428341_9",
  error: -3,
  checksCompatibility: true
}
];


var installListener = {
  onDownloadStarted: function(aAddon) {
    do_throw("onDownloadStarted should not be called for a direct install");
  },

  onDownloadEnded: function(aAddon) {
    do_throw("onDownloadEnded should not be called for a direct install");
  },

  onInstallStarted: function(aAddon) {
    var state = "onInstallStarted";
    if (gNextState != state) {
      do_throw("Encountered invalid state installing add-on " + ADDONS[gIndex].addon +
               ". Saw " + state + " but expected " + gNextState);
    }

    dump("Seen " + state + " for add-on " + ADDONS[gIndex].addon + "\n");

    if (ADDONS[gIndex].checksCompatibility)
      gNextState = "onCompatibilityCheckStarted";
    else
      gNextState = "onInstallEnded";
  },

  onCompatibilityCheckStarted: function(aAddon) {
    var state = "onCompatibilityCheckStarted";
    if (gNextState != state) {
      do_throw("Encountered invalid state installing add-on " + ADDONS[gIndex].addon +
               ". Saw " + state + " but expected " + gNextState);
    }

    dump("Seen " + state + " for add-on " + ADDONS[gIndex].addon + "\n");

    gNextState = "onCompatibilityCheckEnded";
  },

  onCompatibilityCheckEnded: function(aAddon, aStatus) {
    var state = "onCompatibilityCheckEnded";
    if (gNextState != state) {
      do_throw("Encountered invalid state installing add-on " + ADDONS[gIndex].addon +
               ". Saw " + state + " but expected " + gNextState);
    }

    dump("Seen " + state + " for add-on " + ADDONS[gIndex].addon + "\n");

    gNextState = "onInstallEnded";
  },

  onInstallEnded: function(aAddon, aStatus) {
    var state = "onInstallEnded";
    if (gNextState != state) {
      do_throw("Encountered invalid state installing add-on " + ADDONS[gIndex].addon +
               ". Saw " + state + " but expected " + gNextState);
    }

    if (ADDONS[gIndex].id && ADDONS[gIndex].id != aAddon.id)
      do_throw("Add-on " + ADDONS[gIndex].addon + " had an incorrect id " + aAddon.id);

    if (aStatus != ADDONS[gIndex].error)
      do_throw("Add-on " + ADDONS[gIndex].addon + " returned incorrect status " + aStatus + ".");

    dump("Seen " + state + " for add-on " + ADDONS[gIndex].addon + "\n");

    gNextState = "onInstallsCompleted";
  },

  onInstallsCompleted: function() {
    var state = "onInstallsCompleted";
    if (gNextState != state) {
      do_throw("Encountered invalid state installing add-on " + ADDONS[gIndex].addon +
               ". Saw " + state + " but expected " + gNextState);
    }

    dump("Seen " + state + " for add-on " + ADDONS[gIndex].addon + "\n");

    gNextState = null;
    gIndex++;
    installNextAddon();
  },

  onDownloadProgress: function onProgress(aAddon, aValue, aMaxValue) {
    do_throw("onDownloadProgress should not be called for a direct install");
  }
}

function installNextAddon() {
  if (gIndex >= ADDONS.length) {
    testserver.stop();
    do_test_finished();
    return;
  }

  gNextState = "onInstallStarted";
  try {
    dump("Installing add-on " + ADDONS[gIndex].addon + "\n");
    gEM.installItemFromFile(do_get_addon(ADDONS[gIndex].addon),
                            NS_INSTALL_LOCATION_APPPROFILE);
  }
  catch (e) {
    do_throw("Exception installing add-on " + ADDONS[gIndex].addon + " " + e);
  }
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");
  startupEM();
  gEM.addInstallListener(installListener);
  gIndex = 0;
  do_test_pending();

  
  testserver = new nsHttpServer();
  testserver.start(4444);

  installNextAddon();
}
