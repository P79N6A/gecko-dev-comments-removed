



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
const {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const {devtools: {require}} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const {DebuggerClient} = Cu.import("resource://gre/modules/devtools/dbg-client.jsm", {});
const {DebuggerServer} = Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});

const PATH = "browser/toolkit/devtools/server/tests/browser/";
const MAIN_DOMAIN = "http://test1.example.org/" + PATH;
const ALT_DOMAIN = "http://sectest1.example.org/" + PATH;
const ALT_DOMAIN_SECURED = "https://sectest1.example.org:443/" + PATH;


waitForExplicitFinish();




function asyncTest(generator) {
  return () => Task.spawn(generator).then(null, ok.bind(null, false)).then(finish);
}






let addTab = Task.async(function* (url) {
  info("Adding a new tab with URL: '" + url + "'");
  let tab = gBrowser.selectedTab = gBrowser.addTab();
  let loaded = once(gBrowser.selectedBrowser, "load", true);

  content.location = url;
  yield loaded;

  info("URL '" + url + "' loading complete");

  let def = promise.defer();
  let isBlank = url == "about:blank";
  waitForFocus(def.resolve, content, isBlank);

  yield def.promise;

  return tab.linkedBrowser.contentWindow.document;
});

function initDebuggerServer() {
  try {
    
    
    DebuggerServer.destroy();
  } catch (ex) { }
  DebuggerServer.init(() => true);
  DebuggerServer.addBrowserActors();
}







function connectDebuggerClient(client) {
  let def = promise.defer();
  client.connect(() => {
    client.listTabs(tabs => {
      def.resolve(tabs.tabs[tabs.selected]);
    });
  });
  return def.promise;
}






function closeDebuggerClient(client) {
  let def = promise.defer();
  client.close(def.resolve);
  return def.promise;
}









function once(target, eventName, useCapture=false) {
  info("Waiting for event: '" + eventName + "' on " + target + ".");

  let deferred = promise.defer();

  for (let [add, remove] of [
    ["addEventListener", "removeEventListener"],
    ["addListener", "removeListener"],
    ["on", "off"]
  ]) {
    if ((add in target) && (remove in target)) {
      target[add](eventName, function onEvent(...aArgs) {
        info("Got event: '" + eventName + "' on " + target + ".");
        target[remove](eventName, onEvent, useCapture);
        deferred.resolve.apply(deferred, aArgs);
      }, useCapture);
      break;
    }
  }

  return deferred.promise;
}





function forceCollections() {
  Cu.forceGC();
  Cu.forceCC();
  Cu.forceShrinkingGC();
}

registerCleanupFunction(function tearDown() {
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});
