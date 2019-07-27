/** @jsx React.DOM */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global loop:true, React */

var loop = loop || {};
loop.conversationViews = (function(mozL10n) {

  var CALL_STATES = loop.store.CALL_STATES;

  /**
   * Displays details of the incoming/outgoing conversation
   * (name, link, audio/video type etc).
   *
   * Allows the view to be extended with different buttons and progress
   * via children properties.
   */
  var ConversationDetailView = React.createClass({
    propTypes: {
      calleeId: React.PropTypes.string,
    },

    render: function() {
      document.title = this.props.calleeId;

      return (
        <div className="call-window">
          <h2>{this.props.calleeId}</h2>
          <div>{this.props.children}</div>
        </div>
      );
    }
  });

  /**
   * View for pending conversations. Displays a cancel button and appropriate
   * pending/ringing strings.
   */
  var PendingConversationView = React.createClass({
    propTypes: {
      callState: React.PropTypes.string,
      calleeId: React.PropTypes.string,
    },

    render: function() {
      var pendingStateString;
      if (this.props.callState === CALL_STATES.ALERTING) {
        pendingStateString = mozL10n.get("call_progress_ringing_description");
      } else {
        pendingStateString = mozL10n.get("call_progress_connecting_description");
      }

      return (
        <ConversationDetailView calleeId={this.props.calleeId}>

          <p className="btn-label">{pendingStateString}</p>

          <div className="btn-group call-action-group">
            <div className="fx-embedded-call-button-spacer"></div>
              <button className="btn btn-cancel">
                {mozL10n.get("initiate_call_cancel_button")}
              </button>
            <div className="fx-embedded-call-button-spacer"></div>
          </div>

        </ConversationDetailView>
      );
    }
  });

  /**
   * Call failed view. Displayed when a call fails.
   */
  var CallFailedView = React.createClass({
    render: function() {
      return (
        <div className="call-window">
          <h2>{mozL10n.get("generic_failure_title")}</h2>
        </div>
      );
    }
  });

  /**
   * Master View Controller for outgoing calls. This manages
   * the different views that need displaying.
   */
  var OutgoingConversationView = React.createClass({
    propTypes: {
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

    render: function() {
      if (this.state.callState === CALL_STATES.TERMINATED) {
        return (<CallFailedView />);
      }

      return (<PendingConversationView
        callState={this.state.callState}
        calleeId={this.state.calleeId}
      />)
    }
  });

  return {
    PendingConversationView: PendingConversationView,
    ConversationDetailView: ConversationDetailView,
    CallFailedView: CallFailedView,
    OutgoingConversationView: OutgoingConversationView
  };

})(document.mozL10n || navigator.mozL10n);
