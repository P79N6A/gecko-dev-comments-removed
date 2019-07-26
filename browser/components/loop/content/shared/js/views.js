





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.views = (function(_, OT, l10n) {
  "use strict";

  var sharedModels = loop.shared.models;

  


  var L10nView = (function() {
    var L10nViewImpl   = Backbone.View.extend(), 
        originalExtend = L10nViewImpl.extend;    

    





    L10nViewImpl.extend = function() {
      var ExtendedView   = originalExtend.apply(this, arguments),
          originalRender = ExtendedView.prototype.render;

      





      ExtendedView.prototype.render = function() {
        if (originalRender) {
          originalRender.apply(this, arguments);
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

      this.listenTo(this.model, "session:connected", this.publish);
      this.listenTo(this.model, "session:stream-created", this._streamCreated);
      this.listenTo(this.model, ["session:peer-hungup",
                                 "session:network-disconnected",
                                 "session:ended"].join(" "), this.unpublish);
      this.model.startSession();
    },

    









    _streamCreated: function(event) {
      var incoming = this.$(".incoming").get(0);
      event.streams.forEach(function(stream) {
        if (stream.connection.connectionId !==
            this.model.session.connection.connectionId) {
          this.model.session.subscribe(stream, incoming, this.videoStyles);
        }
      }.bind(this));
    },

    




    hangup: function(event) {
      event.preventDefault();
      this.unpublish();
      this.model.endSession();
    },

    






    publish: function(event) {
      var outgoing = this.$(".outgoing").get(0);
      this.publisher = this.sdk.initPublisher(outgoing, this.videoStyles);
      this.model.session.publish(this.publisher);
    },

    


    unpublish: function() {
      this.model.session.unpublish(this.publisher);
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

    






    notifyL10n: function(messageId, level) {
      this.notify({
        message: l10n.get(messageId),
        level: level
      });
    },

    




    warn: function(message) {
      this.notify({level: "warning", message: message});
    },

    




    warnL10n: function(messageId) {
      this.warn(l10n.get(messageId));
    },

    




    error: function(message) {
      this.notify({level: "error", message: message});
    },

    




    errorL10n: function(messageId) {
      this.error(l10n.get(messageId));
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

  


  var UnsupportedView = BaseView.extend({
    template: _.template([
      '<div>',
        '<h2 data-l10n-id="incompatible_browser"></h2>',
        '<p data-l10n-id="powered_by_webrtc"></p>',
        '<p data-l10n-id="use_latest_firefox" ',
          'data-l10n-args=\'{"ff_url": "https://www.mozilla.org/firefox/"}\'>',
        '</p>',
      '</div>'
    ].join(""))
  });

  return {
    L10nView: L10nView,
    BaseView: BaseView,
    ConversationView: ConversationView,
    NotificationListView: NotificationListView,
    NotificationView: NotificationView,
    UnsupportedView: UnsupportedView
  };
})(_, window.OT, document.webL10n || document.mozL10n);
