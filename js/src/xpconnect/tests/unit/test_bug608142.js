





































function run_test() { 
    var tm = Components.classes["@mozilla.org/thread-manager;1"].getService();
    var thr = tm.newThread(0);

    thr.dispatch({
        run: function() {
            do_check_true(false);
        }
    }, Components.interfaces.nsIThread.DISPATCH_NORMAL);

    thr.shutdown();
}

