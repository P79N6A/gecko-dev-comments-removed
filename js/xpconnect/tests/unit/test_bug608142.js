





































function run_test() { 
    var tm = Components.classes["@mozilla.org/thread-manager;1"].getService();
    var thr = tm.newThread(0);

    var foundThreadError = false;
 
    var listener = {
        observe: function(message) {
            if (/JS function on a different thread/.test(message.message))
                foundThreadError = true;
        }
    };

    var cs = Components.classes["@mozilla.org/consoleservice;1"].
        getService(Components.interfaces.nsIConsoleService);
    cs.registerListener(listener);

    thr.dispatch({
        run: function() {
            do_check_true(false);
        }
    }, Components.interfaces.nsIThread.DISPATCH_NORMAL);

    thr.shutdown();

    cs.unregisterListener(listener);
    do_check_true(foundThreadError);
}

