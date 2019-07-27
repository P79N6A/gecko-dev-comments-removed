








this.EXPORTED_SYMBOLS = ["WebChannel", "WebChannelBroker"];

const ERRNO_UNKNOWN_ERROR              = 999;
const ERROR_UNKNOWN                    = "UNKNOWN_ERROR";


const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");







let WebChannelBroker = Object.create({
  





  registerChannel: function (channel) {
    if (!this._channelMap.has(channel)) {
      this._channelMap.set(channel);
    } else {
      Cu.reportError("Failed to register the channel. Channel already exists.");
    }

    
    if (!this._messageListenerAttached) {
      this._messageListenerAttached = true;
      this._manager.addMessageListener("WebChannelMessageToChrome", this._listener.bind(this));
    }
  },

  







  unregisterChannel: function (channelToRemove) {
    if (!this._channelMap.delete(channelToRemove)) {
      Cu.reportError("Failed to unregister the channel. Channel not found.");
    }
  },

  




  _listener: function (event) {
    let data = event.data;
    let sender = event.target;

    if (data && data.id) {
      if (!event.principal) {
        this._sendErrorEventToContent(data.id, sender, "Message principal missing");
      } else {
        let validChannelFound = false;
        data.message = data.message || {};

        for (var channel of this._channelMap.keys()) {
          if (channel.id === data.id &&
            channel._originCheckCallback(event.principal)) {
            validChannelFound = true;
            channel.deliver(data, sender);
          }
        }

        
        if (!validChannelFound) {
          this._sendErrorEventToContent(data.id, sender, "No Such Channel");
        }
      }
    } else {
      Cu.reportError("WebChannel channel id missing");
    }
  },
  


  _manager: Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager),
  


  _messageListenerAttached: false,
  


  _channelMap: new Map(),
  









  _sendErrorEventToContent: function (id, sender, errorMsg) {
    errorMsg = errorMsg || "Web Channel Broker error";

    if (sender.messageManager) {
      sender.messageManager.sendAsyncMessage("WebChannelMessageToContent", {
        id: id,
        error: errorMsg,
      }, sender);
    }
    Cu.reportError(id.toString() + " error message. " + errorMsg);
  },
});















this.WebChannel = function(id, originOrPermission) {
  if (!id || !originOrPermission) {
    throw new Error("WebChannel id and originOrPermission are required.");
  }

  this.id = id;
  
  
  if (typeof originOrPermission == "string") {
    this._originCheckCallback = requestPrincipal => {
      
      
      
      let uri = Services.io.newURI(requestPrincipal.origin, null, null);
      if (uri.scheme != "https") {
        return false;
      }
      
      let perm = Services.perms.testExactPermissionFromPrincipal(requestPrincipal,
                                                                 originOrPermission);
      return perm == Ci.nsIPermissionManager.ALLOW_ACTION;
    }
  } else {
    
    this._originCheckCallback = requestPrincipal => {
      return originOrPermission.prePath === requestPrincipal.origin;
    }
  }
  this._originOrPermission = originOrPermission;
};

this.WebChannel.prototype = {

  


  id: null,

  



  _originOrPermission: null,

  



  _originCheckCallback: null,

  


  _broker: WebChannelBroker,

  


  _deliverCallback: null,

  












  listen: function (callback) {
    if (this._deliverCallback) {
      throw new Error("Failed to listen. Listener already attached.");
    } else if (!callback) {
      throw new Error("Failed to listen. Callback argument missing.");
    } else {
      this._deliverCallback = callback;
      this._broker.registerChannel(this);
    }
  },

  



  stopListening: function () {
    this._broker.unregisterChannel(this);
    this._deliverCallback = null;
  },

  








  send: function (message, target) {
    if (message && target && target.messageManager) {
      target.messageManager.sendAsyncMessage("WebChannelMessageToContent", {
        id: this.id,
        message: message
      });
    } else if (!message) {
      Cu.reportError("Failed to send a WebChannel message. Message not set.");
    } else {
      Cu.reportError("Failed to send a WebChannel message. Target invalid.");
    }
  },

  







  deliver: function(data, sender) {
    if (this._deliverCallback) {
      try {
        this._deliverCallback(data.id, data.message, sender);
      } catch (ex) {
        this.send({
          errno: ERRNO_UNKNOWN_ERROR,
          error: ex.message ? ex.message : ERROR_UNKNOWN
        }, sender);
        Cu.reportError("Failed to execute callback:" + ex);
      }
    } else {
      Cu.reportError("No callback set for this channel.");
    }
  }
};
