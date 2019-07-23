





































const NO_INSTALL_SCRIPT = -204;

var listener = {
  onStateChange: function(index, state, value) {
    if (state == Components.interfaces.nsIXPIProgressDialog.INSTALL_DONE)
      do_check_eq(value, NO_INSTALL_SCRIPT);
  },

  onProgress: function(index, value, maxValue) {
  }
}

function run_test() {
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");
  startupEM();

  var xpi = do_get_addon("test_bug436207");
  var ioservice = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);
  var uri = ioservice.newFileURI(xpi);

  var xpim = Components.classes["@mozilla.org/xpinstall/install-manager;1"]
                       .createInstance(Components.interfaces.nsIXPInstallManager);
  xpim.initManagerFromChrome([uri.spec], 1, listener);
}
