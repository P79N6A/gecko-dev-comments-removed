



let gTestBrowser = null;

let propBagProperties = {
  pluginName: "GlobalTestPlugin",
  pluginDumpID: "1234",
  browserDumpID: "5678",
  submittedCrashReport: false
}




function test() {
  waitForExplicitFinish();
  let tab = gBrowser.loadOneTab("about:blank", { inBackground: false });
  gTestBrowser = gBrowser.getBrowserForTab(tab);
  gTestBrowser.addEventListener("PluginCrashed", onCrash, false);
  gTestBrowser.addEventListener("load", onPageLoad, true);

  registerCleanupFunction(function cleanUp() {
    gTestBrowser.removeEventListener("PluginCrashed", onCrash, false);
    gTestBrowser.removeEventListener("load", onPageLoad, true);
    gBrowser.removeTab(tab);
  });
}

function onPageLoad() {
  executeSoon(generateCrashEvent);
}

function generateCrashEvent() {
  let window = gTestBrowser.contentWindow;
  let propBag = Cc["@mozilla.org/hash-property-bag;1"]
                  .createInstance(Ci.nsIWritablePropertyBag);
  for (let [name, val] of Iterator(propBagProperties)) {
    propBag.setProperty(name, val);
  }

  let event = window.document.createEvent("CustomEvent");
  event.initCustomEvent("PluginCrashed", true, true, propBag);
  window.dispatchEvent(event);
}


function onCrash(event) {
  let target = event.target;
  is (target, gTestBrowser.contentWindow, "Event target is the window.");

  let propBag = event.detail.QueryInterface(Ci.nsIPropertyBag2);
  for (let [name, val] of Iterator(propBagProperties)) {
    let type = typeof val;
    let propVal = type == "string"
                  ? propBag.getPropertyAsAString(name)
                  : propBag.getPropertyAsBool(name);
    is (propVal, val, "Correct property in detail propBag: " + name + ".");
  }

  waitForNotificationBar("plugin-crashed", gTestBrowser, (notification) => {
    let notificationBox = gBrowser.getNotificationBox(gTestBrowser);
    ok(notification, "Infobar was shown.");
    is(notification.priority, notificationBox.PRIORITY_WARNING_MEDIUM, "Correct priority.");
    is(notification.getAttribute("label"), "The GlobalTestPlugin plugin has crashed.", "Correct message.");
    finish();
  });
}
