







function test() {
  const EXCEPTIONS_DLG_URL = 'chrome://pippki/content/exceptionDialog.xul';
  const EXCEPTIONS_DLG_FEATURES = 'chrome,centerscreen';
  const INVALID_CERT_LOCATION = 'https://nocert.example.com/';
  waitForExplicitFinish();

  
  var pbWin = OpenBrowserWindow({private: true});
  pbWin.addEventListener("load", function onLoad() {
    pbWin.removeEventListener("load", onLoad, false);
    doTest();
  }, false);

  
  function doTest() {
    let params = {
      exceptionAdded : false,
      location: INVALID_CERT_LOCATION,
      prefetchCert: true,
    };
    function testCheckbox() {
      win.removeEventListener("load", testCheckbox, false);
      Services.obs.addObserver(function (aSubject, aTopic, aData) {
        Services.obs.removeObserver(arguments.callee, "cert-exception-ui-ready", false);
        ok(win.gCert, "The certificate information should be available now");

        let checkbox = win.document.getElementById("permanent");
        ok(checkbox.hasAttribute("disabled"),
          "the permanent checkbox should be disabled when handling the private browsing mode");
        ok(!checkbox.hasAttribute("checked"),
          "the permanent checkbox should not be checked when handling the private browsing mode");
        win.close();
        cleanup();
      }, "cert-exception-ui-ready", false);
    }
    var win = pbWin.openDialog(EXCEPTIONS_DLG_URL, "", EXCEPTIONS_DLG_FEATURES, params);
    win.addEventListener("load", testCheckbox, false);
  }

  function cleanup() {
    
    pbWin.close();
    finish();
  }
}
