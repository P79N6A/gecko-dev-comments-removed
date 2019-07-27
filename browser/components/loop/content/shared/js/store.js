





var loop = loop || {};
loop.store = loop.store || {};

loop.store.createStore = (function() {
  "use strict";

  var baseStorePrototype = {
    __registerActions: function(actions) {
      
      actions.forEach(function(handler) {
        if (typeof this[handler] !== "function") {
          throw new Error("Store should implement an action handler for " +
                          handler);
        }
      }, this);
      this.dispatcher.register(this, actions);
    },

    




    dispatchAction: function(action) {
      this.dispatcher.dispatch(action);
    },

    






    getStoreState: function(key) {
      return key ? this._storeState[key] : this._storeState;
    },

    





    setStoreState: function(newState) {
      for (var key in newState) {
        this._storeState[key] = newState[key];
        this.trigger("change:" + key);
      }
      this.trigger("change");
    }
  };

  





  function createStore(storeProto) {
    var BaseStore = function(dispatcher, options) {
      options = options || {};

      if (!dispatcher) {
        throw new Error("Missing required dispatcher");
      }
      this.dispatcher = dispatcher;
      if (Array.isArray(this.actions)) {
        this.__registerActions(this.actions);
      }

      if (typeof this.initialize === "function") {
        this.initialize(options);
      }

      if (typeof this.getInitialStoreState === "function") {
        this._storeState = this.getInitialStoreState();
      } else {
        this._storeState = {};
      }
    };
    BaseStore.prototype = _.extend({}, 
                                   Backbone.Events,
                                   baseStorePrototype,
                                   storeProto);
    return BaseStore;
  }

  return createStore;
})();
