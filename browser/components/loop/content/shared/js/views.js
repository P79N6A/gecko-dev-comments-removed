





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

    



    localStream: null,

    template: _.template([
      '<ul class="controls cf">',
      '  <li><button class="btn btn-hangup" ',
      '              data-l10n-id="hangup_button"></button></li>',
      '  <li><button class="btn media-control btn-mute-video"',
      '              data-l10n-id="mute_local_video_button"></button></li>',
      '  <li><button class="btn media-control btn-mute-audio"',
      '              data-l10n-id="mute_local_audio_button"></button></li>',
      '</ul>',
      '<div class="media nested">',
      
      
      
      
      '  <div class="remote"><div class="incoming"></div></div>',
      '  <div class="local"><div class="outgoing"></div></div>',
      '</div>'
    ].join("")),

    
    
    publisherConfig: {
      width: "100%",
      height: "auto",
      style: {
        bugDisplayMode: "off",
        buttonDisplayMode: "off"
      }
    },

    events: {
      'click .btn-hangup': 'hangup',
      'click .btn-mute-audio': 'toggleMuteAudio',
      'click .btn-mute-video': 'toggleMuteVideo'
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
          this.model.session.subscribe(stream, incoming, this.publisherConfig);
        }
      }, this);
    },

    




    hangup: function(event) {
      event.preventDefault();
      this.unpublish();
      this.model.endSession();
    },

    




    toggleMuteAudio: function(event) {
      event.preventDefault();
      if (!this.localStream) {
        return;
      }
      var msgId;
      var $button = this.$(".btn-mute-audio");
      var enabled = !this.localStream.hasAudio;
      this.publisher.publishAudio(enabled);
      if (enabled) {
        msgId = "mute_local_audio_button.title";
        $button.removeClass("muted");
      } else {
        msgId = "unmute_local_audio_button.title";
        $button.addClass("muted");
      }
      $button.attr("title", l10n.get(msgId));
    },

    




    toggleMuteVideo: function(event) {
      event.preventDefault();
      if (!this.localStream) {
        return;
      }
      var msgId;
      var $button = this.$(".btn-mute-video");
      var enabled = !this.localStream.hasVideo;
      this.publisher.publishVideo(enabled);
      if (enabled) {
        $button.removeClass("muted");
        msgId = "mute_local_video_button.title";
      } else {
        $button.addClass("muted");
        msgId = "unmute_local_video_button.title";
      }
      $button.attr("title", l10n.get(msgId));
    },

    






    publish: function(event) {
      var outgoing = this.$(".outgoing").get(0);

      this.publisher = this.sdk.initPublisher(outgoing, this.publisherConfig);

      
      function preventOpeningAccessDialog(event) {
        event.preventDefault();
      }
      this.publisher.on("accessDialogOpened", preventOpeningAccessDialog);
      this.publisher.on("accessDenied", preventOpeningAccessDialog);
      this.publisher.on("streamCreated", function(event) {
        this.localStream = event.stream;
        if (this.localStream.hasAudio) {
          this.$(".btn-mute-audio").addClass("streaming");
        }
        if (this.localStream.hasVideo) {
          this.$(".btn-mute-video").addClass("streaming");
        }
      }.bind(this));
      this.publisher.on("streamDestroyed", function() {
        this.localStream = null;
        this.$(".btn-mute-audio").removeClass("streaming muted");
        this.$(".btn-mute-video").removeClass("streaming muted");
      }.bind(this));

      this.model.session.publish(this.publisher);
    },

    


    unpublish: function() {
      
      this.publisher.off("accessDialogOpened");
      this.publisher.off("accessDenied");

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

  


  var UnsupportedBrowserView = BaseView.extend({
    template: _.template([
      '<div>',
      '  <h2 data-l10n-id="incompatible_browser"></h2>',
      '  <p data-l10n-id="powered_by_webrtc"></p>',
      '  <p data-l10n-id="use_latest_firefox" ',
      '    data-l10n-args=\'{"ff_url": "https://www.mozilla.org/firefox/"}\'>',
      '  </p>',
      '</div>'
    ].join(""))
  });

  


  var UnsupportedDeviceView = BaseView.extend({
    template: _.template([
      '<div>',
      '  <h2 data-l10n-id="incompatible_device"></h2>',
      '  <p data-l10n-id="sorry_device_unsupported"></p>',
      '  <p data-l10n-id="use_firefox_windows_mac_linux"></p>',
      '</div>'
    ].join(""))
  });

  return {
    L10nView: L10nView,
    BaseView: BaseView,
    ConversationView: ConversationView,
    NotificationListView: NotificationListView,
    NotificationView: NotificationView,
    UnsupportedBrowserView: UnsupportedBrowserView,
    UnsupportedDeviceView: UnsupportedDeviceView
  };
})(_, window.OT, document.webL10n || document.mozL10n);
