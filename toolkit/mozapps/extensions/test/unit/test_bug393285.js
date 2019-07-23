





































function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  
  var source = do_get_file("data/test_bug393285.xml");
  source.copyTo(gProfD, "blocklist.xml");

  var blocklist = Components.classes["@mozilla.org/extensions/blocklist;1"]
                            .getService(Components.interfaces.nsIBlocklistService);
  
  
  do_check_false(blocklist.isAddonBlocklisted("test_bug393285_1@tests.mozilla.org", "1", "1", "1.9"));

  
  do_check_true(blocklist.isAddonBlocklisted("test_bug393285_2@tests.mozilla.org", "1", "1", "1.9"));

  
  do_check_true(blocklist.isAddonBlocklisted("test_bug393285_3@tests.mozilla.org", "1", "1", "1.9"));
  do_check_false(blocklist.isAddonBlocklisted("test_bug393285_3@tests.mozilla.org", "2", "1", "1.9"));

  
  do_check_true(blocklist.isAddonBlocklisted("test_bug393285_4@tests.mozilla.org", "1", "1", "1.9"));
  do_check_false(blocklist.isAddonBlocklisted("test_bug393285_4@tests.mozilla.org", "1", "2", "1.9"));

  
  do_check_false(blocklist.isAddonBlocklisted("test_bug393285_5@tests.mozilla.org", "1", "2", "1.9"));

  
  do_check_true(blocklist.isAddonBlocklisted("test_bug393285_6@tests.mozilla.org", "1", "2", "1.9"));
  do_check_true(blocklist.isAddonBlocklisted("test_bug393285_7@tests.mozilla.org", "1", "2", "1.9"));

  
  do_check_false(blocklist.isAddonBlocklisted("test_bug393285_8@tests.mozilla.org", "1", "2", "1.9"));

  
  do_check_true(blocklist.isAddonBlocklisted("test_bug393285_9@tests.mozilla.org", "1", "2", "1.9"));
  do_check_true(blocklist.isAddonBlocklisted("test_bug393285_10@tests.mozilla.org", "1", "2", "1.9"));

  
  do_check_false(blocklist.isAddonBlocklisted("test_bug393285_11@tests.mozilla.org", "1", "2", "1.9"));
  do_check_false(blocklist.isAddonBlocklisted("test_bug393285_12@tests.mozilla.org", "1", "2", "1.9"));
  do_check_false(blocklist.isAddonBlocklisted("test_bug393285_13@tests.mozilla.org", "1", "2", "1.9"));

  
  do_check_true(blocklist.isAddonBlocklisted("test_bug393285_14@tests.mozilla.org", "1", "2", "1.9"));
}
