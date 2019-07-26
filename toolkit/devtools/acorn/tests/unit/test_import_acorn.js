






function run_test() {
  const acorn = require("acorn/acorn");
  const acorn_loose = require("acorn/acorn_loose");
  do_check_true(isObject(acorn));
  do_check_true(isObject(acorn_loose));
  do_check_eq(typeof acorn.parse, "function");
  do_check_eq(typeof acorn_loose.parse_dammit, "function");
}
