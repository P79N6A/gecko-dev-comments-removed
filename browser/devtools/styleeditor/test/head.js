


const FRAME_SCRIPT_UTILS_URL = "chrome://browser/content/devtools/frame-script-utils.js"
const TEST_BASE = "chrome://mochitests/content/browser/browser/devtools/styleeditor/test/";
const TEST_BASE_HTTP = "http://example.com/browser/browser/devtools/styleeditor/test/";
const TEST_BASE_HTTPS = "https://example.com/browser/browser/devtools/styleeditor/test/";
const TEST_HOST = 'mochi.test:8888';

let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let TargetFactory = devtools.TargetFactory;
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});




let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
Services.scriptloader.loadSubScript(testDir + "../../../commandline/test/helpers.js", this);

gDevTools.testing = true;
SimpleTest.registerCleanupFunction(() => {
  gDevTools.testing = false;
});







function addTab(url, win) {
  info("Adding a new tab with URL: '" + url + "'");
  let def = promise.defer();

  let targetWindow = win || window;
  let targetBrowser = targetWindow.gBrowser;

  let tab = targetBrowser.selectedTab = targetBrowser.addTab(url);
  targetBrowser.selectedBrowser.addEventListener("load", function onload() {
    targetBrowser.selectedBrowser.removeEventListener("load", onload, true);
    info("URL '" + url + "' loading complete");
    def.resolve(tab);
  }, true);

  return def.promise;
}






function navigateTo(url) {
  let navigating = promise.defer();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    navigating.resolve();
  }, true);
  content.location = url;
  return navigating.promise;
}

function* cleanup()
{
  while (gBrowser.tabs.length > 1) {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    yield gDevTools.closeToolbox(target);

    gBrowser.removeCurrentTab();
  }
}





let openStyleEditorForURL = Task.async(function* (url, win) {
  let tab = yield addTab(url, win);
  let target = TargetFactory.forTab(tab);
  let toolbox = yield gDevTools.showToolbox(target, "styleeditor");
  let panel = toolbox.getPanel("styleeditor");
  let ui = panel.UI;

  return { tab, toolbox, panel, ui };
});







function loadCommonFrameScript(tab) {
  let browser = tab ? tab.linkedBrowser : gBrowser.selectedBrowser;

  browser.messageManager.loadFrameScript(FRAME_SCRIPT_UTILS_URL, false);
}




















function executeInContent(name, data={}, objects={}, expectResponse=true) {
  let mm = gBrowser.selectedBrowser.messageManager;

  mm.sendAsyncMessage(name, data, objects);
  if (expectResponse) {
    return waitForContentMessage(name);
  } else {
    return promise.resolve();
  }
}








function waitForContentMessage(name) {
  let mm = gBrowser.selectedBrowser.messageManager;

  let def = promise.defer();
  mm.addMessageListener(name, function onMessage(msg) {
    mm.removeMessageListener(name, onMessage);
    def.resolve(msg);
  });
  return def.promise;
}

registerCleanupFunction(cleanup);
