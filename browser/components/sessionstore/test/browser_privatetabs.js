


let Imports = {};
Cu.import("resource://gre/modules/Task.jsm", Imports);
Cu.import("resource://gre/modules/Promise.jsm", Imports);
let {Promise, Task} = Imports;

add_task(function cleanup() {
  while (ss.getClosedTabCount(window)) {
    ss.forgetClosedTab(window, 0);
  }
});

add_task(function() {
  let URL_PUBLIC = "http://example.com/public/" + Math.random();
  let URL_PRIVATE = "http://example.com/private/" + Math.random();
  let tab1, tab2;
  try {
    
    info("Setting up public tab");
    tab1 = gBrowser.addTab(URL_PUBLIC);
    yield promiseBrowserLoaded(tab1.linkedBrowser);

    info("Setting up private tab");
    tab2 = gBrowser.addTab();
    yield promiseBrowserLoaded(tab2.linkedBrowser);
    setUsePrivateBrowsing(tab2.linkedBrowser, true);
    tab2.linkedBrowser.loadURI(URL_PRIVATE);
    yield promiseBrowserLoaded(tab2.linkedBrowser);

    info("Flush to make sure chrome received all data.");
    SyncHandlers.get(tab2.linkedBrowser).flush();

    info("Checking out state");
    yield forceSaveState();
    let state = new TextDecoder().decode((yield OS.File.read(OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js"))));
    info("State: " + state);
    
    ok(state.indexOf(URL_PUBLIC) != -1, "State contains public tab");
    ok(state.indexOf(URL_PRIVATE) == -1, "State does not contain private tab");

    
    gBrowser.removeTab(tab2);
    tab2 = null;

    gBrowser.removeTab(tab1);
    tab1 = null;

    tab1 = ss.undoCloseTab(window, 0);
    ok(true, "Public tab supports undo close");

    is(ss.getClosedTabCount(window), 0, "Private tab does not support undo close");

  } finally {
    if (tab1) {
      gBrowser.removeTab(tab1);
    }
    if (tab2) {
      gBrowser.removeTab(tab2);
    }
  }
});

function setUsePrivateBrowsing(browser, val) {
  return sendMessage(browser, "ss-test:setUsePrivateBrowsing", val);
}
