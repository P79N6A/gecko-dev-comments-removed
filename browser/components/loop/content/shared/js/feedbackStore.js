





var loop = loop || {};
loop.store = loop.store || {};

loop.store.FeedbackStore = (function() {
  "use strict";

  var sharedActions = loop.shared.actions;
  var FEEDBACK_STATES = loop.store.FEEDBACK_STATES = {
    
    INIT: "feedback-init",
    
    DETAILS: "feedback-details",
    
    PENDING: "feedback-pending",
    
    SENT: "feedback-sent",
    
    FAILED: "feedback-failed"
  };

  








  var FeedbackStore = loop.store.createStore({
    actions: [
      "requireFeedbackDetails",
      "sendFeedback",
      "sendFeedbackError"
    ],

    initialize: function(options) {
      if (!options.feedbackClient) {
        throw new Error("Missing option feedbackClient");
      }
      this._feedbackClient = options.feedbackClient;
    },

    


    getInitialStoreState: function() {
      return {feedbackState: FEEDBACK_STATES.INIT};
    },

    


    requireFeedbackDetails: function() {
      this.setStoreState({feedbackState: FEEDBACK_STATES.DETAILS});
    },

    




    sendFeedback: function(actionData) {
      delete actionData.name;
      this._feedbackClient.send(actionData, function(err) {
        if (err) {
          this.dispatchAction(new sharedActions.SendFeedbackError({
            error: err
          }));
          return;
        }
        this.setStoreState({feedbackState: FEEDBACK_STATES.SENT});
      }.bind(this));

      this.setStoreState({feedbackState: FEEDBACK_STATES.PENDING});
    },

    




    sendFeedbackError: function(actionData) {
      this.setStoreState({
        feedbackState: FEEDBACK_STATES.FAILED,
        error: actionData.error
      });
    }
  });

  return FeedbackStore;
})();
