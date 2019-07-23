





































function run_test_on_service()
{
  let Cc = Components.classes;
  let Ci = Components.interfaces;
  let Cu = Components.utils;
  Cu.import("resource://gre/modules/openLocationLastURL.jsm");

  let pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
           getService(Ci.nsIPrivateBrowsingService);
  let pref = Cc["@mozilla.org/preferences-service;1"].
             getService(Ci.nsIPrefBranch);
  gOpenLocationLastURL.reset();

  do_check_eq(typeof gOpenLocationLastURL, "object");
  do_check_eq(gOpenLocationLastURL.value, "");

  const url1 = "mozilla.org";
  const url2 = "mozilla.com";

  gOpenLocationLastURL.value = url1;
  do_check_eq(gOpenLocationLastURL.value, url1);

  gOpenLocationLastURL.value = "";
  do_check_eq(gOpenLocationLastURL.value, "");

  gOpenLocationLastURL.value = url2;
  do_check_eq(gOpenLocationLastURL.value, url2);

  pb.privateBrowsingEnabled = true;
  do_check_eq(gOpenLocationLastURL.value, "");

  pb.privateBrowsingEnabled = false;
  do_check_eq(gOpenLocationLastURL.value, url2);
  pb.privateBrowsingEnabled = true;

  gOpenLocationLastURL.value = url1;
  do_check_eq(gOpenLocationLastURL.value, url1);

  pb.privateBrowsingEnabled = false;
  do_check_eq(gOpenLocationLastURL.value, url2);
}


function run_test() {
  run_test_on_all_services();
}
