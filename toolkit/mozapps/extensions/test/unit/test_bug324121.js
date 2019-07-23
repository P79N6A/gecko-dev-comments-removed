






































gPrefs.setBoolPref("extensions.checkUpdateSecurity", false);


do_load_httpd_js();
var testserver;

var next_test = null;

var gItemsNotCheck = [];

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
  onUpdateStarted: function onUpdateStarted() {
  },

  onUpdateEnded: function onUpdateEnded() {
    
    do_check_eq(gItemsNotCheck.length, 0);
    test_complete();
  },

  onAddonUpdateStarted: function onAddonUpdateStarted(aAddon) {
  },

  onAddonUpdateEnded: function onAddonUpdateEnded(aAddon, aStatus) {
    var nsIAddonUpdateCheckListener = Ci.nsIAddonUpdateCheckListener;
    switch (aAddon.id)
    {
      case "bug324121_1@tests.mozilla.org":
        
        do_throw("Update check for disabled add-on " + aAddon.id);
        break;
      case "bug324121_2@tests.mozilla.org":
        
        do_check_eq(aStatus, nsIAddonUpdateCheckListener.STATUS_UPDATE);
        break;
      case "bug324121_3@tests.mozilla.org":
        
        do_check_eq(aStatus, nsIAddonUpdateCheckListener.STATUS_NO_UPDATE);
        break;
      case "bug324121_4@tests.mozilla.org":
        
        do_check_eq(aStatus, nsIAddonUpdateCheckListener.STATUS_FAILURE);
        break;
      case "bug324121_5@tests.mozilla.org":
        
        do_throw("Update check for compatible add-on " + aAddon.id);
        break;
      case "bug324121_6@tests.mozilla.org":
        
        do_check_eq(aStatus, nsIAddonUpdateCheckListener.STATUS_UPDATE);
        break;
      case "bug324121_7@tests.mozilla.org":
        
        do_check_eq(aStatus, nsIAddonUpdateCheckListener.STATUS_NO_UPDATE);
        break;
      case "bug324121_8@tests.mozilla.org":
        
        do_check_eq(aStatus, nsIAddonUpdateCheckListener.STATUS_FAILURE);
        break;
      case "bug324121_9@tests.mozilla.org":
        
        do_throw("Update check for compatible add-on " + aAddon.id);
        break;
      default:
        do_throw("Update check for unknown " + aAddon.id);
    }

    
    var pos = gItemsNotCheck.indexOf(aAddon.id);
    gItemsNotCheck.splice(pos, 1);
  }
};

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "2");

  const dataDir = do_get_file("data");

  
  testserver = new nsHttpServer();
  testserver.registerDirectory("/data/", dataDir);
  testserver.start(4444);

  startupEM();

  var addons = ["test_bug324121_1"];
  for (var k in ADDONS)
    gEM.installItemFromFile(do_get_addon(ADDONS[k].addon),
                            NS_INSTALL_LOCATION_APPPROFILE);

  restartEM();
  gEM.disableItem(ADDONS[0].id);
  restartEM();

  var items = gEM.getIncompatibleItemList("3", "3", Ci.nsIUpdateItem.TYPE_ANY,
                                          false, { });

  
  for (var k in ADDONS) {
    var found = false;
    for (var i = 0; i < items.length; ++i) {
      if (ADDONS[k].id == items[i].id) {
        gItemsNotCheck.push(items[i].id);
        found = true;
        break;
      }
    }
    do_check_true(ADDONS[k].shouldCheck == found);
  }

  gEM.update(items, items.length, Ci.nsIExtensionManager.UPDATE_NOTIFY_NEWVERSION,
             updateListener, Ci.nsIExtensionManager.UPDATE_WHEN_USER_REQUESTED,
             "3", "3");

  do_test_pending();
}

function test_complete() {
  testserver.stop(do_test_finished);
}
