function run_test() {
  set_process_running_environment();

  var file = get_test_program("TestQuickReturn");
  var thread = Components.classes["@mozilla.org/thread-manager;1"]
                         .getService().currentThread;

  for (var i = 0; i < 1000; i++) {
    var process = Components.classes["@mozilla.org/process/util;1"]
                          .createInstance(Components.interfaces.nsIProcess);
    process.init(file);

    process.run(false, [], 0);

    try {
      process.kill();
    }
    catch (e) { }

    
    
    
    while (thread.hasPendingEvents())
      thread.processNextEvent(false);
  }

}