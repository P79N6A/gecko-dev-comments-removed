





var loop = loop || {};
loop.ConversationStore = (function() {

  var ConversationStore = Backbone.Model.extend({
    defaults: {
      outgoing: false,
      calleeId: undefined,
      callState: "gather"
    },
  });

  return ConversationStore;
})();
