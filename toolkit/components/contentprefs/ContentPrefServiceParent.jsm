




"use strict";

this.EXPORTED_SYMBOLS = [ "ContentPrefServiceParent" ];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

let ContentPrefServiceParent = {
  _cps2: null,

  init: function() {
    let globalMM = Cc["@mozilla.org/parentprocessmessagemanager;1"]
                     .getService(Ci.nsIMessageListenerManager);

    this._cps2 = Cc["@mozilla.org/content-pref/service;1"]
                  .getService(Ci.nsIContentPrefService2);

    globalMM.addMessageListener("ContentPrefs:FunctionCall", this);

    let observerChangeHandler = this.handleObserverChange.bind(this);
    globalMM.addMessageListener("ContentPrefs:AddObserverForName", observerChangeHandler);
    globalMM.addMessageListener("ContentPrefs:RemoveObserverForName", observerChangeHandler);
    globalMM.addMessageListener("child-process-shutdown", observerChangeHandler);
  },

  
  _observers: new Map(),

  handleObserverChange: function(msg) {
    let observer = this._observers.get(msg.target);
    if (msg.name === "child-process-shutdown") {
      for (let i of observer._names) {
        this._cps2.removeObserverForName(i, observer);
      }

      this._observers.delete(msg.target);
      return;
    }

    let prefName = msg.data.name;
    if (msg.name === "ContentPrefs:AddObserverForName") {
      
      
      if (!observer) {
        observer = {
          onContentPrefSet: function(group, name, value) {
            msg.target.sendAsyncMessage("ContentPrefs:NotifyObservers",
                                        { name: name, callback: "onContentPrefSet",
                                          args: [ group, name, value ] });
          },

          onContentPrefRemoved: function(group, name) {
            msg.target.sendAsyncMessage("ContentPrefs:NotifyObservers",
                                        { name: name, callback: "onContentPrefRemoved",
                                          args: [ group, name ] });
          },

          
          
          
          _names: new Set()
        };

        this._observers.set(msg.target, observer);
      }

      observer._names.add(prefName);

      this._cps2.addObserverForName(prefName, observer);
    } else {
      

      
      this._cps2.removeObserverForName(prefName, observer);

      observer._names.delete(prefName);
      if (observer._names.size === 0) {
        
        this._observers.delete(msg.target);
      }
    }
  },

  receiveMessage: function(msg) {
    let data = msg.data;

    let args = data.args;
    let requestId = data.requestId;

    let listener = {
      handleResult: function(pref) {
        msg.target.sendAsyncMessage("ContentPrefs:HandleResult",
                                    { requestId: requestId,
                                      contentPref: pref });
      },

      handleError: function(error) {
        msg.target.sendAsyncMessage("ContentPrefs:HandleError",
                                    { requestId: requestId,
                                      error: error });
      },

      handleCompletion: function(reason) {
        msg.target.sendAsyncMessage("ContentPrefs:HandleCompletion",
                                    { requestId: requestId,
                                      reason: reason });
      }
    };

    
    args.push(listener);

    
    this._cps2[data.call](...args);
  }
};
