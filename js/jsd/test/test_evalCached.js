

function run_test() {
    const Cc = Components.classes;
    const Ci = Components.interfaces;
    const DebuggerService = Cc["@mozilla.org/js/jsd/debugger-service;1"];
    const jsdIDebuggerService = Ci.jsdIDebuggerService;
    var jsd = DebuggerService.getService(jsdIDebuggerService);

    do_check_true(jsd.isOn);

    jsd.scriptHook = {
        onScriptCreated: function(script) {
            
            
        },
        onScriptDestroyed: function(script) {
        }
    }

    eval("4+4");
    eval("4+4"); 
}
