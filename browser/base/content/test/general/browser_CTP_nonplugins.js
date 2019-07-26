var rootDir = getRootDirectory(gTestPath);
const gTestRoot = rootDir;
const gHttpTestRoot = rootDir.replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");

var gTestBrowser = null;
var gNextTest = null;
var gPluginHost = Components.classes["@mozilla.org/plugin/host;1"].getService(Components.interfaces.nsIPluginHost);
var gRunNextTestAfterPluginRemoved = false;

Components.utils.import("resource://gre/modules/Services.jsm");




function TabOpenListener(url, opencallback, closecallback) {
  this.url = url;
  this.opencallback = opencallback;
  this.closecallback = closecallback;

  gBrowser.tabContainer.addEventListener("TabOpen", this, false);
}

TabOpenListener.prototype = {
  url: null,
  opencallback: null,
  closecallback: null,
  tab: null,
  browser: null,

  handleEvent: function(event) {
    if (event.type == "TabOpen") {
      gBrowser.tabContainer.removeEventListener("TabOpen", this, false);
      this.tab = event.originalTarget;
      this.browser = this.tab.linkedBrowser;
      gBrowser.addEventListener("pageshow", this, false);
    } else if (event.type == "pageshow") {
      if (event.target.location.href != this.url)
        return;
      gBrowser.removeEventListener("pageshow", this, false);
      this.tab.addEventListener("TabClose", this, false);
      var url = this.browser.contentDocument.location.href;
      is(url, this.url, "Should have opened the correct tab");
      this.opencallback(this.tab, this.browser.contentWindow);
    } else if (event.type == "TabClose") {
      if (event.originalTarget != this.tab)
        return;
      this.tab.removeEventListener("TabClose", this, false);
      this.opencallback = null;
      this.tab = null;
      this.browser = null;
      
      executeSoon(this.closecallback);
      this.closecallback = null;
    }
  }
};

function test() {
  waitForExplicitFinish();
  registerCleanupFunction(function() {
    clearAllPluginPermissions();
    Services.prefs.clearUserPref("extensions.blocklist.suppressUI");
    gTestBrowser.removeEventListener("PluginRemoved", handlePluginRemoved, true, true);
  });
  Services.prefs.setBoolPref("extensions.blocklist.suppressUI", true);

  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  gTestBrowser = gBrowser.selectedBrowser;
  gTestBrowser.addEventListener("load", pageLoad, true);
  gTestBrowser.addEventListener("PluginRemoved", handlePluginRemoved, true, true);

  Services.prefs.setBoolPref("plugins.click_to_play", true);
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  prepareTest(runAfterPluginBindingAttached(test1), gHttpTestRoot + "plugin_two_types.html");
}

function finishTest() {
  clearAllPluginPermissions();
  gTestBrowser.removeEventListener("load", pageLoad, true);
  gBrowser.removeCurrentTab();
  window.focus();
  finish();
}

function pageLoad() {
  
  
  executeSoon(gNextTest);
}

function prepareTest(nextTest, url) {
  gNextTest = nextTest;
  gTestBrowser.contentWindow.location = url;
}





function runAfterPluginBindingAttached(func) {
  return function() {
    let doc = gTestBrowser.contentDocument;
    let elems = doc.getElementsByTagName('embed');
    if (elems.length < 1) {
      elems = doc.getElementsByTagName('object');
    }
    elems[0].clientTop;
    executeSoon(func);
  };
}

function handlePluginRemoved() {
  if (gRunNextTestAfterPluginRemoved) {
    executeSoon(gNextTest);
    gRunNextTestAfterPluginRemoved = false;
  }
}

function runAfterPluginRemoved(func) {
  gNextTest = func;
  gRunNextTestAfterPluginRemoved = true;
}



function test1() {
  let popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 1, Should have a click-to-play notification");

  let plugin = gTestBrowser.contentDocument.getElementById("secondtestA");
  plugin.parentNode.removeChild(plugin);
  plugin = gTestBrowser.contentDocument.getElementById("secondtestB");
  plugin.parentNode.removeChild(plugin);

  let image = gTestBrowser.contentDocument.createElement("object");
  image.type = "image/png";
  image.data = "moz.png";
  gTestBrowser.contentDocument.body.appendChild(image);

  runAfterPluginRemoved(test2);
}

function test2() {
  let popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 2, Should have a click-to-play notification");

  let plugin = gTestBrowser.contentDocument.getElementById("test");
  plugin.parentNode.removeChild(plugin);

  executeSoon(test3);
}

function test3() {
  let popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 3, Should still have a click-to-play notification");

  finishTest();
}
