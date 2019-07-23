








































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);

  const EXCEPTIONS_DLG_URL = 'chrome://pippki/content/exceptionDialog.xul';
  const EXCEPTIONS_DLG_FEATURES = 'chrome,centerscreen,modal';
  const INVALID_CERT_LOCATION = 'https://nocert.example.com/';
  waitForExplicitFinish();

  
  pb.privateBrowsingEnabled = true;

  let testCheckbox;
  let obs = {
      observe: function(aSubject, aTopic, aData) {
          
          ww.unregisterNotification(this);

          let win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);
          win.addEventListener("load", function() {
              win.removeEventListener("load", arguments.callee, false);
              testCheckbox(win.document.defaultView);
          }, false);
      }
  };

  step1();

  
  function step1() {
    ww.registerNotification(obs);
    let params = {
      exceptionAdded : false,
      location: INVALID_CERT_LOCATION,
      handlePrivateBrowsing : true,
      prefetchCert: true,
    };
    testCheckbox = function(win) {
      let obsSvc = Cc["@mozilla.org/observer-service;1"].
                   getService(Ci.nsIObserverService);
      obsSvc.addObserver({
        observe: function(aSubject, aTopic, aData) {
          obsSvc.removeObserver(this, "cert-exception-ui-ready", false);
          ok(win.gCert, "The certificate information should be available now");

          let checkbox = win.document.getElementById("permanent");
          ok(checkbox.hasAttribute("disabled"),
            "the permanent checkbox should be disabled when handling the private browsing mode");
          ok(!checkbox.hasAttribute("checked"),
            "the permanent checkbox should not be checked when handling the private browsing mode");
          win.close();
          step2();
        }
      }, "cert-exception-ui-ready", false);
    };
    window.openDialog(EXCEPTIONS_DLG_URL, '', EXCEPTIONS_DLG_FEATURES, params);
  }

  
  function step2() {
    ww.registerNotification(obs);
    let params = {
      exceptionAdded : false,
      location: INVALID_CERT_LOCATION,
      prefetchCert: true,
    };
    testCheckbox = function(win) {
      let obsSvc = Cc["@mozilla.org/observer-service;1"].
                   getService(Ci.nsIObserverService);
      obsSvc.addObserver({
        observe: function(aSubject, aTopic, aData) {
          obsSvc.removeObserver(this, "cert-exception-ui-ready", false);
          ok(win.gCert, "The certificate information should be available now");

          let checkbox = win.document.getElementById("permanent");
          ok(!checkbox.hasAttribute("disabled"),
            "the permanent checkbox should not be disabled when not handling the private browsing mode");
          ok(checkbox.hasAttribute("checked"),
            "the permanent checkbox should be checked when not handling the private browsing mode");
          win.close();
          cleanup();
        }
      }, "cert-exception-ui-ready", false);
    };
    window.openDialog(EXCEPTIONS_DLG_URL, '', EXCEPTIONS_DLG_FEATURES, params);
  }

  function cleanup() {
    
    pb.privateBrowsingEnabled = false;
    finish();
  }
}
