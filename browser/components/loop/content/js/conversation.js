








var loop = loop || {};
loop.conversation = (function(mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views;
  var sharedMixins = loop.shared.mixins;
  var sharedModels = loop.shared.models;
  var OutgoingConversationView = loop.conversationViews.OutgoingConversationView;

  var IncomingCallView = React.createClass({displayName: 'IncomingCallView',
    mixins: [sharedMixins.DropdownMenuMixin],

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
          React.DOM.h2(null, mozL10n.get("incoming_call_title2")), 
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

  





  var IncomingConversationView = React.createClass({displayName: 'IncomingConversationView',
    propTypes: {
      client: React.PropTypes.instanceOf(loop.Client).isRequired,
      conversation: React.PropTypes.instanceOf(sharedModels.ConversationModel)
                         .isRequired,
      sdk: React.PropTypes.object.isRequired
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
          
          document.title = mozL10n.get("incoming_call_title2");

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
            document.title = mozL10n.get("generic_failure_title");
          } else {
            document.title = mozL10n.get("conversation_has_ended");
          }

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
              onAfterFeedbackReceived: this.closeWindow.bind(this)}
            )
          );
        }
        case "close": {
          window.close();
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

      var callData = navigator.mozLoop.getCallData(this.props.conversation.get("callId"));
      if (!callData) {
        
        
        console.error("Failed to get the call data");
        return;
      }
      this.props.conversation.setIncomingSessionData(callData);
      this._setupWebSocket();
    },

    


    accepted: function() {
      this.setState({callStatus: "connected"});
    },

    


    endCall: function() {
      navigator.mozLoop.releaseCallData(this.props.conversation.get("callId"));
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

      if (progressData.reason === "cancel") {
        this._abortIncomingCall();
        return;
      }

      if (progressData.reason === "timeout" &&
          (previousState === "init" || previousState === "alerting")) {
        this._abortIncomingCall();
      }
    },

    



    _abortIncomingCall: function() {
      navigator.mozLoop.stopAlerting();
      this._websocket.close();
      
      
      setTimeout(this.closeWindow, 0);
    },

    closeWindow: function() {
      window.close();
    },

    


    accept: function() {
      navigator.mozLoop.stopAlerting();
      this._websocket.accept();
      this.props.conversation.accepted();
    },

    


    _declineCall: function() {
      this._websocket.decline();
      navigator.mozLoop.releaseCallData(this.props.conversation.get("callId"));
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
      this.props.client.deleteCallUrl(token, function(error) {
        
        
        
        console.log(error);
      });
      this._declineCall();
    },

    


    _handleSessionError: function() {
      
      
      console.error("Failed initiating the call session.");
    },
  });

  



  var ConversationControllerView = React.createClass({displayName: 'ConversationControllerView',
    propTypes: {
      
      client: React.PropTypes.instanceOf(loop.Client).isRequired,
      conversation: React.PropTypes.instanceOf(sharedModels.ConversationModel)
                         .isRequired,
      sdk: React.PropTypes.object.isRequired,

      
      store: React.PropTypes.instanceOf(loop.store.ConversationStore).isRequired,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired
    },

    getInitialState: function() {
      return this.props.store.attributes;
    },

    componentWillMount: function() {
      this.props.store.on("change:outgoing", function() {
        this.setState(this.props.store.attributes);
      }, this);
    },

    render: function() {
      
      if (this.state.outgoing === undefined) {
        return null;
      }

      if (this.state.outgoing) {
        return (OutgoingConversationView({
          store: this.props.store, 
          dispatcher: this.props.dispatcher}
        ));
      }

      return (IncomingConversationView({
        client: this.props.client, 
        conversation: this.props.conversation, 
        sdk: this.props.sdk}
      ));
    }
  });

  


  function init() {
    
    
    mozL10n.initialize(navigator.mozLoop);

    
    
    window.OT.overrideGuidStorage({
      get: function(callback) {
        callback(null, navigator.mozLoop.getLoopCharPref("ot.guid"));
      },
      set: function(guid, callback) {
        navigator.mozLoop.setLoopCharPref("ot.guid", guid);
        callback(null);
      }
    });

    var dispatcher = new loop.Dispatcher();
    var client = new loop.Client();
    var sdkDriver = new loop.OTSdkDriver({
      dispatcher: dispatcher,
      sdk: OT
    });

    var conversationStore = new loop.store.ConversationStore({}, {
      client: client,
      dispatcher: dispatcher,
      sdkDriver: sdkDriver
    });

    
    
    var outgoingEmail = navigator.mozLoop.getLoopCharPref("outgoingemail");

    
    
    var conversation = new sharedModels.ConversationModel(
      {},                
      {sdk: window.OT}   
    );

    
    var helper = new loop.shared.utils.Helper();
    var locationHash = helper.locationHash();
    var callId;
    if (locationHash) {
      callId = locationHash.match(/\#incoming\/(.*)/)[1]
      conversation.set("callId", callId);
    }

    window.addEventListener("unload", function(event) {
      
      navigator.mozLoop.releaseCallData(conversation.get("callId"));
    });

    document.body.classList.add(loop.shared.utils.getTargetPlatform());

    React.renderComponent(ConversationControllerView({
      store: conversationStore, 
      client: client, 
      conversation: conversation, 
      dispatcher: dispatcher, 
      sdk: window.OT}
    ), document.querySelector('#main'));

    dispatcher.dispatch(new loop.shared.actions.GatherCallData({
      callId: callId,
      calleeId: outgoingEmail
    }));
  }

  return {
    ConversationControllerView: ConversationControllerView,
    IncomingConversationView: IncomingConversationView,
    IncomingCallView: IncomingCallView,
    init: init
  };
})(document.mozL10n);

document.addEventListener('DOMContentLoaded', loop.conversation.init);
