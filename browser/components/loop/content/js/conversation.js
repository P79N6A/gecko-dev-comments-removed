








var loop = loop || {};
loop.conversation = (function(OT, mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views,
      
      __ = mozL10n.get;

  



  var router;

  var IncomingCallView = React.createClass({displayName: 'IncomingCallView',

    propTypes: {
      model: React.PropTypes.object.isRequired
    },

    getInitialState: function() {
      return {showDeclineMenu: false};
    },

    componentDidMount: function() {
      window.addEventListener("click", this.clickHandler);
      window.addEventListener("blur", this._hideDeclineMenu);
    },

    componentWillUnmount: function() {
      window.removeEventListener("click", this.clickHandler);
      window.removeEventListener("blur", this._hideDeclineMenu);
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

    _toggleDeclineMenu: function() {
      var currentState = this.state.showDeclineMenu;
      this.setState({showDeclineMenu: !currentState});
    },

    _hideDeclineMenu: function() {
      this.setState({showDeclineMenu: false});
    },

    render: function() {
      
      var btnClassAccept = "btn btn-accept";
      var btnClassDecline = "btn btn-error btn-decline";
      var conversationPanelClass = "incoming-call";
      var dropdownMenuClassesDecline = React.addons.classSet({
        "native-dropdown-menu": true,
        "conversation-window-dropdown": true,
        "visually-hidden": !this.state.showDeclineMenu
      });
      return (
        React.DOM.div({className: conversationPanelClass}, 
          React.DOM.h2(null, __("incoming_call")), 
          React.DOM.div({className: "btn-group incoming-call-action-group"}, 

            React.DOM.div({className: "fx-embedded-incoming-call-button-spacer"}), 

            React.DOM.div({className: "btn-chevron-menu-group"}, 
              React.DOM.div({className: "btn-group-chevron"}, 
                React.DOM.div({className: "btn-group"}, 

                  React.DOM.button({className: btnClassDecline, 
                          onClick: this._handleDecline}, 
                    __("incoming_call_cancel_button")
                  ), 
                  React.DOM.div({className: "btn-chevron", 
                       onClick: this._toggleDeclineMenu}
                  )
                ), 

                React.DOM.ul({className: dropdownMenuClassesDecline}, 
                  React.DOM.li({className: "btn-block", onClick: this._handleDeclineBlock}, 
                    __("incoming_call_cancel_and_block_button")
                  )
                )

              )
            ), 

            React.DOM.div({className: "fx-embedded-incoming-call-button-spacer"}), 

            React.DOM.div({className: "btn-chevron-menu-group"}, 
              React.DOM.div({className: "btn-group"}, 
                React.DOM.button({className: btnClassAccept, 
                        onClick: this._handleAccept("audio-video")}, 
                  React.DOM.span({className: "fx-embedded-answer-btn-text"}, 
                    __("incoming_call_accept_button")
                  ), 
                  React.DOM.span({className: "fx-embedded-btn-icon-video"}
                  )
                ), 
                React.DOM.div({className: "call-audio-only", 
                     onClick: this._handleAccept("audio"), 
                     title: __("incoming_call_accept_audio_only_tooltip")}
                )
              )
            ), 

            React.DOM.div({className: "fx-embedded-incoming-call-button-spacer"})

          )
        )
      );
      
    }
  });

  








  var ConversationRouter = loop.desktopRouter.DesktopConversationRouter.extend({
    routes: {
      "incoming/:version": "incoming",
      "call/accept": "accept",
      "call/decline": "decline",
      "call/ongoing": "conversation",
      "call/declineAndBlock": "declineAndBlock",
      "call/feedback": "feedback"
    },

    


    startCall: function() {
      this.navigate("call/ongoing", {trigger: true});
    },

    


    endCall: function() {
      this.navigate("call/feedback", {trigger: true});
    },

    





    incoming: function(loopVersion) {
      navigator.mozLoop.startAlerting();
      this._conversation.set({loopVersion: loopVersion});
      this._conversation.once("accept", function() {
        this.navigate("call/accept", {trigger: true});
      }.bind(this));
      this._conversation.once("decline", function() {
        this.navigate("call/decline", {trigger: true});
      }.bind(this));
      this._conversation.once("declineAndBlock", function() {
        this.navigate("call/declineAndBlock", {trigger: true});
      }.bind(this));
      this._conversation.once("call:incoming", this.startCall, this);
      this._conversation.once("change:publishedStream", this._checkConnected, this);
      this._conversation.once("change:subscribedStream", this._checkConnected, this);

      this._client.requestCallsInfo(loopVersion, function(err, sessionData) {
        if (err) {
          console.error("Failed to get the sessionData", err);
          
          
          this._notifier.errorL10n("cannot_start_call_session_not_ready");
          return;
        }

        
        
        
        
        
        this._conversation.setIncomingSessionData(sessionData[0]);

        this._setupWebSocketAndCallView();
      }.bind(this));
    },

    



    _setupWebSocketAndCallView: function() {
      this._websocket = new loop.CallConnectionWebSocket({
        url: this._conversation.get("progressURL"),
        websocketToken: this._conversation.get("websocketToken"),
        callId: this._conversation.get("callId"),
      });
      this._websocket.promiseConnect().then(function() {
        this.loadReactComponent(loop.conversation.IncomingCallView({
          model: this._conversation,
          video: {enabled: this._conversation.hasVideoStream("incoming")}
        }));
      }.bind(this), function() {
        this._handleSessionError();
        return;
      }.bind(this));
    },

    



    _checkConnected: function() {
      
      
      if (this._conversation.streamsConnected()) {
        this._websocket.mediaUp();
      }
    },

    


    accept: function() {
      navigator.mozLoop.stopAlerting();
      this._websocket.accept();
      this._conversation.incoming();
    },

    


    _declineCall: function() {
      this._websocket.decline();
      
      
      
      
      
      setTimeout(window.close, 0);
    },

    


    decline: function() {
      navigator.mozLoop.stopAlerting();
      this._declineCall();
    },

    





    declineAndBlock: function() {
      navigator.mozLoop.stopAlerting();
      var token = this._conversation.get("callToken");
      this._client.deleteCallUrl(token, function(error) {
        
        
        
        console.log(error);
      });
      this._declineCall();
    },

    



    conversation: function() {
      if (!this._conversation.isSessionReady()) {
        console.error("Error: navigated to conversation route without " +
          "the start route to initialise the call first");
        this._handleSessionError();
        return;
      }

      var callType = this._conversation.get("selectedCallType");
      var videoStream = callType === "audio" ? false : true;

      
      this.loadReactComponent(sharedViews.ConversationView({
        sdk: OT,
        model: this._conversation,
        video: {enabled: videoStream}
      }));
    },

    


    _handleSessionError: function() {
      
      
      this._notifier.errorL10n("cannot_start_call_session_not_ready");
    },

    


    feedback: function() {
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

      this.loadReactComponent(sharedViews.FeedbackView({
        feedbackApiClient: feedbackClient
      }));
    }
  });

  


  function init() {
    
    
    mozL10n.initialize(navigator.mozLoop);

    document.title = mozL10n.get("incoming_call_title");

    document.body.classList.add(loop.shared.utils.getTargetPlatform());

    var client = new loop.Client();
    router = new ConversationRouter({
      client: client,
      conversation: new loop.shared.models.ConversationModel(
        {},         
        {sdk: OT}), 
      notifier: new sharedViews.NotificationListView({el: "#messages"})
    });
    Backbone.history.start();
  }

  return {
    ConversationRouter: ConversationRouter,
    IncomingCallView: IncomingCallView,
    init: init
  };
})(window.OT, document.mozL10n);
