



function ChromePowers(window) {
  this.window = Components.utils.getWeakReference(window);

  
  
  if (typeof(window) == "ChromeWindow" && typeof(content.window) == "Window") {
    this.DOMWindowUtils = bindDOMWindowUtils(content.window);
    this.window = Components.utils.getWeakReference(content.window);
  } else {
    this.DOMWindowUtils = bindDOMWindowUtils(window);
  }

  this.spObserver = new SpecialPowersObserverAPI();
}

ChromePowers.prototype = new SpecialPowersAPI();

ChromePowers.prototype.toString = function() { return "[ChromePowers]"; };
ChromePowers.prototype.sanityCheck = function() { return "foo"; };


ChromePowers.prototype.DOMWindowUtils = undefined;

ChromePowers.prototype._sendSyncMessage = function(type, msg) {
  var aMessage = {'name':type, 'json': msg};
  return [this._receiveMessage(aMessage)];
};

ChromePowers.prototype._sendAsyncMessage = function(type, msg) {
  var aMessage = {'name':type, 'json': msg};
  this._receiveMessage(aMessage);
};

ChromePowers.prototype.registerProcessCrashObservers = function() {
  this._sendSyncMessage("SPProcessCrashService", { op: "register-observer" });
};

ChromePowers.prototype.unregisterProcessCrashObservers = function() {
  this._sendSyncMessage("SPProcessCrashService", { op: "unregister-observer" });
};

ChromePowers.prototype._receiveMessage = function(aMessage) {
  switch (aMessage.name) {
    case "SpecialPowers.Quit":
      let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
      appStartup.quit(Ci.nsIAppStartup.eForceQuit);
      break;
    case "SPProcessCrashService":
      if (aMessage.json.op == "register-observer" || aMessage.json.op == "unregister-observer") {
        
        break;
      } else if (aMessage.type == "crash-observed") {
        for (let e of msg.dumpIDs) {
          this._encounteredCrashDumpFiles.push(e.id + "." + e.extension);
        }
      }
    default:
      
      return this.spObserver._receiveMessageAPI(aMessage);
  }
  return undefined;		
};

ChromePowers.prototype.quit = function() {
  
  
  
  SpecialPowers._sendSyncMessage("SpecialPowers.Quit", {});
};

ChromePowers.prototype.focus = function(aWindow) {
  
  
  
  if (aWindow)
    aWindow.focus();
};

ChromePowers.prototype.executeAfterFlushingMessageQueue = function(aCallback) {
  aCallback();
};




ChromePowers.prototype.__exposedProps__ = {};
for (var i in ChromePowers.prototype) {
  if (i.charAt(0) != "_")
    ChromePowers.prototype.__exposedProps__[i] = "r";
}

if ((window.parent !== null) &&
    (window.parent !== undefined) &&
    (window.parent.wrappedJSObject.SpecialPowers) &&
    !(window.wrappedJSObject.SpecialPowers)) {
  window.wrappedJSObject.SpecialPowers = window.parent.SpecialPowers;
} else {
  window.wrappedJSObject.SpecialPowers = new ChromePowers(window);
}

