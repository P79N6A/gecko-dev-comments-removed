






































Services.prefs.setBoolPref("extensions.checkUpdateSecurity", false);


do_load_httpd_js();
var testserver;

var next_test = null;
var gItemsNotChecked =[];

var ADDONS = [ {id: "bug324121_1@tests.mozilla.org",
                addon: "test_bug324121_1",
                shouldCheck: false },
               {id: "bug324121_2@tests.mozilla.org",
                addon: "test_bug324121_2",
                shouldCheck: true },
               {id: "bug324121_3@tests.mozilla.org",
                addon: "test_bug324121_3",
                shouldCheck: true },
               {id: "bug324121_4@tests.mozilla.org",
                addon: "test_bug324121_4",
                shouldCheck: true },
               {id: "bug324121_5@tests.mozilla.org",
                addon: "test_bug324121_5",
                shouldCheck: false },
               {id: "bug324121_6@tests.mozilla.org",
                addon: "test_bug324121_6",
                shouldCheck: true },
               {id: "bug324121_7@tests.mozilla.org",
                addon: "test_bug324121_7",
                shouldCheck: true },
               {id: "bug324121_8@tests.mozilla.org",
                addon: "test_bug324121_8",
                shouldCheck: true },
               {id: "bug324121_9@tests.mozilla.org",
                addon: "test_bug324121_9",
                shouldCheck: false } ];


var updateListener = {
  pendingCount: 0,

  onUpdateAvailable: function onAddonUpdateEnded(aAddon) {
    switch (aAddon.id) {
      
      case "bug324121_1@tests.mozilla.org":
      
      case "bug324121_5@tests.mozilla.org":
      
      case "bug324121_9@tests.mozilla.org":
        do_throw("Should not have seen an update check for " + aAddon.id);
        break;

      
      case "bug324121_3@tests.mozilla.org":
      
      case "bug324121_4@tests.mozilla.org":
      
      case "bug324121_7@tests.mozilla.org":
      
      case "bug324121_8@tests.mozilla.org":
        do_throw("Should be no update available for " + aAddon.id);
        break;

      
      case "bug324121_2@tests.mozilla.org":
      case "bug324121_6@tests.mozilla.org":
        break;

      default:
        do_throw("Update check for unknown " + aAddon.id);
    }

    
    var pos = gItemsNotChecked.indexOf(aAddon.id);
    gItemsNotChecked.splice(pos, 1);
  },

  onNoUpdateAvailable: function onNoUpdateAvailable(aAddon) {
    switch (aAddon.id) {
      
      case "bug324121_1@tests.mozilla.org":
      
      case "bug324121_5@tests.mozilla.org":
      
      case "bug324121_9@tests.mozilla.org":
        do_throw("Should not have seen an update check for " + aAddon.id);
        break;

      
      case "bug324121_3@tests.mozilla.org":
      
      case "bug324121_4@tests.mozilla.org":
      
      case "bug324121_7@tests.mozilla.org":
      
      case "bug324121_8@tests.mozilla.org":
        break;

      
      case "bug324121_2@tests.mozilla.org":
      case "bug324121_6@tests.mozilla.org":
        do_throw("Should be an update available for " + aAddon.id);
        break;

      default:
        do_throw("Update check for unknown " + aAddon.id);
    }

    
    var pos = gItemsNotChecked.indexOf(aAddon.id);
    gItemsNotChecked.splice(pos, 1);
  },

  onUpdateFinished: function onUpdateFinished(aAddon) {
    if (--this.pendingCount == 0)
      test_complete();
  }
};

function run_test() {
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "2");

  const dataDir = do_get_file("data");

  
  testserver = new nsHttpServer();
  testserver.registerDirectory("/data/", dataDir);
  testserver.start(4444);

  startupManager();

  installAllFiles([do_get_addon(a.addon) for each (a in ADDONS)], function() {
    restartManager();
    AddonManager.getAddonByID(ADDONS[0].id, function(addon) {
      addon.userDisabled = true;
      restartManager();

      AddonManager.getAddonsByTypes(["extension"], function(installedItems) {
        var items = [];

        for (let k = 0; k < ADDONS.length; k++) {
          for (let i = 0; i < installedItems.length; i++) {
            if (ADDONS[k].id != installedItems[i].id)
              continue;
            if (installedItems[i].userDisabled)
              continue;

            if (ADDONS[k].shouldCheck == installedItems[i].isCompatibleWith("3", "3")) {
              do_throw(installedItems[i].id + " had the wrong compatibility: " +
                installedItems[i].isCompatibleWith("3", "3"));
            }

            if (ADDONS[k].shouldCheck) {
              gItemsNotChecked.push(ADDONS[k].id);
              updateListener.pendingCount++;
              installedItems[i].findUpdates(updateListener,
                                            AddonManager.UPDATE_WHEN_USER_REQUESTED,
                                            "3", "3");
            }
          }
        }
      });
    });
  });
}

function test_complete() {
  do_check_eq(gItemsNotChecked.length, 0);
  testserver.stop(do_test_finished);
}
