Cu.import("resource://services-sync/util.js");

function run_test() {
  let str;

  
  
 
  str = Utils.getErrorString("error.login.reason.account");
  do_check_true(str.match(/unknown/i) == null);

  str = Utils.getErrorString("foobar");
  do_check_true(str.match(/unknown/i) != null);
}
