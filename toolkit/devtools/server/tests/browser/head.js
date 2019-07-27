



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
const {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
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

  yield new Promise(resolve => {
    let isBlank = url == "about:blank";
    waitForFocus(resolve, content, isBlank);
  });;

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
  return new Promise(resolve => {
    client.connect(() => {
      client.listTabs(tabs => {
        resolve(tabs.tabs[tabs.selected]);
      });
    });
  });
}






function closeDebuggerClient(client) {
  return new Promise(resolve => client.close(resolve));
}









function once(target, eventName, useCapture=false) {
  info("Waiting for event: '" + eventName + "' on " + target + ".");

  return new Promise(resolve => {

    for (let [add, remove] of [
      ["addEventListener", "removeEventListener"],
      ["addListener", "removeListener"],
      ["on", "off"]
    ]) {
      if ((add in target) && (remove in target)) {
        target[add](eventName, function onEvent(...aArgs) {
          info("Got event: '" + eventName + "' on " + target + ".");
          target[remove](eventName, onEvent, useCapture);
          resolve(...aArgs);
        }, useCapture);
        break;
      }
    }
  });
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
