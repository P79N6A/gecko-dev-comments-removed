







var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.views = (function(_, OT, l10n) {
  "use strict";

  var sharedModels = loop.shared.models;
  var WINDOW_AUTOCLOSE_TIMEOUT_IN_SECONDS = 5;

  


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

  








  var MediaControlButton = React.createClass({displayName: 'MediaControlButton',
    propTypes: {
      scope: React.PropTypes.string.isRequired,
      type: React.PropTypes.string.isRequired,
      action: React.PropTypes.func.isRequired,
      enabled: React.PropTypes.bool.isRequired
    },

    getDefaultProps: function() {
      return {enabled: true};
    },

    handleClick: function() {
      this.props.action();
    },

    _getClasses: function() {
      var cx = React.addons.classSet;
      
      var classesObj = {
        "btn": true,
        "media-control": true,
        "local-media": this.props.scope === "local",
        "muted": !this.props.enabled
      };
      classesObj["btn-mute-" + this.props.type] = true;
      return cx(classesObj);
    },

    _getTitle: function(enabled) {
      var prefix = this.props.enabled ? "mute" : "unmute";
      var suffix = "button_title";
      var msgId = [prefix, this.props.scope, this.props.type, suffix].join("_");
      return l10n.get(msgId);
    },

    render: function() {
      return (
        
        React.DOM.button({className: this._getClasses(), 
                title: this._getTitle(), 
                onClick: this.handleClick})
        
      );
    }
  });

  


  var ConversationToolbar = React.createClass({displayName: 'ConversationToolbar',
    getDefaultProps: function() {
      return {
        video: {enabled: true},
        audio: {enabled: true}
      };
    },

    propTypes: {
      video: React.PropTypes.object.isRequired,
      audio: React.PropTypes.object.isRequired,
      hangup: React.PropTypes.func.isRequired,
      publishStream: React.PropTypes.func.isRequired
    },

    handleClickHangup: function() {
      this.props.hangup();
    },

    handleToggleVideo: function() {
      this.props.publishStream("video", !this.props.video.enabled);
    },

    handleToggleAudio: function() {
      this.props.publishStream("audio", !this.props.audio.enabled);
    },

    render: function() {
      
      return (
        React.DOM.ul({className: "conversation-toolbar"}, 
          React.DOM.li(null, React.DOM.button({className: "btn btn-hangup", 
                      onClick: this.handleClickHangup, 
                      title: l10n.get("hangup_button_title")})), 
          React.DOM.li(null, MediaControlButton({action: this.handleToggleVideo, 
                                  enabled: this.props.video.enabled, 
                                  scope: "local", type: "video"})), 
          React.DOM.li(null, MediaControlButton({action: this.handleToggleAudio, 
                                  enabled: this.props.audio.enabled, 
                                  scope: "local", type: "audio"}))
        )
      );
      
    }
  });

  var ConversationView = React.createClass({displayName: 'ConversationView',
    mixins: [Backbone.Events],

    propTypes: {
      sdk: React.PropTypes.object.isRequired,
      model: React.PropTypes.object.isRequired
    },

    
    
    publisherConfig: {
      insertMode: "append",
      width: "100%",
      height: "100%",
      style: {
        bugDisplayMode: "off",
        buttonDisplayMode: "off",
        nameDisplayMode: "off"
      }
    },

    getInitialState: function() {
      return {
        video: {enabled: false},
        audio: {enabled: false}
      };
    },

    componentDidMount: function() {
      this.listenTo(this.props.model, "session:connected",
                                      this.startPublishing);
      this.listenTo(this.props.model, "session:stream-created",
                                      this._streamCreated);
      this.listenTo(this.props.model, ["session:peer-hungup",
                                       "session:network-disconnected",
                                       "session:ended"].join(" "),
                                       this.stopPublishing);

      this.props.model.startSession();
    },

    componentWillUnmount: function() {
      
      this.stopListening();
      this.hangup();
    },

    hangup: function() {
      this.stopPublishing();
      this.props.model.endSession();
    },

    









    _streamCreated: function(event) {
      var incoming = this.getDOMNode().querySelector(".remote");
      event.streams.forEach(function(stream) {
        if (stream.connection.connectionId !==
            this.props.model.session.connection.connectionId) {
          this.props.model.session.subscribe(stream, incoming,
                                             this.publisherConfig);
        }
      }, this);
    },

    






    startPublishing: function(event) {
      var outgoing = this.getDOMNode().querySelector(".local");

      
      this.publisher = this.props.sdk.initPublisher(
        outgoing, this.publisherConfig);

      
      this.listenTo(this.publisher, "accessDialogOpened accessDenied",
                    function(event) {
                      event.preventDefault();
                    });

      this.listenTo(this.publisher, "streamCreated", function(event) {
        this.setState({
          audio: {enabled: event.stream.hasAudio},
          video: {enabled: event.stream.hasVideo}
        });
      }.bind(this));

      this.listenTo(this.publisher, "streamDestroyed", function() {
        this.setState({
          audio: {enabled: false},
          video: {enabled: false}
        });
      }.bind(this));

      this.props.model.session.publish(this.publisher);
    },

    





    publishStream: function(type, enabled) {
      if (type === "audio") {
        this.publisher.publishAudio(enabled);
        this.setState({audio: {enabled: enabled}});
      } else {
        this.publisher.publishVideo(enabled);
        this.setState({video: {enabled: enabled}});
      }
    },

    


    stopPublishing: function() {
      
      this.stopListening(this.publisher);

      this.props.model.session.unpublish(this.publisher);
    },

    render: function() {
      
      return (
        React.DOM.div({className: "conversation"}, 
          ConversationToolbar({video: this.state.video, 
                               audio: this.state.audio, 
                               publishStream: this.publishStream, 
                               hangup: this.hangup}), 
          React.DOM.div({className: "media nested"}, 
            React.DOM.div({className: "video_wrapper remote_wrapper"}, 
              React.DOM.div({className: "video_inner remote"})
            ), 
            React.DOM.div({className: "local"})
          )
        )
      );
      
    }
  });

  





  var FeedbackLayout = React.createClass({displayName: 'FeedbackLayout',
    propTypes: {
      children: React.PropTypes.component.isRequired,
      title: React.PropTypes.string.isRequired,
      reset: React.PropTypes.func 
    },

    render: function() {
      var backButton = React.DOM.div(null);
      if (this.props.reset) {
        backButton = (
          React.DOM.button({className: "back", type: "button", onClick: this.props.reset}, 
            "« ", l10n.get("feedback_back_button")
          )
        );
      }
      return (
        React.DOM.div({className: "feedback"}, 
          backButton, 
          React.DOM.h3(null, this.props.title), 
          this.props.children
        )
      );
    }
  });

  


  var FeedbackForm = React.createClass({displayName: 'FeedbackForm',
    propTypes: {
      pending:      React.PropTypes.bool,
      sendFeedback: React.PropTypes.func,
      reset:        React.PropTypes.func
    },

    getInitialState: function() {
      return {category: "", description: ""};
    },

    getInitialProps: function() {
      return {pending: false};
    },

    _getCategories: function() {
      return {
        audio_quality: l10n.get("feedback_category_audio_quality"),
        video_quality: l10n.get("feedback_category_video_quality"),
        disconnected : l10n.get("feedback_category_was_disconnected"),
        confusing:     l10n.get("feedback_category_confusing"),
        other:         l10n.get("feedback_category_other")
      };
    },

    _getCategoryFields: function() {
      var categories = this._getCategories();
      return Object.keys(categories).map(function(category, key) {
        return (
          React.DOM.label({key: key}, 
            React.DOM.input({type: "radio", ref: "category", name: "category", 
                   value: category, 
                   onChange: this.handleCategoryChange, 
                   checked: this.state.category === category}), 
            categories[category]
          )
        );
      }, this);
    },

    









    _isFormReady: function() {
      if (this.props.pending || !this.state.category) {
        return false;
      }
      if (this.state.category === "other" && !this.state.description) {
        return false;
      }
      return true;
    },

    handleCategoryChange: function(event) {
      var category = event.target.value;
      this.setState({
        category: category,
        description: category == "other" ? "" : this._getCategories()[category]
      });
      if (category == "other") {
        this.refs.description.getDOMNode().focus();
      }
    },

    handleDescriptionFieldChange: function(event) {
      this.setState({description: event.target.value});
    },

    handleDescriptionFieldFocus: function(event) {
      this.setState({category: "other", description: ""});
    },

    handleFormSubmit: function(event) {
      event.preventDefault();
      this.props.sendFeedback({
        happy: false,
        category: this.state.category,
        description: this.state.description
      });
    },

    render: function() {
      var descriptionDisplayValue = this.state.category === "other" ?
                                    this.state.description : "";
      return (
        FeedbackLayout({title: l10n.get("feedback_what_makes_you_sad"), 
                        reset: this.props.reset}, 
          React.DOM.form({onSubmit: this.handleFormSubmit}, 
            this._getCategoryFields(), 
            React.DOM.p(null, 
              React.DOM.input({type: "text", ref: "description", name: "description", 
                onChange: this.handleDescriptionFieldChange, 
                onFocus: this.handleDescriptionFieldFocus, 
                value: descriptionDisplayValue, 
                placeholder: 
                  l10n.get("feedback_custom_category_text_placeholder")})
            ), 
            React.DOM.button({type: "submit", className: "btn btn-success", 
                    disabled: !this._isFormReady()}, 
              l10n.get("feedback_submit_button")
            )
          )
        )
      );
    }
  });

  


  var FeedbackReceived = React.createClass({displayName: 'FeedbackReceived',
    getInitialState: function() {
      return {countdown: WINDOW_AUTOCLOSE_TIMEOUT_IN_SECONDS};
    },

    componentDidMount: function() {
      this._timer = setInterval(function() {
        this.setState({countdown: this.state.countdown - 1});
      }.bind(this), 1000);
    },

    componentWillUnmount: function() {
      if (this._timer) {
        clearInterval(this._timer);
      }
    },

    render: function() {
      if (this.state.countdown < 1) {
        clearInterval(this._timer);
        window.close();
      }
      return (
        FeedbackLayout({title: l10n.get("feedback_thank_you_heading")}, 
          React.DOM.p({className: "info thank-you"}, 
            l10n.get("feedback_window_will_close_in", {
              countdown: this.state.countdown
            }))
        )
      );
    }
  });

  


  var FeedbackView = React.createClass({displayName: 'FeedbackView',
    propTypes: {
      
      feedbackApiClient: React.PropTypes.object.isRequired,
      
      step: React.PropTypes.oneOf(["start", "form", "finished"])
    },

    getInitialState: function() {
      return {pending: false, step: this.props.step || "start"};
    },

    getInitialProps: function() {
      return {step: "start"};
    },

    reset: function() {
      this.setState(this.getInitialState());
    },

    handleHappyClick: function() {
      this.sendFeedback({happy: true}, this._onFeedbackSent);
    },

    handleSadClick: function() {
      this.setState({step: "form"});
    },

    sendFeedback: function(fields) {
      
      
      this.setState({pending: true});
      
      this.props.feedbackApiClient.send(fields, this._onFeedbackSent);
    },

    _onFeedbackSent: function(err) {
      if (err) {
        
        console.error("Unable to send user feedback", err);
      }
      this.setState({pending: false, step: "finished"});
    },

    render: function() {
      switch(this.state.step) {
        case "finished":
          return FeedbackReceived(null);
        case "form":
          return FeedbackForm({feedbackApiClient: this.props.feedbackApiClient, 
                               sendFeedback: this.sendFeedback, 
                               reset: this.reset, 
                               pending: this.state.pending});
        default:
          return (
            FeedbackLayout({title: 
              l10n.get("feedback_call_experience_heading")}, 
              React.DOM.div({className: "faces"}, 
                React.DOM.button({className: "face face-happy", 
                        onClick: this.handleHappyClick}), 
                React.DOM.button({className: "face face-sad", 
                        onClick: this.handleSadClick})
              )
            )
          );
      }
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
    ConversationToolbar: ConversationToolbar,
    FeedbackView: FeedbackView,
    MediaControlButton: MediaControlButton,
    NotificationListView: NotificationListView,
    NotificationView: NotificationView,
    UnsupportedBrowserView: UnsupportedBrowserView,
    UnsupportedDeviceView: UnsupportedDeviceView
  };
})(_, window.OT, document.webL10n || document.mozL10n);
