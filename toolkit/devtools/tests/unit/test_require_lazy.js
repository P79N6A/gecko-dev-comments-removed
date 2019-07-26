





function run_test() {
  const o = {};
  devtools.lazyRequireGetter(o, "asyncUtils", "devtools/async-utils");
  const asyncUtils = devtools.require("devtools/async-utils");
  
  
  do_check_true(o.asyncUtils === asyncUtils);
}
