


const TEST_BASE = "chrome://mochitests/content/browser/browser/devtools/styleeditor/test/";
const TEST_BASE_HTTP = "http://example.com/browser/browser/devtools/styleeditor/test/";
const TEST_BASE_HTTPS = "https://example.com/browser/browser/devtools/styleeditor/test/";
const TEST_HOST = 'mochi.test:8888';

let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let TargetFactory = devtools.TargetFactory;
let {LoadContextInfo} = Cu.import("resource://gre/modules/LoadContextInfo.jsm", {});
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});

let gPanelWindow;
let cache = Cc["@mozilla.org/netwerk/cache-storage-service;1"]
              .getService(Ci.nsICacheStorageService);



let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
Services.scriptloader.loadSubScript(testDir + "../../../commandline/test/helpers.js", this);

gDevTools.testing = true;
SimpleTest.registerCleanupFunction(() => {
  gDevTools.testing = false;
});




function asyncTest(generator) {
  return () => Task.spawn(generator).then(null, ok.bind(null, false)).then(finish);
}






function addTab(url) {
  info("Adding a new tab with URL: '" + url + "'");
  let def = promise.defer();

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    info("URL '" + url + "' loading complete");
    def.resolve(tab);
  }, true);
  content.location = url;

  return def.promise;
}

function* cleanup()
{
  gPanelWindow = null;
  while (gBrowser.tabs.length > 1) {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    yield gDevTools.closeToolbox(target);

    gBrowser.removeCurrentTab();
  }
}

function addTabAndOpenStyleEditors(count, callback, uri) {
  let deferred = promise.defer();
  let currentCount = 0;
  let panel;
  addTabAndCheckOnStyleEditorAdded(p => panel = p, function (editor) {
    currentCount++;
    info(currentCount + " of " + count + " editors opened: "
         + editor.styleSheet.href);
    if (currentCount == count) {
      if (callback) {
        callback(panel);
      }
      deferred.resolve(panel);
    }
  });

  if (uri) {
    content.location = uri;
  }
  return deferred.promise;
}

function addTabAndCheckOnStyleEditorAdded(callbackOnce, callbackOnAdded) {
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openStyleEditorInWindow(window, function (panel) {
      
      callbackOnce(panel);
      
      for (let editor of panel.UI.editors) {
        callbackOnAdded(editor);
      }
      
      panel.UI.on("editor-added", (event, editor) => callbackOnAdded(editor));
    });
  }, true);
}

function openStyleEditorInWindow(win, callback) {
  let target = TargetFactory.forTab(win.gBrowser.selectedTab);
  win.gDevTools.showToolbox(target, "styleeditor").then(function(toolbox) {
    let panel = toolbox.getCurrentPanel();
    gPanelWindow = panel._panelWin;

    panel.UI._alwaysDisableAnimations = true;
    callback(panel);
  });
}

function checkDiskCacheFor(host, done)
{
  let foundPrivateData = false;

  Visitor.prototype = {
    onCacheStorageInfo: function(num, consumption)
    {
      info("disk storage contains " + num + " entries");
    },
    onCacheEntryInfo: function(uri)
    {
      var urispec = uri.asciiSpec;
      info(urispec);
      foundPrivateData |= urispec.contains(host);
    },
    onCacheEntryVisitCompleted: function()
    {
      is(foundPrivateData, false, "web content present in disk cache");
      done();
    }
  };
  function Visitor() {}

  var storage = cache.diskCacheStorage(LoadContextInfo.default, false);
  storage.asyncVisitStorage(new Visitor(), true );
}

registerCleanupFunction(cleanup);
