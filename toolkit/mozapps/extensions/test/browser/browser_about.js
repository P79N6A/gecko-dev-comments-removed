










var gManagerWindow;

const URI_ABOUT_DEFAULT = "chrome://mozapps/content/extensions/about.xul";
const URI_ABOUT_CUSTOM = CHROMEROOT + "addon_about.xul";

 add_task(function* test() {
  var gProvider = new MockProvider();
  gProvider.createAddons([{
    id: "test1@tests.mozilla.org",
    name: "Test add-on 1",
    description: "foo"
  },
  {
    id: "test2@tests.mozilla.org",
    name: "Test add-on 2",
    description: "bar",
    aboutURL: URI_ABOUT_CUSTOM
  }]);

  gManagerWindow = yield open_manager("addons://list/extension");

  yield test_about_window("Test add-on 1", URI_ABOUT_DEFAULT);
  yield test_about_window("Test add-on 2", URI_ABOUT_CUSTOM);
  yield close_manager(gManagerWindow);
});

function test_about_window(aAddonItemName, aExpectedAboutUri) {
  return new Promise((resolve, reject) => {
    let addonList = gManagerWindow.document.getElementById("addon-list");
    let selectedItem = null;

    for (let addonItem of addonList.childNodes) {
      if (addonItem.hasAttribute("name") &&
          addonItem.getAttribute("name") === aAddonItemName) {
        selectedItem = addonItem;
        break;
      }
    }
    ok(selectedItem, "Found addon item for " + aAddonItemName);

    info("Waiting for about dialog");
    
    
    Services.ww.registerNotification(function TEST_ww_observer(aSubject, aTopic,
                                                               aData) {
      if (aTopic == "domwindowclosed") {
        Services.ww.unregisterNotification(TEST_ww_observer);

        info("About dialog closing, waiting for focus on browser window");
      } else if (aTopic == "domwindowopened") {
        
        executeSoon(() => {
          info("About dialog opened, waiting for focus");

          let win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);
          waitForFocus(function() {
            info("Saw about dialog");

            is(win.location,
               aExpectedAboutUri,
               "The correct add-on about window should have opened");

            is(win.arguments && win.arguments[0] && win.arguments[0].name,
               aAddonItemName,
               "window.arguments[0] should refer to the add-on object");

            win.close();
          }, win);
        });
      } else {
        info("Got window notification " + aTopic);
      }
    });

    
    
    
    gManagerWindow.gViewController.doCommand("cmd_showItemAbout",
                                             selectedItem.mAddon);
    
    waitForFocus(resolve);
  });
}
