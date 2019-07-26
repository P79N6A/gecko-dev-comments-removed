





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.views = (function(_, OT, l10n) {
  "use strict";

  var sharedModels = loop.shared.models;

  


  var L10nView = (function() {
    var L10nViewImpl = Backbone.View.extend(),
        extend       = L10nViewImpl.extend;

    
    
    L10nViewImpl.extend = function() {
      var ExtendedView = extend.apply(this, arguments),
          render       = ExtendedView.prototype.render;

      
      
      ExtendedView.prototype.render = function() {
        if (render) {
          render.apply(this, arguments);
          l10n.translate(this.el);
        }
        return this;
      };

      return ExtendedView;
    };

    return L10nViewImpl;
  })();

  


  var BaseView = L10nView.extend({
    




    hide: function() {
      this.$el.hide();
      return this;
    },

    




    show: function() {
      this.$el.show();
      return this;
    },

    







    render: function() {
      if (this.template) {
        this.$el.html(this.template());
      }
      return this;
    }
  });

  


  var ConversationView = BaseView.extend({
    className: "conversation",

    template: _.template([
      '<nav class="controls">',
      '  <button class="btn stop" data-l10n-id="stop"></button>',
      '</nav>',
      '<div class="media nested">',
      
      
      
      
      '  <div class="remote"><div class="incoming"></div></div>',
      '  <div class="local"><div class="outgoing"></div></div>',
      '</div>'
    ].join("")),

    
    
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

      
      
      
      this.session = this.sdk.initSession(this.model.get("sessionId"));
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
      this.publisher = this.sdk.initPublisher(this.$(".outgoing").get(0),
                                              this.videoStyles);
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
      var incomingContainer = this.$(".incoming").get(0);
      streams.forEach(function(stream) {
        if (stream.connection.connectionId !==
            this.session.connection.connectionId) {
          this.session.subscribe(stream, incomingContainer, this.videoStyles);
        }
      }.bind(this));
    },

    




    render: function() {
      this.$el.html(this.template(this.model.toJSON()));
      return this;
    }
  });

  


  var NotificationView = BaseView.extend({
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
    L10nView: L10nView,
    BaseView: BaseView,
    ConversationView: ConversationView,
    NotificationListView: NotificationListView,
    NotificationView: NotificationView
  };
})(_, window.OT, document.webL10n || document.mozL10n);
