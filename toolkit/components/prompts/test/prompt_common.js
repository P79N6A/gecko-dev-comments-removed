const Ci = SpecialPowers.Ci;
const Cc = SpecialPowers.Cc;
ok(Ci != null, "Access Ci");
ok(Cc != null, "Access Cc");

var didDialog;

var isSelectDialog = false;
var isTabModal = false;
var usePromptService = true;

var timer; 
function startCallbackTimer() {
    didDialog = false;

    
    const dialogDelay = 10;

    
    timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.init(observer, dialogDelay, Ci.nsITimer.TYPE_ONE_SHOT);
}


var observer = {
    QueryInterface : SpecialPowers.wrapCallback(function (iid) {
        const interfaces = [Ci.nsIObserver,
                            Ci.nsISupports, Ci.nsISupportsWeakReference];

        if (!interfaces.some( function(v) { return iid.equals(v) } ))
            throw SpecialPowers.Cr.NS_ERROR_NO_INTERFACE;
        return this;
    }),

    observe : SpecialPowers.wrapCallback(function (subject, topic, data) {
      try {
        if (isTabModal) {
          var promptBox = getTabModalPromptBox(window);
          ok(promptBox, "got tabmodal promptbox");
          var prompts = SpecialPowers.wrap(promptBox).listPrompts();
          if (prompts.length)
              handleDialog(prompts[0].Dialog.ui, testNum);
          else
              startCallbackTimer(); 
        } else {
          var doc = getDialogDoc();
          if (isSelectDialog && doc)
              handleDialog(doc, testNum);
          else if (doc)
              handleDialog(doc.defaultView.Dialog.ui, testNum);
          else
              startCallbackTimer(); 
        }
      } catch (e) {
        ok(false, "Exception thrown in the timer callback: " + e + " at " + (e.fileName || e.filename) + ":" + (e.lineNumber || e.linenumber));
      }
    })
};

function getTabModalPromptBox(domWin) {
    var promptBox = null;

    
    function getChromeWindow(aWindow) {
        var chromeWin = SpecialPowers.wrap(aWindow).QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIWebNavigation)
                               .QueryInterface(Ci.nsIDocShell)
                               .chromeEventHandler.ownerDocument.defaultView;
        return chromeWin;
    }

    try {
        
        var promptWin = domWin.top;

        
        var chromeWin = getChromeWindow(promptWin);

        if (chromeWin.getTabModalPromptBox)
            promptBox = chromeWin.getTabModalPromptBox(promptWin);
    } catch (e) {
        
    }

    
    return SpecialPowers.unwrap(promptBox);
}

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
        if (childDoc.location.href == "chrome://global/content/selectDialog.xul")
          return childDoc;
    }
  }

  return null;
}
