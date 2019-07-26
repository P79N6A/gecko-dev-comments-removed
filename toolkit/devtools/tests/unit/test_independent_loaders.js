


const { DevToolsLoader } =
  Cu.import("resource://gre/modules/devtools/Loader.jsm", {});






function run_test() {
  let loader1 = new DevToolsLoader();
  let loader2 = new DevToolsLoader();

  let server1 = loader1.require("devtools/server/main");
  let server2 = loader2.require("devtools/server/main");

  do_check_true(server1 !== server2);

  do_check_true(loader1._provider !== loader2._provider);
  do_check_true(loader1._provider.loader !== loader2._provider.loader);
}
