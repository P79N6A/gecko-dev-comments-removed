



var Ci = Components.interfaces;
var Cc = Components.classes;

function Quitter() {
}

Quitter.prototype = {
  toString: function() { return "[Quitter]"; },
  quit: function() {
    sendSyncMessage('Quitter.Quit', {});
  },
  __exposedProps__: {
    'toString': 'r',
    'quit': 'r'
  }
};






function QuitterManager() {
  addEventListener("DOMWindowCreated", this, false);
}

QuitterManager.prototype = {
  handleEvent: function handleEvent(aEvent) {
    var window = aEvent.target.defaultView;
    window.wrappedJSObject.Quitter = new Quitter(window);
  }
};

var quittermanager = new QuitterManager();
