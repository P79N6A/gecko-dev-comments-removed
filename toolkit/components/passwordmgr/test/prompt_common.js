netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

const Ci = SpecialPowers.wrap(Components).interfaces;
ok(Ci != null, "Access Ci");
const Cc = SpecialPowers.wrap(Components).classes;
ok(Cc != null, "Access Cc");

var didDialog;

var timer; 
function startCallbackTimer() {
    didDialog = false;

    
    const dialogDelay = 10;

    
    timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.init(observer, dialogDelay, Ci.nsITimer.TYPE_ONE_SHOT);
}


var observer = {
    QueryInterface : function (iid) {
        const interfaces = [Ci.nsIObserver,
                            Ci.nsISupports, Ci.nsISupportsWeakReference];

        if (!interfaces.some( function(v) { return iid.equals(v) } ))
            throw Components.results.NS_ERROR_NO_INTERFACE;
        return this;
    },

    observe : function (subject, topic, data) {
        netscape.security.PrivilegeManager
                         .enablePrivilege('UniversalXPConnect');

        var doc = getDialogDoc();
        if (doc)
            handleDialog(doc, testNum);
        else
            startCallbackTimer(); 
    }
};

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
