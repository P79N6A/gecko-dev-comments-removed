











































function run_test() {
  PRIVATEBROWSING_CONTRACT_ID = "@mozilla.org/privatebrowsing;1";
  do_import_script("browser/components/privatebrowsing/test/unit/do_test_removeDataFromDomain_activeDownloads.js");
  do_test();
}
