






































function run_test() {
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  startupManager();

  installAllFiles([do_get_file("data/test_bug541420.xpi")], function() {

    restartManager();

    AddonManager.getAddonByID("bug541420@tests.mozilla.org", function(addon) {

      do_check_neq(addon, null);
      do_check_true(addon.hasResource("binary"));
      let uri = Services.io.newURI(addon.getResourceURL("binary"), "UTF-8", null);
      do_check_true(uri instanceof AM_Ci.nsIFileURL);
      let file = uri.file;
      do_check_true(file.exists());
      do_check_true(file.isReadable());
      do_check_true(file.isWritable());

      
      
      
      
      if (!("nsIWindowsRegKey" in Components.interfaces))
        do_check_true((file.permissions & 0100) == 0100);

      do_test_finished();
    });
  });
}
