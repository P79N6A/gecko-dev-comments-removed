







var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.views = (function(TB) {
  "use strict";

  


  var BaseView = Backbone.View.extend({
    




    hide: function() {
      this.$el.hide();
      return this;
    },

    




    show: function() {
      this.$el.show();
      return this;
    }
  });

  


  var ConversationView = BaseView.extend({
    el: "#conversation",

    initialize: function() {
      this.videoStyles = { width: "100%", height: "100%" };

      
      
      
      this.session   = TB.initSession(this.model.get("sessionId"));
      this.publisher = TB.initPublisher(this.model.get("apiKey"), "outgoing",
                                        this.videoStyles);

      this.session.connect(this.model.get("apiKey"),
                           this.model.get("sessionToken"));

      this.listenTo(this.session, "sessionConnected", this._sessionConnected);
      this.listenTo(this.session, "streamCreated", this._streamCreated);
      this.listenTo(this.session, "connectionDestroyed", this._sessionEnded);
    },

    _sessionConnected: function(event) {
      this.session.publish(this.publisher);
      this._subscribeToStreams(event.streams);
    },

    _streamCreated: function(event) {
      this._subscribeToStreams(event.streams);
    },

    _sessionEnded: function(event) {
      
      alert("Your session has ended. Reason: " + event.reason);
      this.model.trigger("session:ended");
    },

    _subscribeToStreams: function(streams) {
      streams.forEach(function(stream) {
        if (stream.connection.connectionId !==
            this.session.connection.connectionId) {
          this.session.subscribe(stream, "incoming", this.videoStyles);
        }
      }.bind(this));
    }
  });

  return {
    BaseView: BaseView,
    ConversationView: ConversationView
  };
})(window.TB);
