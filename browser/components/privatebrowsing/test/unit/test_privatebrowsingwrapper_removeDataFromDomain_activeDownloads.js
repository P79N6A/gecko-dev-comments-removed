










Components.utils.import("resource://gre/modules/Services.jsm");

function run_test() {
  PRIVATEBROWSING_CONTRACT_ID = "@mozilla.org/privatebrowsing;1";
  load("do_test_removeDataFromDomain_activeDownloads.js");
  do_test();

  
  Services.obs.notifyObservers(null, "quit-application", null);
}
