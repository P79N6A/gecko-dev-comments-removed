


Cu.import("resource://services-sync/util.js");

function run_test() {
  do_check_eq(Utils.encodeKeyBase32("foobarbafoobarba"), "mzxw6ytb9jrgcztpn5rgc4tcme");
  do_check_eq(Utils.decodeKeyBase32("mzxw6ytb9jrgcztpn5rgc4tcme"), "foobarbafoobarba");
  do_check_eq(
      Utils.encodeKeyBase32("\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"),
      "aeaqcaibaeaqcaibaeaqcaibae");
  do_check_eq(
      Utils.decodeKeyBase32("aeaqcaibaeaqcaibaeaqcaibae"),
      "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01");
}
