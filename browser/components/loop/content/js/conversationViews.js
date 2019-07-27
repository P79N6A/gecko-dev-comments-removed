







var loop = loop || {};
loop.conversationViews = (function(mozL10n) {

  






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
      callState: React.PropTypes.string,
      calleeId: React.PropTypes.string,
    },

    render: function() {
      var pendingStateString;
      if (this.props.callState === "ringing") {
        pendingStateString = mozL10n.get("call_progress_pending_description");
      } else {
        pendingStateString = mozL10n.get("call_progress_connecting_description");
      }

      return (
        ConversationDetailView({calleeId: this.props.calleeId}, 

          React.DOM.p({className: "btn-label"}, pendingStateString), 

          React.DOM.div({className: "btn-group call-action-group"}, 
            React.DOM.div({className: "fx-embedded-call-button-spacer"}), 
              React.DOM.button({className: "btn btn-cancel"}, 
                mozL10n.get("initiate_call_cancel_button")
              ), 
            React.DOM.div({className: "fx-embedded-call-button-spacer"})
          )

        )
      );
    }
  });

  



  var OutgoingConversationView = React.createClass({displayName: 'OutgoingConversationView',
    propTypes: {
      store: React.PropTypes.instanceOf(
        loop.ConversationStore).isRequired
    },

    getInitialState: function() {
      return this.props.store.attributes;
    },

    render: function() {
      return (PendingConversationView({
        callState: this.state.callState, 
        calleeId: this.state.calleeId}
      ))
    }
  });

  return {
    PendingConversationView: PendingConversationView,
    ConversationDetailView: ConversationDetailView,
    OutgoingConversationView: OutgoingConversationView
  };

})(document.mozL10n || navigator.mozL10n);
