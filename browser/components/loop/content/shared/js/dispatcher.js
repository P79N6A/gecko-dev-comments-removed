














var loop = loop || {};
loop.Dispatcher = (function() {

  function Dispatcher() {
    this._eventData = {};
    this._actionQueue = [];
    this._debug = loop.shared.utils.getBoolPreference("debug.dispatcher");
  }

  Dispatcher.prototype = {
    





    register: function(store, eventTypes) {
      eventTypes.forEach(function(type) {
        if (this._eventData.hasOwnProperty(type)) {
          this._eventData[type].push(store);
        } else {
          this._eventData[type] = [store];
        }
      }.bind(this));
    },

    


    dispatch: function(action) {
      
      this._actionQueue.push(action);
      this._dispatchNextAction();
    },

    


    _dispatchNextAction: function() {
      if (!this._actionQueue.length || this._active) {
        return;
      }

      var action = this._actionQueue.shift();
      var type = action.name;

      var registeredStores = this._eventData[type];
      if (!registeredStores) {
        console.warn("No stores registered for event type ", type);
        return;
      }

      this._active = true;

      if (this._debug) {
        console.log("[Dispatcher] Dispatching action", action);
      }

      registeredStores.forEach(function(store) {
        try {
          store[type](action);
        } catch (x) {
          console.error("[Dispatcher] Dispatching action caused an exception: ", x);
        }
      });

      this._active = false;
      this._dispatchNextAction();
    }
  };

  return Dispatcher;
})();
