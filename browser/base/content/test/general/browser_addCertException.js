










function test() {
  waitForExplicitFinish();
  whenNewTabLoaded(window, loadBadCertPage);
}


function loadBadCertPage() {
  gBrowser.addProgressListener(certErrorProgressListener);
  gBrowser.selectedBrowser.loadURI("https://expired.example.com");
}



let certErrorProgressListener = {
  buttonClicked: false,

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
      let self = this;
      
      executeSoon(function() {
        let button = content.document.getElementById("exceptionDialogButton");
        
        
        if (button && !self.buttonClicked) {
          gBrowser.removeProgressListener(self);
          Services.obs.addObserver(certExceptionDialogObserver,
                                   "cert-exception-ui-ready", false);
          button.click();
        }
      });
    }
  }
};



const EXCEPTION_DIALOG_URI = "chrome://pippki/content/exceptionDialog.xul";
let certExceptionDialogObserver = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "cert-exception-ui-ready") {
      Services.obs.removeObserver(this, "cert-exception-ui-ready");
      let certExceptionDialog = getDialog(EXCEPTION_DIALOG_URI);
      ok(certExceptionDialog, "found exception dialog");
      executeSoon(function() {
        gBrowser.selectedBrowser.addEventListener("load",
                                                  successfulLoadListener,
                                                  true);
        certExceptionDialog.documentElement.getButton("extra1").click();
      });
    }
  }
};


let successfulLoadListener = {
  handleEvent: function() {
    gBrowser.selectedBrowser.removeEventListener("load", this, true);
    let certOverrideService = Cc["@mozilla.org/security/certoverride;1"]
                                .getService(Ci.nsICertOverrideService);
    certOverrideService.clearValidityOverride("expired.example.com", -1);
    gBrowser.removeTab(gBrowser.selectedTab);
    finish();
  }
};



function getDialog(aLocation) {
  let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
             .getService(Ci.nsIWindowMediator);
  let enumerator = wm.getXULWindowEnumerator(null);

  while (enumerator.hasMoreElements()) {
    let win = enumerator.getNext();
    let windowDocShell = win.QueryInterface(Ci.nsIXULWindow).docShell;

    let containedDocShells = windowDocShell.getDocShellEnumerator(
                                      Ci.nsIDocShellTreeItem.typeChrome,
                                      Ci.nsIDocShell.ENUMERATE_FORWARDS);
    while (containedDocShells.hasMoreElements()) {
      
      let childDocShell = containedDocShells.getNext();
      let childDoc = childDocShell.QueryInterface(Ci.nsIDocShell)
                                  .contentViewer
                                  .DOMDocument;

      if (childDoc.location.href == aLocation) {
        return childDoc;
      }
    }
  }
}
