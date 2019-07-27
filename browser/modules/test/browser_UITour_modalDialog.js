"use strict";

let gTestTab;
let gContentAPI;
let gContentWindow;
let handleDialog;


var didDialog;

var timer; 
function startCallbackTimer() {
    didDialog = false;

    
    const dialogDelay = 10;

    
    timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.init(observer, dialogDelay, Ci.nsITimer.TYPE_ONE_SHOT);
}


var observer = SpecialPowers.wrapCallbackObject({
    QueryInterface : function (iid) {
        const interfaces = [Ci.nsIObserver,
                            Ci.nsISupports, Ci.nsISupportsWeakReference];

        if (!interfaces.some( function(v) { return iid.equals(v) } ))
            throw SpecialPowers.Components.results.NS_ERROR_NO_INTERFACE;
        return this;
    },

    observe : function (subject, topic, data) {
        var doc = getDialogDoc();
        if (doc)
            handleDialog(doc);
        else
            startCallbackTimer(); 
    }
});

function getDialogDoc() {
  
  
  var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  
  var enumerator = wm.getXULWindowEnumerator(null);

  while (enumerator.hasMoreElements()) {
    var win = enumerator.getNext();
    var windowDocShell = win.QueryInterface(Ci.nsIXULWindow).docShell;

    var containedDocShells = windowDocShell.getDocShellEnumerator(
                                      Ci.nsIDocShellTreeItem.typeChrome,
                                      Ci.nsIDocShell.ENUMERATE_FORWARDS);
    while (containedDocShells.hasMoreElements()) {
        
        var childDocShell = containedDocShells.getNext();
        
        if (childDocShell.busyFlags != Ci.nsIDocShell.BUSY_FLAGS_NONE)
          continue;
        var childDoc = childDocShell.QueryInterface(Ci.nsIDocShell)
                                    .contentViewer
                                    .DOMDocument;

        
        if (childDoc.location.href == "chrome://global/content/commonDialog.xul")
          return childDoc;
    }
  }

  return null;
}

Components.utils.import("resource:///modules/UITour.jsm");

function test() {
  UITourTest();
}


let tests = [
  taskify(function* test_modal_dialog_while_opening_tooltip(done) {
    let panelShown;
    let popup;

    handleDialog = (doc) => {
      popup = document.getElementById("UITourTooltip");
      gContentAPI.showInfo("appMenu", "test title", "test text");
      doc.defaultView.setTimeout(function() {
        is(popup.state, "closed", "Popup shouldn't be shown while dialog is up");
        panelShown = promisePanelElementShown(window, popup);
        let dialog = doc.getElementById("commonDialog");
        dialog.acceptDialog();
      }, 1000);
    };
    startCallbackTimer();
    executeSoon(() => alert("test"));
    yield waitForConditionPromise(() => panelShown, "Timed out waiting for panel promise to be assigned", 100);
    yield panelShown;

    yield hideInfoPromise();
  })
];
