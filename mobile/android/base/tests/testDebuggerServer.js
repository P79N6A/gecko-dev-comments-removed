




"use strict";

const { utils: Cu } = Components;

const DEBUGGER_REMOTE_ENABLED = "devtools.debugger.remote-enabled";

Cu.import("resource://gre/modules/Services.jsm");

add_test(function() {
  let window = Services.wm.getMostRecentWindow("navigator:browser");

  
  do_register_cleanup(function() {
    Services.prefs.clearUserPref(DEBUGGER_REMOTE_ENABLED);
  });
  Services.prefs.setBoolPref(DEBUGGER_REMOTE_ENABLED, true);

  let DebuggerServer = window.DebuggerServer;
  do_check_true(DebuggerServer.initialized);
  do_check_true(!!DebuggerServer._listener);

  run_next_test();
});

run_next_test();
