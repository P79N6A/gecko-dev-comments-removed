var rootDir = getRootDirectory(gTestPath);
const gTestRoot = rootDir;
const gHttpTestRoot = rootDir.replace("chrome://mochitests/content/", "http://127.0.0.1:8888/");

var gTestBrowser = null;
var gNextTest = null;
var gPluginHost = Components.classes["@mozilla.org/plugin/host;1"].getService(Components.interfaces.nsIPluginHost);
var gRunNextTestAfterPluginRemoved = false;

Components.utils.import("resource://gre/modules/Services.jsm");

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

  prepareTest(test1, gHttpTestRoot + "plugin_outsideScrollArea.html");
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


function addPlugin(x, y) {
  let doc = gTestBrowser.contentDocument;
  let p = doc.createElement('embed');

  p.setAttribute('id', 'test');
  p.setAttribute('type', 'application/x-test');
  p.style.left = x.toString() + 'px';
  p.style.bottom = y.toString() + 'px';

  doc.getElementById('container').appendChild(p);
}




function test1() {
  addPlugin(0, -200);
  executeSoon(runAfterPluginBindingAttached(test2));
}

function test2() {
  let doc = gTestBrowser.contentDocument;
  let plugin = doc.getElementById("test");
  let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
  ok(overlay, "Test 2, Should have an overlay.");
  ok(overlay.classList.contains("visible"), "Test 2, Overlay should be visible");

  prepareTest(test3, gHttpTestRoot + "plugin_outsideScrollArea.html");
}

function test3() {
  addPlugin(0, -410);
  executeSoon(runAfterPluginBindingAttached(test4));
}

function test4() {
  let doc = gTestBrowser.contentDocument;
  let plugin = doc.getElementById("test");
  let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
  ok(overlay, "Test 4, Should have an overlay.");
  ok(overlay.classList.contains("visible"), "Test 4, Overlay should be visible");

  prepareTest(test5, gHttpTestRoot + "plugin_outsideScrollArea.html");
}

function test5() {
  addPlugin(-600, 0);
  executeSoon(runAfterPluginBindingAttached(test6));
}

function test6() {
  let doc = gTestBrowser.contentDocument;
  let plugin = doc.getElementById("test");
  let overlay = doc.getAnonymousElementByAttribute(plugin, "anonid", "main");
  ok(overlay, "Test 6, Should have an overlay.");
  ok(!overlay.classList.contains("visible"), "Test 6, Overlay should be hidden");

  finishTest();
}
