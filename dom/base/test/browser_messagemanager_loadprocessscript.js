let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
           .getService(Ci.nsIMessageBroadcaster);
ppmm.QueryInterface(Ci.nsIProcessScriptLoader);

function processScript() {
  let cpmm = Components.classes["@mozilla.org/childprocessmessagemanager;1"]
           .getService(Components.interfaces.nsISyncMessageSender);
  if (cpmm !== this) {
    dump("Test failed: wrong global object\n");
    return;
  }

  this.cpmm = cpmm;

  addMessageListener("ProcessTest:Reply", function listener(msg) {
    removeMessageListener("ProcessTest:Reply", listener);
    sendAsyncMessage("ProcessTest:Finished");
  });
  sendSyncMessage("ProcessTest:Loaded");
}
let processScriptURL = "data:,(" + processScript.toString() + ")()";

let checkProcess = Task.async(function*(mm) {
  let { target } = yield promiseMessage(mm, "ProcessTest:Loaded");
  target.sendAsyncMessage("ProcessTest:Reply");
  yield promiseMessage(target, "ProcessTest:Finished");
  ok(true, "Saw process finished");
});

function promiseMessage(messageManager, message) {
  return new Promise(resolve => {
    let listener = (msg) => {
      messageManager.removeMessageListener(message, listener);
      resolve(msg);
    };

    messageManager.addMessageListener(message, listener);
  })
}


add_task(function*() {
  let checks = [];
  for (let i = 0; i < ppmm.childCount; i++)
    checks.push(checkProcess(ppmm.getChildAt(i)));

  ppmm.loadProcessScript(processScriptURL, false);
  yield Promise.all(checks);
});


add_task(function*() {
  
  if (!gMultiProcessBrowser)
    return;

  is(ppmm.childCount, 2, "Should be two processes at this point");

  
  gBrowser.selectedBrowser.loadURI("about:robots");
  yield BrowserTestUtils.browserLoaded(gBrowser.selectedBrowser);

  
  
  
  
  if (ppmm.childCount == 1) {
    let check = checkProcess(ppmm);
    ppmm.loadProcessScript(processScriptURL, true);

    
    yield check;

    check = checkProcess(ppmm);
    
    gBrowser.updateBrowserRemoteness(gBrowser.selectedBrowser, true);
    gBrowser.selectedBrowser.loadURI("about:blank");
    yield BrowserTestUtils.browserLoaded(gBrowser.selectedBrowser);

    is(ppmm.childCount, 2, "Should be back to two processes at this point");

    
    yield check;

    ppmm.removeDelayedProcessScript(processScriptURL);
  } else {
    info("Unable to finish test entirely");
  }
});
