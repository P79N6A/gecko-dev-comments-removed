



function run_test() {
  let serv = Cc["@mozilla.org/content-pref/service;1"].
             getService(Ci.nsIContentPrefService2);
  do_check_eq(serv.QueryInterface(Ci.nsIContentPrefService2), serv);
  do_check_eq(serv.QueryInterface(Ci.nsISupports), serv);
  let val = serv.QueryInterface(Ci.nsIContentPrefService);
  do_check_true(val instanceof Ci.nsIContentPrefService);
}
