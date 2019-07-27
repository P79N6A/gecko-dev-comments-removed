



var Ci = Components.interfaces;
var Cc = Components.classes;
var Cu = Components.utils;

function Quitter() {
}

Quitter.prototype = {
  toString: function() { return "[Quitter]"; },
  quit: function() { sendSyncMessage('Quitter.Quit', {}); }
};






function QuitterManager() {
  addEventListener("DOMWindowCreated", this, false);
}

QuitterManager.prototype = {
  handleEvent: function handleEvent(aEvent) {
    var quitter = new Quitter(window);
    var window = aEvent.target.defaultView;
    window.wrappedJSObject.Quitter = Cu.cloneInto({
      toString: quitter.toString.bind(quitter),
      quit: quitter.quit.bind(quitter)
    }, window, {cloneFunctions: true});
  }
};

var quittermanager = new QuitterManager();
