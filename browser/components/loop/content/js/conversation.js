








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
      window.addEventListener('click', this.clickHandler);
      window.addEventListener('blur', this._hideDeclineMenu);
    },

    componentWillUnmount: function() {
      window.removeEventListener('click', this.clickHandler);
      window.removeEventListener('blur', this._hideDeclineMenu);
    },

    clickHandler: function(e) {
      var target = e.target;
      if (!target.classList.contains('btn-chevron')) {
        this._hideDeclineMenu();
      }
    },

    _handleAccept: function() {
      this.props.model.trigger("accept");
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
      
      var btnClassAccept = "btn btn-success btn-accept";
      var btnClassBlock = "btn btn-error btn-block";
      var btnClassDecline = "btn btn-error btn-decline";
      var conversationPanelClass = "incoming-call " +
                                  loop.shared.utils.getTargetPlatform();
      var cx = React.addons.classSet;
      var declineDropdownMenuClasses = cx({
        "native-dropdown-menu": true,
        "decline-block-menu": true,
        "visually-hidden": !this.state.showDeclineMenu
      });
      return (
        React.DOM.div({className: conversationPanelClass}, 
          React.DOM.h2(null, __("incoming_call")), 
          React.DOM.div({className: "button-group incoming-call-action-group"}, 
            React.DOM.div({className: "button-chevron-menu-group"}, 
              React.DOM.div({className: "button-group-chevron"}, 
                React.DOM.div({className: "button-group"}, 
                  React.DOM.button({className: btnClassDecline, onClick: this._handleDecline}, 
                    __("incoming_call_decline_button")
                  ), 
                  React.DOM.div({className: "btn-chevron", 
                    onClick: this._toggleDeclineMenu}
                  )
                ), 
                React.DOM.ul({className: declineDropdownMenuClasses}, 
                  React.DOM.li({className: "btn-block", onClick: this._handleDeclineBlock}, 
                    __("incoming_call_decline_and_block_button")
                  )
                )
              )
            ), 
            React.DOM.button({className: btnClassAccept, onClick: this._handleAccept}, 
              __("incoming_call_answer_button")
            )
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
      this._conversation.once("accept", () => {
        this.navigate("call/accept", {trigger: true});
      });
      this._conversation.once("decline", () => {
        this.navigate("call/decline", {trigger: true});
      });
      this._conversation.once("declineAndBlock", () => {
        this.navigate("call/declineAndBlock", {trigger: true});
      });
      this._conversation.once("call:incoming", this.startCall, this);
      this._client.requestCallsInfo(loopVersion, (err, sessionData) => {
        if (err) {
          console.error("Failed to get the sessionData", err);
          
          
          this._notifier.errorL10n("cannot_start_call_session_not_ready");
          return;
        }
        
        
        
        
        
        this._conversation.setSessionData(sessionData[0]);
        this.loadReactComponent(loop.conversation.IncomingCallView({
          model: this._conversation
        }));
      });
    },

    


    accept: function() {
      navigator.mozLoop.stopAlerting();
      this._conversation.incoming();
    },

    


    decline: function() {
      navigator.mozLoop.stopAlerting();
      
      window.close();
    },

    





    declineAndBlock: function() {
      navigator.mozLoop.stopAlerting();
      var token = navigator.mozLoop.getLoopCharPref('loopToken');
      this._client.deleteCallUrl(token, function(error) {
        
        
        
        console.log(error);
      });
      window.close();
    },

    



    conversation: function() {
      if (!this._conversation.isSessionReady()) {
        console.error("Error: navigated to conversation route without " +
          "the start route to initialise the call first");
        this._notifier.errorL10n("cannot_start_call_session_not_ready");
        return;
      }

      
      this.loadReactComponent(sharedViews.ConversationView({
        sdk: OT,
        model: this._conversation
      }));
    },

    


    feedback: function() {
      document.title = mozL10n.get("call_has_ended");

      this.loadReactComponent(sharedViews.FeedbackView({
        feedbackApiClient: new loop.FeedbackAPIClient({
          baseUrl: navigator.mozLoop.getLoopCharPref("feedback.baseUrl"),
          product: navigator.mozLoop.getLoopCharPref("feedback.product")
        })
      }));
    }
  });

  


  function init() {
    
    
    mozL10n.initialize(navigator.mozLoop);

    document.title = mozL10n.get("incoming_call_title");

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
