



var loop = loop || {};
loop.store = loop.store || {};





loop.store.ConversationAppStore = (function() {
  "use strict";

  




  var ConversationAppStore = function(options) {
    if (!options.dispatcher) {
      throw new Error("Missing option dispatcher");
    }
    if (!options.mozLoop) {
      throw new Error("Missing option mozLoop");
    }

    this._dispatcher = options.dispatcher;
    this._mozLoop = options.mozLoop;
    this._storeState = this.getInitialStoreState();

    this._dispatcher.register(this, [
      "getWindowData",
      "showFeedbackForm"
    ]);
  };

  ConversationAppStore.prototype = _.extend({
    getInitialStoreState: function() {
      return {
        
        feedbackPeriod: this._mozLoop.getLoopPref("feedback.periodSec") * 1000,
        
        feedbackTimestamp: this._mozLoop
                               .getLoopPref("feedback.dateLastSeenSec") * 1000,
        showFeedbackForm: false
      };
    },

    




    getStoreState: function() {
      return this._storeState;
    },

    




    setStoreState: function(state) {
      this._storeState = state;
      this.trigger("change");
    },

    



    showFeedbackForm: function() {
      var timestamp = Math.floor(new Date().getTime() / 1000);

      this._mozLoop.setLoopPref("feedback.dateLastSeenSec", timestamp);

      this.setStoreState({
        showFeedbackForm: true
      });
    },

    





    getWindowData: function(actionData) {
      var windowData = this._mozLoop.getConversationWindowData(actionData.windowId);

      if (!windowData) {
        console.error("Failed to get the window data");
        this.setStoreState({windowType: "failed"});
        return;
      }

      this.setStoreState({windowType: windowData.type});

      this._dispatcher.dispatch(new loop.shared.actions.SetupWindowData(_.extend({
        windowId: actionData.windowId}, windowData)));
    }
  }, Backbone.Events);

  return ConversationAppStore;

})();
