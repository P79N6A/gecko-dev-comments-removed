







let gTab, gDebuggee, gPanel, gDebugger;
let gOldListener;

const TAB_URL = EXAMPLE_URL + "doc_inline-script.html";

function test() {
  installListener();

  initDebugger(TAB_URL).then(([aTab, aDebuggee, aPanel]) => {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;

    is(!!gDebugger.DebuggerController._startup, true,
      "Controller should be initialized after starting the test.");
    is(!!gDebugger.DebuggerView._startup, true,
      "View should be initialized after starting the test.");

    testPause();
  });
}

function testPause() {
  waitForSourceAndCaretAndScopes(gPanel, ".html", 16).then(() => {
    is(gDebugger.gThreadClient.state, "paused",
      "The debugger statement was reached.");

    resumeDebuggerThenCloseAndFinish(gPanel);
  });

  gDebuggee.runDebuggerStatement();
}


function installListener() {
  if ("_testPL" in window) {
    gOldListener = _testPL;

    Cc['@mozilla.org/docloaderservice;1']
      .getService(Ci.nsIWebProgress)
      .removeProgressListener(_testPL);
  }

  window._testPL = {
    START_DOC: Ci.nsIWebProgressListener.STATE_START |
               Ci.nsIWebProgressListener.STATE_IS_DOCUMENT,
    onStateChange: function(wp, req, stateFlags, status) {
      if ((stateFlags & this.START_DOC) === this.START_DOC) {
        
        wp.DOMWindow;
      }
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsISupportsWeakReference) ||
          iid.equals(Ci.nsIWebProgressListener))
        return this;
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  }

  Cc['@mozilla.org/docloaderservice;1']
    .getService(Ci.nsIWebProgress)
    .addProgressListener(_testPL, Ci.nsIWebProgress.NOTIFY_STATE_REQUEST);
}

registerCleanupFunction(function() {
  if (gOldListener) {
    window._testPL = gOldListener;
  } else {
    delete window._testPL;
  }

  gTab = null;
  gDebuggee = null;
  gPanel = null;
  gDebugger = null;
  gOldListener = null;
});
