






function run_test() {
  const escodegen = require("escodegen/escodegen");
  do_check_eq(typeof escodegen.generate, "function");
}
