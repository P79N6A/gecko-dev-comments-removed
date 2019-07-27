







var loop = loop || {};
loop.conversationViews = (function(mozL10n) {

  var CALL_STATES = loop.store.CALL_STATES;
  var CALL_TYPES = loop.shared.utils.CALL_TYPES;
  var sharedActions = loop.shared.actions;
  var sharedUtils = loop.shared.utils;
  var sharedViews = loop.shared.views;

  
  
  
  function _getPreferredEmail(contact) {
    
    if (!contact.email || contact.email.length === 0) {
      return { value: "" };
    }
    return contact.email.find(e => e.pref) || contact.email[0];
  }

  



  var CallIdentifierView = React.createClass({displayName: 'CallIdentifierView',
    propTypes: {
      peerIdentifier: React.PropTypes.string,
      showIcons: React.PropTypes.bool.isRequired,
      urlCreationDate: React.PropTypes.string,
      video: React.PropTypes.bool
    },

    getDefaultProps: function() {
      return {
        peerIdentifier: "",
        showLinkDetail: true,
        urlCreationDate: "",
        video: true
      };
    },

    getInitialState: function() {
      return {timestamp: 0};
    },

    


    formatCreationDate: function() {
      if (!this.props.urlCreationDate) {
        return "";
      }

      var timestamp = this.props.urlCreationDate;
      return "(" + loop.shared.utils.formatDate(timestamp) + ")";
    },

    render: function() {
      var iconVideoClasses = React.addons.classSet({
        "fx-embedded-tiny-video-icon": true,
        "muted": !this.props.video
      });
      var callDetailClasses = React.addons.classSet({
        "fx-embedded-call-detail": true,
        "hide": !this.props.showIcons
      });

      return (
        React.DOM.div({className: "fx-embedded-call-identifier"}, 
          React.DOM.div({className: "fx-embedded-call-identifier-avatar fx-embedded-call-identifier-item"}), 
          React.DOM.div({className: "fx-embedded-call-identifier-info fx-embedded-call-identifier-item"}, 
            React.DOM.div({className: "fx-embedded-call-identifier-text overflow-text-ellipsis"}, 
              this.props.peerIdentifier
            ), 
            React.DOM.div({className: callDetailClasses}, 
              React.DOM.span({className: "fx-embedded-tiny-audio-icon"}), 
              React.DOM.span({className: iconVideoClasses}), 
              React.DOM.span({className: "fx-embedded-conversation-timestamp"}, 
                this.formatCreationDate()
              )
            )
          )
        )
      );
    }
  });

  






  var ConversationDetailView = React.createClass({displayName: 'ConversationDetailView',
    propTypes: {
      contact: React.PropTypes.object
    },

    render: function() {
      var contactName;

      if (this.props.contact.name &&
          this.props.contact.name[0]) {
        contactName = this.props.contact.name[0];
      } else {
        contactName = _getPreferredEmail(this.props.contact).value;
      }

      document.title = contactName;

      return (
        React.DOM.div({className: "call-window"}, 
          CallIdentifierView({
            peerIdentifier: contactName, 
            showIcons: false}), 
          React.DOM.div(null, this.props.children)
        )
      );
    }
  });

  



  var PendingConversationView = React.createClass({displayName: 'PendingConversationView',
    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      callState: React.PropTypes.string,
      contact: React.PropTypes.object,
      enableCancelButton: React.PropTypes.bool
    },

    getDefaultProps: function() {
      return {
        enableCancelButton: false
      };
    },

    cancelCall: function() {
      this.props.dispatcher.dispatch(new sharedActions.CancelCall());
    },

    render: function() {
      var cx = React.addons.classSet;
      var pendingStateString;
      if (this.props.callState === CALL_STATES.ALERTING) {
        pendingStateString = mozL10n.get("call_progress_ringing_description");
      } else {
        pendingStateString = mozL10n.get("call_progress_connecting_description");
      }

      var btnCancelStyles = cx({
        "btn": true,
        "btn-cancel": true,
        "disabled": !this.props.enableCancelButton
      });

      return (
        ConversationDetailView({contact: this.props.contact}, 

          React.DOM.p({className: "btn-label"}, pendingStateString), 

          React.DOM.div({className: "btn-group call-action-group"}, 
            React.DOM.button({className: btnCancelStyles, 
                    onClick: this.cancelCall}, 
              mozL10n.get("initiate_call_cancel_button")
            )
          )

        )
      );
    }
  });

  


  var CallFailedView = React.createClass({displayName: 'CallFailedView',
    mixins: [Backbone.Events],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      store: React.PropTypes.instanceOf(
        loop.store.ConversationStore).isRequired,
      contact: React.PropTypes.object.isRequired,
      
      emailLinkError: React.PropTypes.bool,
    },

    getInitialState: function() {
      return {
        emailLinkError: this.props.emailLinkError,
        emailLinkButtonDisabled: false
      };
    },

    componentDidMount: function() {
      this.listenTo(this.props.store, "change:emailLink",
                    this._onEmailLinkReceived);
      this.listenTo(this.props.store, "error:emailLink",
                    this._onEmailLinkError);
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.store);
    },

    _onEmailLinkReceived: function() {
      var emailLink = this.props.store.get("emailLink");
      var contactEmail = _getPreferredEmail(this.props.contact).value;
      sharedUtils.composeCallUrlEmail(emailLink, contactEmail);
      window.close();
    },

    _onEmailLinkError: function() {
      this.setState({
        emailLinkError: true,
        emailLinkButtonDisabled: false
      });
    },

    _renderError: function() {
      if (!this.state.emailLinkError) {
        return;
      }
      return React.DOM.p({className: "error"}, mozL10n.get("unable_retrieve_url"));
    },

    retryCall: function() {
      this.props.dispatcher.dispatch(new sharedActions.RetryCall());
    },

    cancelCall: function() {
      this.props.dispatcher.dispatch(new sharedActions.CancelCall());
    },

    emailLink: function() {
      this.setState({
        emailLinkError: false,
        emailLinkButtonDisabled: true
      });

      this.props.dispatcher.dispatch(new sharedActions.FetchEmailLink());
    },

    render: function() {
      return (
        React.DOM.div({className: "call-window"}, 
          React.DOM.h2(null, mozL10n.get("generic_failure_title")), 

          React.DOM.p({className: "btn-label"}, mozL10n.get("generic_failure_with_reason2")), 

          this._renderError(), 

          React.DOM.div({className: "btn-group call-action-group"}, 
            React.DOM.button({className: "btn btn-cancel", 
                    onClick: this.cancelCall}, 
              mozL10n.get("cancel_button")
            ), 
            React.DOM.button({className: "btn btn-info btn-retry", 
                    onClick: this.retryCall}, 
              mozL10n.get("retry_call_button")
            ), 
            React.DOM.button({className: "btn btn-info btn-email", 
                    onClick: this.emailLink, 
                    disabled: this.state.emailLinkButtonDisabled}, 
              mozL10n.get("share_button2")
            )
          )
        )
      );
    }
  });

  var OngoingConversationView = React.createClass({displayName: 'OngoingConversationView',
    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      video: React.PropTypes.object,
      audio: React.PropTypes.object
    },

    getDefaultProps: function() {
      return {
        video: {enabled: true, visible: true},
        audio: {enabled: true, visible: true}
      };
    },

    componentDidMount: function() {
      





      window.addEventListener('orientationchange', this.updateVideoContainer);
      window.addEventListener('resize', this.updateVideoContainer);

      
      
      
      this.props.dispatcher.dispatch(new sharedActions.SetupStreamElements({
        publisherConfig: this._getPublisherConfig(),
        getLocalElementFunc: this._getElement.bind(this, ".local"),
        getRemoteElementFunc: this._getElement.bind(this, ".remote")
      }));
    },

    componentWillUnmount: function() {
      window.removeEventListener('orientationchange', this.updateVideoContainer);
      window.removeEventListener('resize', this.updateVideoContainer);
    },

    




    _getElement: function(className) {
      return this.getDOMNode().querySelector(className);
    },

    


    _getPublisherConfig: function() {
      
      
      return {
        insertMode: "append",
        width: "100%",
        height: "100%",
        publishVideo: this.props.video.enabled,
        style: {
          audioLevelDisplayMode: "off",
          bugDisplayMode: "off",
          buttonDisplayMode: "off",
          nameDisplayMode: "off",
          videoDisabledDisplayMode: "off"
        }
      }
    },

    



    updateVideoContainer: function() {
      var localStreamParent = this._getElement('.local .OT_publisher');
      var remoteStreamParent = this._getElement('.remote .OT_subscriber');
      if (localStreamParent) {
        localStreamParent.style.width = "100%";
      }
      if (remoteStreamParent) {
        remoteStreamParent.style.height = "100%";
      }
    },

    


    hangup: function() {
      this.props.dispatcher.dispatch(
        new sharedActions.HangupCall());
    },

    





    publishStream: function(type, enabled) {
      this.props.dispatcher.dispatch(
        new sharedActions.SetMute({
          type: type,
          enabled: enabled
        }));
    },

    render: function() {
      var localStreamClasses = React.addons.classSet({
        local: true,
        "local-stream": true,
        "local-stream-audio": !this.props.video.enabled
      });

      return (
        React.DOM.div({className: "video-layout-wrapper"}, 
          React.DOM.div({className: "conversation"}, 
            React.DOM.div({className: "media nested"}, 
              React.DOM.div({className: "video_wrapper remote_wrapper"}, 
                React.DOM.div({className: "video_inner remote"})
              ), 
              React.DOM.div({className: localStreamClasses})
            ), 
            loop.shared.views.ConversationToolbar({
              video: this.props.video, 
              audio: this.props.audio, 
              publishStream: this.publishStream, 
              hangup: this.hangup})
          )
        )
      );
    }
  });

  



  var OutgoingConversationView = React.createClass({displayName: 'OutgoingConversationView',
    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      store: React.PropTypes.instanceOf(
        loop.store.ConversationStore).isRequired
    },

    getInitialState: function() {
      return this.props.store.attributes;
    },

    componentWillMount: function() {
      this.props.store.on("change", function() {
        this.setState(this.props.store.attributes);
      }, this);
    },

    _closeWindow: function() {
      window.close();
    },

    


    _isCancellable: function() {
      return this.state.callState !== CALL_STATES.INIT &&
             this.state.callState !== CALL_STATES.GATHER;
    },

    


    _renderFeedbackView: function() {
      document.title = mozL10n.get("conversation_has_ended");

      
      var feebackAPIBaseUrl = navigator.mozLoop.getLoopCharPref(
        "feedback.baseUrl");

      var appVersionInfo = navigator.mozLoop.appVersionInfo;

      var feedbackClient = new loop.FeedbackAPIClient(feebackAPIBaseUrl, {
        product: navigator.mozLoop.getLoopCharPref("feedback.product"),
        platform: appVersionInfo.OS,
        channel: appVersionInfo.channel,
        version: appVersionInfo.version
      });

      return (
        sharedViews.FeedbackView({
          feedbackApiClient: feedbackClient, 
          onAfterFeedbackReceived: this._closeWindow.bind(this)}
        )
      );
    },

    render: function() {
      switch (this.state.callState) {
        case CALL_STATES.CLOSE: {
          this._closeWindow();
          return null;
        }
        case CALL_STATES.TERMINATED: {
          return (CallFailedView({
            dispatcher: this.props.dispatcher, 
            store: this.props.store, 
            contact: this.state.contact}
          ));
        }
        case CALL_STATES.ONGOING: {
          return (OngoingConversationView({
            dispatcher: this.props.dispatcher, 
            video: {enabled: !this.state.videoMuted}, 
            audio: {enabled: !this.state.audioMuted}}
            )
          );
        }
        case CALL_STATES.FINISHED: {
          return this._renderFeedbackView();
        }
        case CALL_STATES.INIT: {
          
          return null;
        }
        default: {
          return (PendingConversationView({
            dispatcher: this.props.dispatcher, 
            callState: this.state.callState, 
            contact: this.state.contact, 
            enableCancelButton: this._isCancellable()}
          ));
        }
      }
    },
  });

  return {
    PendingConversationView: PendingConversationView,
    CallIdentifierView: CallIdentifierView,
    ConversationDetailView: ConversationDetailView,
    CallFailedView: CallFailedView,
    OngoingConversationView: OngoingConversationView,
    OutgoingConversationView: OutgoingConversationView
  };

})(document.mozL10n || navigator.mozL10n);
