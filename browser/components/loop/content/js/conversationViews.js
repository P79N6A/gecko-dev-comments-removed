







var loop = loop || {};
loop.conversationViews = (function(mozL10n) {

  var CALL_STATES = loop.store.CALL_STATES;
  var CALL_TYPES = loop.shared.utils.CALL_TYPES;
  var sharedActions = loop.shared.actions;
  var sharedUtils = loop.shared.utils;
  var sharedViews = loop.shared.views;
  var sharedMixins = loop.shared.mixins;
  var sharedModels = loop.shared.models;

  
  
  
  function _getPreferredEmail(contact) {
    
    if (!contact.email || contact.email.length === 0) {
      return { value: "" };
    }
    return contact.email.find(e => e.pref) || contact.email[0];
  }

  function _getContactDisplayName(contact) {
    if (contact.name && contact.name[0]) {
      return contact.name[0];
    }
    return _getPreferredEmail(contact).value;
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
      var contactName = _getContactDisplayName(this.props.contact);

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

  
  var EMAIL_OR_PHONE_RE = /^(:?\S+@\S+|\+\d+)$/;

  var IncomingCallView = React.createClass({displayName: 'IncomingCallView',
    mixins: [sharedMixins.DropdownMenuMixin, sharedMixins.AudioMixin],

    propTypes: {
      model: React.PropTypes.object.isRequired,
      video: React.PropTypes.bool.isRequired
    },

    getDefaultProps: function() {
      return {
        showMenu: false,
        video: true
      };
    },

    clickHandler: function(e) {
      var target = e.target;
      if (!target.classList.contains('btn-chevron')) {
        this._hideDeclineMenu();
      }
    },

    _handleAccept: function(callType) {
      return function() {
        this.props.model.set("selectedCallType", callType);
        this.props.model.trigger("accept");
      }.bind(this);
    },

    _handleDecline: function() {
      this.props.model.trigger("decline");
    },

    _handleDeclineBlock: function(e) {
      this.props.model.trigger("declineAndBlock");
      

      return false;
    },

    




    _answerModeProps: function() {
      var videoButton = {
        handler: this._handleAccept("audio-video"),
        className: "fx-embedded-btn-icon-video",
        tooltip: "incoming_call_accept_audio_video_tooltip"
      };
      var audioButton = {
        handler: this._handleAccept("audio"),
        className: "fx-embedded-btn-audio-small",
        tooltip: "incoming_call_accept_audio_only_tooltip"
      };
      var props = {};
      props.primary = videoButton;
      props.secondary = audioButton;

      
      if (!this.props.video) {
        audioButton.className = "fx-embedded-btn-icon-audio";
        videoButton.className = "fx-embedded-btn-video-small";
        props.primary = audioButton;
        props.secondary = videoButton;
      }

      return props;
    },

    render: function() {
      
      var dropdownMenuClassesDecline = React.addons.classSet({
        "native-dropdown-menu": true,
        "conversation-window-dropdown": true,
        "visually-hidden": !this.state.showMenu
      });

      return (
        React.DOM.div({className: "call-window"}, 
          CallIdentifierView({video: this.props.video, 
            peerIdentifier: this.props.model.getCallIdentifier(), 
            urlCreationDate: this.props.model.get("urlCreationDate"), 
            showIcons: true}), 

          React.DOM.div({className: "btn-group call-action-group"}, 

            React.DOM.div({className: "fx-embedded-call-button-spacer"}), 

            React.DOM.div({className: "btn-chevron-menu-group"}, 
              React.DOM.div({className: "btn-group-chevron"}, 
                React.DOM.div({className: "btn-group"}, 

                  React.DOM.button({className: "btn btn-decline", 
                          onClick: this._handleDecline}, 
                    mozL10n.get("incoming_call_cancel_button")
                  ), 
                  React.DOM.div({className: "btn-chevron", onClick: this.toggleDropdownMenu})
                ), 

                React.DOM.ul({className: dropdownMenuClassesDecline}, 
                  React.DOM.li({className: "btn-block", onClick: this._handleDeclineBlock}, 
                    mozL10n.get("incoming_call_cancel_and_block_button")
                  )
                )

              )
            ), 

            React.DOM.div({className: "fx-embedded-call-button-spacer"}), 

            AcceptCallButton({mode: this._answerModeProps()}), 

            React.DOM.div({className: "fx-embedded-call-button-spacer"})

          )
        )
      );
      
    }
  });

  



  var AcceptCallButton = React.createClass({displayName: 'AcceptCallButton',

    propTypes: {
      mode: React.PropTypes.object.isRequired,
    },

    render: function() {
      var mode = this.props.mode;
      return (
        
        React.DOM.div({className: "btn-chevron-menu-group"}, 
          React.DOM.div({className: "btn-group"}, 
            React.DOM.button({className: "btn btn-accept", 
                    onClick: mode.primary.handler, 
                    title: mozL10n.get(mode.primary.tooltip)}, 
              React.DOM.span({className: "fx-embedded-answer-btn-text"}, 
                mozL10n.get("incoming_call_accept_button")
              ), 
              React.DOM.span({className: mode.primary.className})
            ), 
            React.DOM.div({className: mode.secondary.className, 
                 onClick: mode.secondary.handler, 
                 title: mozL10n.get(mode.secondary.tooltip)}
            )
          )
        )
        
      );
    }
  });

  





  var GenericFailureView = React.createClass({displayName: 'GenericFailureView',
    mixins: [sharedMixins.AudioMixin],

    propTypes: {
      cancelCall: React.PropTypes.func.isRequired
    },

    componentDidMount: function() {
      this.play("failure");
    },

    render: function() {
      document.title = mozL10n.get("generic_failure_title");

      return (
        React.DOM.div({className: "call-window"}, 
          React.DOM.h2(null, mozL10n.get("generic_failure_title")), 

          React.DOM.div({className: "btn-group call-action-group"}, 
            React.DOM.button({className: "btn btn-cancel", 
                    onClick: this.props.cancelCall}, 
              mozL10n.get("cancel_button")
            )
          )
        )
      );
    }
  });

  





  var IncomingConversationView = React.createClass({displayName: 'IncomingConversationView',
    mixins: [sharedMixins.AudioMixin, sharedMixins.WindowCloseMixin],

    propTypes: {
      client: React.PropTypes.instanceOf(loop.Client).isRequired,
      conversation: React.PropTypes.instanceOf(sharedModels.ConversationModel)
                         .isRequired,
      sdk: React.PropTypes.object.isRequired,
      conversationAppStore: React.PropTypes.instanceOf(
        loop.store.ConversationAppStore).isRequired,
      feedbackStore:
        React.PropTypes.instanceOf(loop.store.FeedbackStore).isRequired
    },

    getInitialState: function() {
      return {
        callFailed: false, 
        callStatus: "start"
      };
    },

    componentDidMount: function() {
      this.props.conversation.on("accept", this.accept, this);
      this.props.conversation.on("decline", this.decline, this);
      this.props.conversation.on("declineAndBlock", this.declineAndBlock, this);
      this.props.conversation.on("call:accepted", this.accepted, this);
      this.props.conversation.on("change:publishedStream", this._checkConnected, this);
      this.props.conversation.on("change:subscribedStream", this._checkConnected, this);
      this.props.conversation.on("session:ended", this.endCall, this);
      this.props.conversation.on("session:peer-hungup", this._onPeerHungup, this);
      this.props.conversation.on("session:network-disconnected", this._onNetworkDisconnected, this);
      this.props.conversation.on("session:connection-error", this._notifyError, this);

      this.setupIncomingCall();
    },

    componentDidUnmount: function() {
      this.props.conversation.off(null, null, this);
    },

    render: function() {
      switch (this.state.callStatus) {
        case "start": {
          document.title = mozL10n.get("incoming_call_title2");

          
          
          return null;
        }
        case "incoming": {
          document.title = mozL10n.get("incoming_call_title2");

          return (
            IncomingCallView({
              model: this.props.conversation, 
              video: this.props.conversation.hasVideoStream("incoming")}
            )
          );
        }
        case "connected": {
          document.title = this.props.conversation.getCallIdentifier();

          var callType = this.props.conversation.get("selectedCallType");

          return (
            sharedViews.ConversationView({
              initiate: true, 
              sdk: this.props.sdk, 
              model: this.props.conversation, 
              video: {enabled: callType !== "audio"}}
            )
          );
        }
        case "end": {
          
          if (this.state.callFailed) {
            return GenericFailureView({
              cancelCall: this.closeWindow.bind(this)}
            );
          }

          document.title = mozL10n.get("conversation_has_ended");

          this.play("terminated");

          return (
            sharedViews.FeedbackView({
              feedbackStore: this.props.feedbackStore, 
              onAfterFeedbackReceived: this.closeWindow.bind(this)}
            )
          );
        }
        case "close": {
          this.closeWindow();
          return (React.DOM.div(null));
        }
      }
    },

    



    _notifyError: function(error) {
      
      
      console.error(error);
      this.setState({callFailed: true, callStatus: "end"});
    },

    





    _onPeerHungup: function() {
      this.setState({callFailed: false, callStatus: "end"});
    },

    


    _onNetworkDisconnected: function() {
      
      
      this.setState({callFailed: true, callStatus: "end"});
    },

    


    setupIncomingCall: function() {
      navigator.mozLoop.startAlerting();

      
      var callData = this.props.conversationAppStore.getStoreState().windowData;

      this.props.conversation.setIncomingSessionData(callData);
      this._setupWebSocket();
    },

    


    accepted: function() {
      this.setState({callStatus: "connected"});
    },

    


    endCall: function() {
      navigator.mozLoop.calls.clearCallInProgress(
        this.props.conversation.get("windowId"));
      this.setState({callStatus: "end"});
    },

    



    _setupWebSocket: function() {
      this._websocket = new loop.CallConnectionWebSocket({
        url: this.props.conversation.get("progressURL"),
        websocketToken: this.props.conversation.get("websocketToken"),
        callId: this.props.conversation.get("callId"),
      });
      this._websocket.promiseConnect().then(function(progressStatus) {
        this.setState({
          callStatus: progressStatus === "terminated" ? "close" : "incoming"
        });
      }.bind(this), function() {
        this._handleSessionError();
        return;
      }.bind(this));

      this._websocket.on("progress", this._handleWebSocketProgress, this);
    },

    



    _checkConnected: function() {
      
      
      if (this.props.conversation.streamsConnected()) {
        this._websocket.mediaUp();
      }
    },

    







    _handleWebSocketProgress: function(progressData, previousState) {
      
      if (progressData.state !== "terminated")
        return;

      
      
      
      
      navigator.mozLoop.stopAlerting();

      
      
      
      
      
      
      
      if (previousState === "init" || previousState === "alerting") {
        this._abortIncomingCall();
      } else {
        this.setState({callFailed: true, callStatus: "end"});
      }

    },

    



    _abortIncomingCall: function() {
      this._websocket.close();
      
      
      setTimeout(this.closeWindow, 0);
    },

    


    accept: function() {
      navigator.mozLoop.stopAlerting();
      this._websocket.accept();
      this.props.conversation.accepted();
    },

    


    _declineCall: function() {
      this._websocket.decline();
      navigator.mozLoop.calls.clearCallInProgress(
        this.props.conversation.get("windowId"));
      this._websocket.close();
      
      
      setTimeout(this.closeWindow, 0);
    },

    


    decline: function() {
      navigator.mozLoop.stopAlerting();
      this._declineCall();
    },

    





    declineAndBlock: function() {
      navigator.mozLoop.stopAlerting();
      var token = this.props.conversation.get("callToken");
      var callerId = this.props.conversation.get("callerId");

      
      if (callerId && EMAIL_OR_PHONE_RE.test(callerId)) {
        navigator.mozLoop.calls.blockDirectCaller(callerId, function(err) {
          
          
          
          console.log(err.fileName + ":" + err.lineNumber + ": " + err.message);
        });
      } else {
        this.props.client.deleteCallUrl(token,
          this.props.conversation.get("sessionType"),
          function(error) {
            
            
            
            console.log(error);
          });
      }

      this._declineCall();
    },

    


    _handleSessionError: function() {
      
      
      console.error("Failed initiating the call session.");
    },
  });

  



  var PendingConversationView = React.createClass({displayName: 'PendingConversationView',
    mixins: [sharedMixins.AudioMixin],

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

    componentDidMount: function() {
      this.play("ringtone", {loop: true});
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
    mixins: [
      Backbone.Events,
      sharedMixins.AudioMixin,
      sharedMixins.WindowCloseMixin
    ],

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
      this.play("failure");
      this.listenTo(this.props.store, "change:emailLink",
                    this._onEmailLinkReceived);
      this.listenTo(this.props.store, "error:emailLink",
                    this._onEmailLinkError);
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.store);
    },

    _onEmailLinkReceived: function() {
      var emailLink = this.props.store.getStoreState("emailLink");
      var contactEmail = _getPreferredEmail(this.props.contact).value;
      sharedUtils.composeCallUrlEmail(emailLink, contactEmail);
      this.closeWindow();
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

      this.props.dispatcher.dispatch(new sharedActions.FetchRoomEmailLink({
        roomOwner: navigator.mozLoop.userProfile.email,
        roomName: _getContactDisplayName(this.props.contact)
      }));
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
      };
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
    mixins: [
      sharedMixins.AudioMixin,
      Backbone.Events
    ],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      store: React.PropTypes.instanceOf(
        loop.store.ConversationStore).isRequired,
      feedbackStore: React.PropTypes.instanceOf(loop.store.FeedbackStore)
    },

    getInitialState: function() {
      return this.props.store.getStoreState();
    },

    componentWillMount: function() {
      this.listenTo(this.props.store, "change", function() {
        this.setState(this.props.store.getStoreState());
      }, this);
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.store, "change", function() {
        this.setState(this.props.store.getStoreState());
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

      return (
        sharedViews.FeedbackView({
          feedbackStore: this.props.feedbackStore, 
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
          this.play("terminated");
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
    GenericFailureView: GenericFailureView,
    IncomingCallView: IncomingCallView,
    IncomingConversationView: IncomingConversationView,
    OngoingConversationView: OngoingConversationView,
    OutgoingConversationView: OutgoingConversationView
  };

})(document.mozL10n || navigator.mozL10n);
