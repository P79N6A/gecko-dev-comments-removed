var rootDir = getRootDirectory(gTestPath);
const gTestRoot = rootDir;
const gHttpTestRoot = rootDir.replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");

var gTestBrowser = null;
var gNextTest = null;
var gPluginHost = Components.classes["@mozilla.org/plugin/host;1"].getService(Components.interfaces.nsIPluginHost);

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
  });
  Services.prefs.setBoolPref("extensions.blocklist.suppressUI", true);

  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  gTestBrowser = gBrowser.selectedBrowser;
  gTestBrowser.addEventListener("load", pageLoad, true);

  Services.prefs.setBoolPref("plugins.click_to_play", true);
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY);
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY, "Second Test Plug-in");

  prepareTest(test1a, gHttpTestRoot + "plugin_data_url.html");
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


function test1a() {
  let popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 1a, Should have a click-to-play notification");

  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(!objLoadingContent.activated, "Test 1a, Plugin should not be activated");

  gNextTest = runAfterPluginBindingAttached(test1b);
  gTestBrowser.contentDocument.getElementById("data-link-1").click();
}

function test1b() {
  let popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 1b, Should have a click-to-play notification");

  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(!objLoadingContent.activated, "Test 1b, Plugin should not be activated");

  
  popupNotification.reshow();
  PopupNotifications.panel.firstChild._primaryButton.click();

  let condition = function() objLoadingContent.activated;
  waitForCondition(condition, test1c, "Test 1b, Waited too long for plugin to activate");
}

function test1c() {
  clearAllPluginPermissions();
  prepareTest(runAfterPluginBindingAttached(test2a), gHttpTestRoot + "plugin_data_url.html");
}


function test2a() {
  let popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 2a, Should have a click-to-play notification");
  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(!objLoadingContent.activated, "Test 2a, Plugin should not be activated");

  gNextTest = runAfterPluginBindingAttached(test2b);
  gTestBrowser.contentDocument.getElementById("data-link-2").click();
}

function test2b() {
  let notification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(notification, "Test 2b, Should have a click-to-play notification");

  
  notification.reshow();
  is(notification.options.centerActions.length, 2, "Test 2b, Should have two types of plugin in the notification");

  var centerAction = null;
  for (var action of notification.options.centerActions) {
    if (action.pluginName == "Test") {
      centerAction = action;
      break;
    }
  }
  ok(centerAction, "Test 2b, found center action for the Test plugin");

  var centerItem = null;
  for (var item of PopupNotifications.panel.firstChild.childNodes) {
    is(item.value, "block", "Test 2b, all plugins should start out blocked");
    if (item.action == centerAction) {
      centerItem = item;
      break;
    }
  }
  ok(centerItem, "Test 2b, found center item for the Test plugin");

  
  centerItem.value = "allownow";
  PopupNotifications.panel.firstChild._primaryButton.click();

  let plugin = gTestBrowser.contentDocument.getElementById("test1");
  let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  let condition = function() objLoadingContent.activated;
  waitForCondition(condition, test2c, "Test 2b, Waited too long for plugin to activate");
}

function test2c() {
  let plugin = gTestBrowser.contentDocument.getElementById("test1");
  let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(objLoadingContent.activated, "Test 2c, Plugin should be activated");

  clearAllPluginPermissions();
  prepareTest(runAfterPluginBindingAttached(test3a), gHttpTestRoot + "plugin_data_url.html");
}


function test3a() {
  let popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 3a, Should have a click-to-play notification");
  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(!objLoadingContent.activated, "Test 3a, Plugin should not be activated");

  
  popupNotification.reshow();
  PopupNotifications.panel.firstChild._primaryButton.click();

  let condition = function() objLoadingContent.activated;
  waitForCondition(condition, test3b, "Test 3a, Waited too long for plugin to activate");
}

function test3b() {
  gNextTest = test3c;
  gTestBrowser.contentDocument.getElementById("data-link-1").click();
}

function test3c() {
  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(objLoadingContent.activated, "Test 3c, Plugin should be activated");

  clearAllPluginPermissions();
  prepareTest(runAfterPluginBindingAttached(test4b),
              'data:text/html,<embed id="test" style="width: 200px; height: 200px" type="application/x-test"/>');
}


function test4a() {
  let popupNotification = PopupNotifications.getNotification("click-to-play-plugins", gTestBrowser);
  ok(popupNotification, "Test 4a, Should have a click-to-play notification");
  let plugin = gTestBrowser.contentDocument.getElementById("test");
  let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
  ok(!objLoadingContent.activated, "Test 4a, Plugin should not be activated");

  
  popupNotification.reshow();
  PopupNotifications.panel.firstChild._primaryButton.click();

  let condition = function() objLoadingContent.activated;
  waitForCondition(condition, test4b, "Test 4a, Waited too long for plugin to activate");
}

function test4b() {
  clearAllPluginPermissions();
  finishTest();
}
