






function run_test()
{
  Cu.import("resource://gre/modules/jsdebugger.jsm");
  addDebuggerToGlobal(this);
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
    .getService(Components.interfaces.mozIJSSubScriptLoader);
  loader.loadSubScript("resource://gre/modules/devtools/server/actors/script.js");

  let instance1 = new ThreadActor();
  let instance2 = new ThreadActor();
  do_check_eq(instance1._breakpointStore, ThreadActor._breakpointStore);
  do_check_eq(instance2._breakpointStore, ThreadActor._breakpointStore);
  do_check_eq(instance1._breakpointStore, instance2._breakpointStore);
}
