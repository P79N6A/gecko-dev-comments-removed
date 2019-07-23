





































var keygenThread;

function onLoad()
{
  keygenThread = window.arguments[0].QueryInterface(Components.interfaces.nsIKeygenThread);
  
  if (!keygenThread) {
    window.close();
    return;
  }
  
  setCursor("wait");

  var obs = {
    observe : function keygenListenerObserve(subject, topic, data) {
      if (topic == "keygen-finished")
        window.close();
    }
  };
  
  keygenThread.startKeyGeneration(obs);
}

function onClose()
{
  setCursor("default");

  var alreadyClosed = new Object();
  keygenThread.userCanceled(alreadyClosed);
}
