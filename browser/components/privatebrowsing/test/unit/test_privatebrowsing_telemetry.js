





function run_test() {
  PRIVATEBROWSING_CONTRACT_ID = "@mozilla.org/privatebrowsing;1";
  load("do_test_privatebrowsing_telemetry.js");
  do_test();
}
