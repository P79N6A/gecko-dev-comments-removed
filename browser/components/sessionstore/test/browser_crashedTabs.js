


"use strict";

const PAGE_1 = "data:text/html,<html><body>A%20regular,%20everyday,%20normal%20page.";
const PAGE_2 = "data:text/html,<html><body>Another%20regular,%20everyday,%20normal%20page.";









function crashBrowser(browser) {
  
  
  
  
  let frame_script = () => {
    const Cu = Components.utils;
    Cu.import("resource://gre/modules/ctypes.jsm");

    let dies = function() {
      let zero = new ctypes.intptr_t(8);
      let badptr = ctypes.cast(zero, ctypes.PointerType(ctypes.int32_t));
      badptr.contents
    };

    dump("Et tu, Brute?");
    dies();
  }

  let crashCleanupPromise = new Promise((resolve, reject) => {
    let observer = (subject, topic, data) => {
      is(topic, 'ipc:content-shutdown', 'Received correct observer topic.');
      ok(subject instanceof Ci.nsIPropertyBag2,
         'Subject implements nsIPropertyBag2.');
      
      
      if (!subject.hasKey("abnormal")) {
        info("This is a normal termination and isn't the one we are looking for...");
        return;
      }

      let dumpID;
      if ('nsICrashReporter' in Ci) {
        dumpID = subject.getPropertyAsAString('dumpID');
        ok(dumpID, "dumpID is present and not an empty string");
      }

      if (dumpID) {
        let minidumpDirectory = getMinidumpDirectory();
        removeFile(minidumpDirectory, dumpID + '.dmp');
        removeFile(minidumpDirectory, dumpID + '.extra');
      }

      Services.obs.removeObserver(observer, 'ipc:content-shutdown');
      resolve();
    };

    Services.obs.addObserver(observer, 'ipc:content-shutdown');
  });

  let aboutTabCrashedLoadPromise = new Promise((resolve, reject) => {
    browser.addEventListener("AboutTabCrashedLoad", function onCrash() {
      browser.removeEventListener("AboutTabCrashedLoad", onCrash, false);
      resolve();
    }, false, true);
  });

  
  
  let mm = browser.messageManager;
  mm.loadFrameScript("data:,(" + frame_script.toString() + ")();", false);
  return Promise.all([crashCleanupPromise, aboutTabCrashedLoadPromise]);
}






function getMinidumpDirectory() {
  let dir = Services.dirsvc.get('ProfD', Ci.nsIFile);
  dir.append("minidumps");
  return dir;
}










function removeFile(directory, filename) {
  let file = directory.clone();
  file.append(filename);
  if (file.exists()) {
    file.remove(false);
  }
}













function promiseContentDocumentURIEquals(browser, URI) {
  return new Promise((resolve, reject) => {
    let frame_script = () => {
      sendAsyncMessage("test:documenturi", {
        uri: content.document.documentURI,
      });
    };

    let mm = browser.messageManager;
    mm.addMessageListener("test:documenturi", function onMessage(message) {
      mm.removeMessageListener("test:documenturi", onMessage);
      let contentURI = message.data.uri;
      if (contentURI == URI) {
        resolve();
      } else {
        reject(`Content has URI ${contentURI} which does not match ${URI}`);
      }
    });

    mm.loadFrameScript("data:,(" + frame_script.toString() + ")();", false);
  });
}













function promiseHistoryLength(browser, length) {
  return new Promise((resolve, reject) => {
    let frame_script = () => {
      sendAsyncMessage("test:historylength", {
        length: content.history.length,
      });
    };

    let mm = browser.messageManager;
    mm.addMessageListener("test:historylength", function onMessage(message) {
      mm.removeMessageListener("test:historylength", onMessage);
      let contentLength = message.data.length;
      if (contentLength == length) {
        resolve();
      } else {
        reject(`Content has window.history.length ${contentLength} which does ` +
               `not equal expected ${length}`);
      }
    });

    mm.loadFrameScript("data:,(" + frame_script.toString() + ")();", false);
  });
}





add_task(function test_crash_page_not_in_history() {
  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  let browser = newTab.linkedBrowser;
  ok(browser.isRemoteBrowser, "Should be a remote browser");
  yield promiseBrowserLoaded(browser);

  browser.loadURI(PAGE_1);
  yield promiseBrowserLoaded(browser);
  TabState.flush(browser);

  
  yield crashBrowser(browser);
  
  TabState.flush(browser);

  
  
  let {entries} = JSON.parse(ss.getTabState(newTab));
  is(entries.length, 1, "Should have a single history entry");
  is(entries[0].url, PAGE_1,
    "Single entry should be the page we visited before crashing");

  gBrowser.removeTab(newTab);
});






add_task(function test_revived_history_from_remote() {
  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  let browser = newTab.linkedBrowser;
  ok(browser.isRemoteBrowser, "Should be a remote browser");
  yield promiseBrowserLoaded(browser);

  browser.loadURI(PAGE_1);
  yield promiseBrowserLoaded(browser);
  TabState.flush(browser);

  
  yield crashBrowser(browser);
  
  TabState.flush(browser);

  
  
  browser.loadURI(PAGE_2);
  yield promiseBrowserLoaded(browser);
  ok(browser.isRemoteBrowser, "Should be a remote browser");
  TabState.flush(browser);

  
  
  let {entries} = JSON.parse(ss.getTabState(newTab));
  is(entries.length, 2, "Should have two history entries");
  is(entries[0].url, PAGE_1,
    "First entry should be the page we visited before crashing");
  is(entries[1].url, PAGE_2,
    "Second entry should be the page we visited after crashing");

  gBrowser.removeTab(newTab);
});






add_task(function test_revived_history_from_non_remote() {
  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  let browser = newTab.linkedBrowser;
  ok(browser.isRemoteBrowser, "Should be a remote browser");
  yield promiseBrowserLoaded(browser);

  browser.loadURI(PAGE_1);
  yield promiseBrowserLoaded(browser);
  TabState.flush(browser);

  
  yield crashBrowser(browser);
  
  TabState.flush(browser);

  
  
  browser.loadURI("about:mozilla");
  yield promiseBrowserLoaded(browser);
  ok(!browser.isRemoteBrowser, "Should not be a remote browser");
  TabState.flush(browser);

  
  
  let {entries} = JSON.parse(ss.getTabState(newTab));
  is(entries.length, 2, "Should have two history entries");
  is(entries[0].url, PAGE_1,
    "First entry should be the page we visited before crashing");
  is(entries[1].url, "about:mozilla",
    "Second entry should be the page we visited after crashing");

  gBrowser.removeTab(newTab);
});





add_task(function test_revive_tab_from_session_store() {
  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  let browser = newTab.linkedBrowser;
  ok(browser.isRemoteBrowser, "Should be a remote browser");
  yield promiseBrowserLoaded(browser);

  browser.loadURI(PAGE_1);
  yield promiseBrowserLoaded(browser);

  browser.loadURI(PAGE_2);
  yield promiseBrowserLoaded(browser);

  TabState.flush(browser);

  
  yield crashBrowser(browser);
  
  TabState.flush(browser);

  
  SessionStore.reviveCrashedTab(newTab);
  yield promiseBrowserLoaded(browser);

  
  
  
  
  yield promiseContentDocumentURIEquals(browser, PAGE_2);

  
  yield promiseHistoryLength(browser, 2);

  gBrowser.removeTab(newTab);
});