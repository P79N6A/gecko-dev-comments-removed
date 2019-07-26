








function run_test() {
  do_get_profile();
  let startup = Cc["@mozilla.org/browser/sessionstartup;1"].
    getService(Ci.nsISessionStartup);
  do_check_eq(startup.sessionType, Ci.nsISessionStartup.NO_SESSION);
}