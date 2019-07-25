







































function SpecialPowers(window) {
  this.window = window;
  this._encounteredCrashDumpFiles = [];
  this._unexpectedCrashDumpFiles = { };
  this._crashDumpDir = null;
  this.DOMWindowUtils = bindDOMWindowUtils(window);
  this._pongHandlers = [];
  this._messageListener = this._messageReceived.bind(this);
  addMessageListener("SPPingService", this._messageListener);
  this._consoleListeners = [];
}

SpecialPowers.prototype = new SpecialPowersAPI();

SpecialPowers.prototype.toString = function() { return "[SpecialPowers]"; };
SpecialPowers.prototype.sanityCheck = function() { return "foo"; };
 

SpecialPowers.prototype.DOMWindowUtils = undefined;

SpecialPowers.prototype._sendSyncMessage = function(msgname, msg) {
  return sendSyncMessage(msgname, msg);
};

SpecialPowers.prototype._sendAsyncMessage = function(msgname, msg) {
  sendAsyncMessage(msgname, msg);
};

SpecialPowers.prototype.registerProcessCrashObservers = function() {
  addMessageListener("SPProcessCrashService", this._messageListener);
  sendSyncMessage("SPProcessCrashService", { op: "register-observer" });
};

SpecialPowers.prototype.unregisterProcessCrashObservers = function() {
  addMessageListener("SPProcessCrashService", this._messageListener);
  sendSyncMessage("SPProcessCrashService", { op: "unregister-observer" });
};

SpecialPowers.prototype._messageReceived = function(aMessage) {
  switch (aMessage.name) {
    case "SPProcessCrashService":
      if (aMessage.json.type == "crash-observed") {
        var self = this;
        aMessage.json.dumpIDs.forEach(function(id) {
          self._encounteredCrashDumpFiles.push(id + ".dmp");
          self._encounteredCrashDumpFiles.push(id + ".extra");
        });
      }
      break;

    case "SPPingService":
      if (aMessage.json.op == "pong") {
        var handler = this._pongHandlers.shift();
        if (handler) {
          handler();
        }
      }
      break;
  }
  return true;
};

SpecialPowers.prototype.executeAfterFlushingMessageQueue = function(aCallback) {
  this._pongHandlers.push(aCallback);
  sendAsyncMessage("SPPingService", { op: "ping" });
};




SpecialPowers.prototype.__exposedProps__ = {};
for (var i in SpecialPowers.prototype) {
  if (i.charAt(0) != "_")
    SpecialPowers.prototype.__exposedProps__[i] = "r";
}


function attachSpecialPowersToWindow(aWindow) {
  try {
    if ((aWindow !== null) &&
        (aWindow !== undefined) &&
        (aWindow.wrappedJSObject) &&
        (aWindow.parent !== null) &&
        (aWindow.parent !== undefined) &&
        (aWindow.parent.wrappedJSObject.SpecialPowers) &&
        !(aWindow.wrappedJSObject.SpecialPowers)) {
      aWindow.wrappedJSObject.SpecialPowers = aWindow.parent.SpecialPowers;
    }
    else if ((aWindow !== null) &&
             (aWindow !== undefined) &&
             (aWindow.wrappedJSObject) &&
             !(aWindow.wrappedJSObject.SpecialPowers)) {
      aWindow.wrappedJSObject.SpecialPowers = new SpecialPowers(aWindow);
    }
  } catch(ex) {
    dump("TEST-INFO | specialpowers.js |  Failed to attach specialpowers to window exception: " + ex + "\n");
  }
}






function SpecialPowersManager() {
  addEventListener("DOMWindowCreated", this, false);
}

SpecialPowersManager.prototype = {
  handleEvent: function handleEvent(aEvent) {
    var window = aEvent.target.defaultView;
    attachSpecialPowersToWindow(window);
  }
};

var specialpowersmanager = new SpecialPowersManager();
