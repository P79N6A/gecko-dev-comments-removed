





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.views = (function(OT) {
  "use strict";

  var sharedModels = loop.shared.models;

  


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

    
    
    videoStyles: { width: "100%", height: "auto" },

    events: {
      'click .btn.stop': 'hangup'
    },

    


    initialize: function(options) {
      options = options || {};
      if (!options.sdk) {
        throw new Error("missing required sdk");
      }
      this.sdk = options.sdk;
      
      
      
      this.session   = this.sdk.initSession(this.model.get("sessionId"));
      this.publisher = this.sdk.initPublisher(this.model.get("apiKey"),
                                              "outgoing", this.videoStyles);

      this.session.connect(this.model.get("apiKey"),
                           this.model.get("sessionToken"));

      this.listenTo(this.session, "sessionConnected", this._sessionConnected);
      this.listenTo(this.session, "streamCreated", this._streamCreated);
      this.listenTo(this.session, "connectionDestroyed",
                                  this._connectionDestroyed);
      this.listenTo(this.session, "sessionDisconnected",
                                  this._sessionDisconnected);
      this.listenTo(this.session, "networkDisconnected",
                                  this._networkDisconnected);
    },

    hangup: function(event) {
      event.preventDefault();
      this.session.disconnect();
    },

    





    _sessionConnected: function(event) {
      this.session.publish(this.publisher);
    },

    





    _streamCreated: function(event) {
      this._subscribeToStreams(event.streams);
    },

    





    _sessionDisconnected: function(event) {
      this.model.trigger("session:ended");
    },

    





    _connectionDestroyed: function(event) {
      this.model.trigger("session:peer-hungup", {
        connectionId: event.connection.connectionId
      });
      this.session.unpublish(this.publisher);
      this.session.disconnect();
    },

    





    _networkDisconnected: function(event) {
      this.model.trigger("session:network-disconnected");
      this.session.unpublish(this.publisher);
      this.session.disconnect();
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

  


  var NotificationView = Backbone.View.extend({
    template: _.template([
      '<div class="alert alert-<%- level %>">',
      '  <button class="close"></button>',
      '  <p class="message"><%- message %></p>',
      '</div>'
    ].join("")),

    events: {
      "click .close": "dismiss"
    },

    dismiss: function(event) {
      event.preventDefault();
      this.$el.addClass("fade-out");
      setTimeout(function() {
        this.collection.remove(this.model);
        this.remove();
      }.bind(this), 500); 
    },

    render: function() {
      this.$el.html(this.template(this.model.toJSON()));
      return this;
    }
  });

  


  var NotificationListView = Backbone.View.extend({
    








    initialize: function(options) {
      options = options || {};
      if (!options.collection) {
        this.collection = new sharedModels.NotificationCollection();
      }
      this.listenTo(this.collection, "reset add remove", this.render);
    },

    


    clear: function() {
      this.collection.reset();
    },

    




    notify: function(notification) {
      this.collection.add(notification);
    },

    




    warn: function(message) {
      this.notify({level: "warning", message: message});
    },

    




    error: function(message) {
      this.notify({level: "error", message: message});
    },

    




    render: function() {
      this.$el.html(this.collection.map(function(notification) {
        return new NotificationView({
          model: notification,
          collection: this.collection
        }).render().$el;
      }.bind(this)));
      return this;
    }
  });

  return {
    BaseView: BaseView,
    ConversationView: ConversationView,
    NotificationListView: NotificationListView,
    NotificationView: NotificationView
  };
})(window.OT);
