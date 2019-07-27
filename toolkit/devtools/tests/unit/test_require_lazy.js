





function run_test() {
  const name = "asyncUtils";
  const path = "devtools/async-utils";
  const o = {};
  devtools.lazyRequireGetter(o, name, path);
  const asyncUtils = devtools.require(path);
  
  
  do_check_true(o.asyncUtils === asyncUtils);

  
  
  const o2 = {};
  let loader = new DevToolsLoader();
  loader.lazyRequireGetter(o2, name, path);
  do_check_true(o2.asyncUtils !== asyncUtils);

  
  
  let exposeLoader = loader.require("xpcshell-test/exposeLoader");
  const o3 = exposeLoader.exerciseLazyRequire(name, path);
  do_check_true(o3.asyncUtils === o2.asyncUtils);
}
