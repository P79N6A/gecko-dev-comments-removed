function test() {
  

  let threadman = Components.classes["@mozilla.org/thread-manager;1"].
                             getService(Ci.nsIThreadManager);
  let thread = threadman.newThread(0);

  thread.dispatch(
    function() {
      
      
      let threadman2 = Components.classes["@mozilla.org/thread-manager;1"].
                                  getService(Ci.nsIThreadManager);

      
      
      if (threadman2.mainThread == threadman2.currentThread)
        ok(false, "Shouldn't be on the main thread.");

    }, 1); 

  thread.shutdown();
  ok(true, "Done!");
}
