








function run_test() {
  let profd = do_get_profile();
  let source = do_get_file("data/sessionstore_valid.js");
  source.copyTo(profd, "sessionstore.js");

  do_test_pending();
  let startup = Cc["@mozilla.org/browser/sessionstartup;1"].
    getService(Ci.nsISessionStartup);

  afterSessionStartupInitialization(function cb() {
    do_check_eq(startup.sessionType, Ci.nsISessionStartup.DEFER_SESSION);
    do_test_finished();
  });
}