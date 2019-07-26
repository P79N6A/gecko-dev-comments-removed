






function run_test() {
  const tern = require("tern/tern");
  const ecma5 = require("tern/ecma5");
  const browser = require("tern/browser");
  do_check_true(!!tern);
  do_check_true(!!ecma5);
  do_check_true(!!browser);
  do_check_eq(typeof tern.Server, "function");
}
