
















































var frame = {}; Components.utils.import('resource://mozmill/modules/frame.js', frame);

const MODULE_NAME = 'ModalDialogAPI';




var mdObserver = {
  QueryInterface : function (iid) {
    const interfaces = [Ci.nsIObserver,
                        Ci.nsISupports,
                        Ci.nsISupportsWeakReference];

    if (!interfaces.some( function(v) { return iid.equals(v) } ))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  },

  observe : function (subject, topic, data)
  {
    if (this.docFinder()) {
      try {
        var window = mozmill.wm.getMostRecentWindow("");
        this.handler(new mozmill.controller.MozMillController(window));
      } catch (ex) {
          window.close();
          frame.events.fail({'function':ex});
      }
    } else {
      
      this.startTimer(null, this);
    }
  },

  handler: null,
  startTimer: null,
  docFinder: null
};









function modalDialog(callback)
{
  this.observer = mdObserver;
  this.observer.handler = callback;
  this.observer.startTimer = this.start;
  this.observer.docFinder = this.getDialog;
}







modalDialog.prototype.setHandler = function modalDialog_setHandler(callback)
{
  this.observer.handler = callback;
}









modalDialog.prototype.start = function modalDialog_start(delay, observer)
{
  const dialogDelay = (delay == undefined) ? 100 : delay;

  var modalDialogTimer = Cc["@mozilla.org/timer;1"].
                         createInstance(Ci.nsITimer);

  
  
  if (observer) {
    modalDialogTimer.init(observer,
                          dialogDelay,
                          Ci.nsITimer.TYPE_ONE_SHOT);
  } else {
    modalDialogTimer.init(this.observer,
                          dialogDelay,
                          Ci.nsITimer.TYPE_ONE_SHOT);
  }
}








modalDialog.prototype.getDialog = function modalDialog_getDialog()
{
  var enumerator = mozmill.wm.getXULWindowEnumerator("");

  
  
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

      
      
      var chrome = win.QueryInterface(Ci.nsIInterfaceRequestor).
                       getInterface(Ci.nsIWebBrowserChrome);
      if (chrome.isWindowModal()) {
        return true;
      }
    }
  }

  return false;
}
