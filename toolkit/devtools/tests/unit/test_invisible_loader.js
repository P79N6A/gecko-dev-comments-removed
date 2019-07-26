


const { DevToolsLoader } =
  Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
Cu.import("resource://gre/modules/jsdebugger.jsm");
addDebuggerToGlobal(this);

const MAIN_URI = "resource://gre/modules/devtools/server/main.js";





function run_test() {
  visible_loader();
  invisible_loader();
}

function visible_loader() {
  let loader = new DevToolsLoader();
  loader.invisibleToDebugger = false;
  loader.require("devtools/server/main");

  let dbg = new Debugger();
  let sandbox = loader._provider.loader.sandboxes[MAIN_URI];

  try {
    dbg.addDebuggee(sandbox);
    do_check_true(true);
  } catch(e) {
    do_throw("debugger could not add visible value");
  }
}

function invisible_loader() {
  let loader = new DevToolsLoader();
  loader.invisibleToDebugger = true;
  loader.require("devtools/server/main");

  let dbg = new Debugger();
  let sandbox = loader._provider.loader.sandboxes[MAIN_URI];

  try {
    dbg.addDebuggee(sandbox);
    do_throw("debugger added invisible value");
  } catch(e) {
    do_check_true(true);
  }
}
