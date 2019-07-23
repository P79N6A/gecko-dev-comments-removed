
importScripts();

onmessage = function(event) {
  switch (event.data) {
    case 'start':
      importScripts("importScripts_worker_imported2.js");
      importedScriptFunction2();
      tryBadScripts();
      postMessage('started');
      break;
    case 'stop':
      tryBadScripts();
      postMessage('stopped');
      break;
    default:
      throw new Error("Bad message: " + event.data);
      break;
  }
}


var constructor = {}.constructor;

importScripts("importScripts_worker_imported1.js");


importedScriptFunction();

function tryBadScripts() {
  var badScripts = [
    
    "importScripts_worker_imported3.js",
    
    "importScripts_worker_imported4.js",
    
    "http://flippety.com/floppety/foo.js",
    
    "http://flippety::foo_js ftw"
  ];

  for (var i = 0; i < badScripts.length; i++) {
    var caughtException = false;
    var url = badScripts[i];
    try {
      importScripts(url);
    }
    catch (e) {
      caughtException = true;
    }
    if (!caughtException) {
      throw "Bad script didn't throw exception: " + url;
    }
  }
}

tryBadScripts();
