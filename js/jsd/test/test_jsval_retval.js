








function run_test() {
    const Cc = Components.classes;
    const Ci = Components.interfaces;
    const DebuggerService = Cc["@mozilla.org/js/jsd/debugger-service;1"];
    const jsdIDebuggerService = Ci.jsdIDebuggerService;
    var jsd = DebuggerService.getService(jsdIDebuggerService);

    do_check_true(jsd.isOn);

    var n = 0;
    function f() {
        n++;
    }

    jsd.enumerateScripts({ enumerateScript: function(script) {
        script.setBreakpoint(0);
    } });

    jsd.breakpointHook = function(frame, type, dummy) {
        var scope = frame.scope;
        var parent = scope.jsParent; 
        var wrapped = scope.getWrappedValue();
        
        
        do_check_eq(typeof(wrapped), "object");
        return Ci.jsdIExecutionHook.RETURN_CONTINUE;
    };

    f();
}
