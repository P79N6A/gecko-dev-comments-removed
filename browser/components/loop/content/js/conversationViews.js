







var loop = loop || {};
loop.conversationViews = (function(mozL10n) {

  var CALL_STATES = loop.store.CALL_STATES;
  var CALL_TYPES = loop.shared.utils.CALL_TYPES;
  var sharedActions = loop.shared.actions;
  var sharedViews = loop.shared.views;

  






  var ConversationDetailView = React.createClass({displayName: 'ConversationDetailView',
    propTypes: {
      calleeId: React.PropTypes.string,
    },

    render: function() {
      document.title = this.props.calleeId;

      return (
        React.DOM.div({className: "call-window"}, 
          React.DOM.h2(null, this.props.calleeId), 
          React.DOM.div(null, this.props.children)
        )
      );
    }
  });

  



  var PendingConversationView = React.createClass({displayName: 'PendingConversationView',
    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      callState: React.PropTypes.string,
      calleeId: React.PropTypes.string,
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
        ConversationDetailView({calleeId: this.props.calleeId}, 

          React.DOM.p({className: "btn-label"}, pendingStateString), 

          React.DOM.div({className: "btn-group call-action-group"}, 
            React.DOM.div({className: "fx-embedded-call-button-spacer"}), 
              React.DOM.button({className: btnCancelStyles, 
                      onClick: this.cancelCall}, 
                mozL10n.get("initiate_call_cancel_button")
              ), 
            React.DOM.div({className: "fx-embedded-call-button-spacer"})
          )

        )
      );
    }
  });

  


  var CallFailedView = React.createClass({displayName: 'CallFailedView',
    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired
    },

    retryCall: function() {
      this.props.dispatcher.dispatch(new sharedActions.RetryCall());
    },

    cancelCall: function() {
      this.props.dispatcher.dispatch(new sharedActions.CancelCall());
    },

    render: function() {
      return (
        React.DOM.div({className: "call-window"}, 
          React.DOM.h2(null, mozL10n.get("generic_failure_title")), 

          React.DOM.p({className: "btn-label"}, mozL10n.get("generic_failure_no_reason2")), 

          React.DOM.div({className: "btn-group call-action-group"}, 
            React.DOM.div({className: "fx-embedded-call-button-spacer"}), 
              React.DOM.button({className: "btn btn-accept btn-retry", 
                      onClick: this.retryCall}, 
                mozL10n.get("retry_call_button")
              ), 
            React.DOM.div({className: "fx-embedded-call-button-spacer"}), 
              React.DOM.button({className: "btn btn-cancel", 
                      onClick: this.cancelCall}, 
                mozL10n.get("cancel_button")
              ), 
            React.DOM.div({className: "fx-embedded-call-button-spacer"})
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
    },

    componentWillUnmount: function() {
      window.removeEventListener('orientationchange', this.updateVideoContainer);
      window.removeEventListener('resize', this.updateVideoContainer);
    },

    updateVideoContainer: function() {
      var localStreamParent = document.querySelector('.local .OT_publisher');
      var remoteStreamParent = document.querySelector('.remote .OT_subscriber');
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
            dispatcher: this.props.dispatcher}
          ));
        }
        case CALL_STATES.ONGOING: {
          return (OngoingConversationView({
            dispatcher: this.props.dispatcher, 
            video: {enabled: this.state.callType === CALL_TYPES.AUDIO_VIDEO}}
            )
          );
        }
        case CALL_STATES.FINISHED: {
          return this._renderFeedbackView();
        }
        default: {
          return (PendingConversationView({
            dispatcher: this.props.dispatcher, 
            callState: this.state.callState, 
            calleeId: this.state.calleeId, 
            enableCancelButton: this._isCancellable()}
          ))
        }
      }
    },
  });

  return {
    PendingConversationView: PendingConversationView,
    ConversationDetailView: ConversationDetailView,
    CallFailedView: CallFailedView,
    OngoingConversationView: OngoingConversationView,
    OutgoingConversationView: OutgoingConversationView
  };

})(document.mozL10n || navigator.mozL10n);
