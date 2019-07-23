










































const ipcIService = Components.interfaces.ipcIService;

function registerServer()
{
  var ipc = Components.classes["@mozilla.org/ipc/service;1"].getService(ipcIService); 
  ipc.addName("test-server");
}

function runEventQ()
{
  var thread =
      Components.classes["@mozilla.org/thread-manager;1"].
      getService().currentThread;

  
  while (true)
    thread.processNextEvent();
}

registerServer();
runEventQ();
