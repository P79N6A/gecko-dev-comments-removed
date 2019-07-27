





var loop = loop || {};
loop.store = loop.store || {};





loop.store.ConversationAppStore = (function() {
  




  var ConversationAppStore = function(options) {
    if (!options.dispatcher) {
      throw new Error("Missing option dispatcher");
    }
    if (!options.mozLoop) {
      throw new Error("Missing option mozLoop");
    }

    this._dispatcher = options.dispatcher;
    this._mozLoop = options.mozLoop;
    this._storeState = {};

    this._dispatcher.register(this, [
      "getWindowData"
    ]);
  };

  ConversationAppStore.prototype = _.extend({
    




    getStoreState: function() {
      return this._storeState;
    },

    




    setStoreState: function(state) {
      this._storeState = state;
      this.trigger("change");
    },

    





    getWindowData: function(actionData) {
      var windowData;
      
      if (this._mozLoop.getLoopBoolPref("test.alwaysUseRooms")) {
        windowData = {type: "room", localRoomId: "42"};
      } else {
        windowData = this._mozLoop.getCallData(actionData.windowId);
      }

      if (!windowData) {
        console.error("Failed to get the window data");
        this.setStoreState({windowType: "failed"});
        return;
      }

      this.setStoreState({windowType: windowData.type});

      this._dispatcher.dispatch(new loop.shared.actions.SetupWindowData({
        windowData: windowData
      }));
    }
  }, Backbone.Events);

  return ConversationAppStore;

})();
