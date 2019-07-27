/** @jsx React.DOM */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global loop:true, React */
/* jshint newcap:false, maxlen:false */

var loop = loop || {};
loop.standaloneRoomViews = (function(mozL10n) {
  "use strict";

  var FAILURE_REASONS = loop.shared.utils.FAILURE_REASONS;
  var ROOM_STATES = loop.store.ROOM_STATES;
  var sharedActions = loop.shared.actions;
  var sharedMixins = loop.shared.mixins;
  var sharedViews = loop.shared.views;

  var StandaloneRoomInfoArea = React.createClass({
    propTypes: {
      helper: React.PropTypes.instanceOf(loop.shared.utils.Helper).isRequired
    },

    _renderCallToActionLink: function() {
      if (this.props.helper.isFirefox(navigator.userAgent)) {
        return (
          <a href={loop.config.learnMoreUrl} className="btn btn-info">
            {mozL10n.get("rooms_room_full_call_to_action_label", {
              clientShortname: mozL10n.get("clientShortname2")
            })}
          </a>
        );
      }
      return (
        <a href={loop.config.brandWebsiteUrl} className="btn btn-info">
          {mozL10n.get("rooms_room_full_call_to_action_nonFx_label", {
            brandShortname: mozL10n.get("brandShortname")
          })}
        </a>
      );
    },

    /**
     * @return String An appropriate string according to the failureReason.
     */
    _getFailureString: function() {
      switch(this.props.failureReason) {
        case FAILURE_REASONS.MEDIA_DENIED:
          return mozL10n.get("rooms_media_denied_message");
        case FAILURE_REASONS.EXPIRED_OR_INVALID:
          return mozL10n.get("rooms_unavailable_notification_message");
        default:
          return mozL10n.get("status_error");
      };
    },

    _renderContent: function() {
      switch(this.props.roomState) {
        case ROOM_STATES.INIT:
        case ROOM_STATES.READY: {
          return (
            <button className="btn btn-join btn-info"
                    onClick={this.props.joinRoom}>
              {mozL10n.get("rooms_room_join_label")}
            </button>
          );
        }
        case ROOM_STATES.JOINED:
        case ROOM_STATES.SESSION_CONNECTED: {
          return (
            <p className="empty-room-message">
              {mozL10n.get("rooms_only_occupant_label")}
            </p>
          );
        }
        case ROOM_STATES.FULL:
          return (
            <div>
              <p className="full-room-message">
                {mozL10n.get("rooms_room_full_label")}
              </p>
              <p>{this._renderCallToActionLink()}</p>
            </div>
          );
        case ROOM_STATES.FAILED:
          return (
            <p className="failed-room-message">
              {this._getFailureString()}
            </p>
          );
        default:
          return null;
      }
    },

    render: function() {
      return (
        <div className="room-inner-info-area">
          {this._renderContent()}
        </div>
      );
    }
  });

  var StandaloneRoomHeader = React.createClass({
    render: function() {
      return (
        <header>
          <h1>{mozL10n.get("clientShortname2")}</h1>
          <a target="_blank" href={loop.config.roomsSupportUrl}>
            <i className="icon icon-help"></i>
          </a>
        </header>
      );
    }
  });

  var StandaloneRoomFooter = React.createClass({
    _getContent: function() {
      return mozL10n.get("legal_text_and_links", {
        "clientShortname": mozL10n.get("clientShortname2"),
        "terms_of_use_url": React.renderComponentToStaticMarkup(
          <a href={loop.config.legalWebsiteUrl} target="_blank">
            {mozL10n.get("terms_of_use_link_text")}
          </a>
        ),
        "privacy_notice_url": React.renderComponentToStaticMarkup(
          <a href={loop.config.privacyWebsiteUrl} target="_blank">
            {mozL10n.get("privacy_notice_link_text")}
          </a>
        ),
      });
    },

    render: function() {
      return (
        <footer>
          <p dangerouslySetInnerHTML={{__html: this._getContent()}}></p>
          <div className="footer-logo" />
        </footer>
      );
    }
  });

  var StandaloneRoomView = React.createClass({
    mixins: [
      Backbone.Events,
      sharedMixins.RoomsAudioMixin
    ],

    propTypes: {
      activeRoomStore:
        React.PropTypes.instanceOf(loop.store.ActiveRoomStore).isRequired,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      helper: React.PropTypes.instanceOf(loop.shared.utils.Helper).isRequired
    },

    getInitialState: function() {
      var storeState = this.props.activeRoomStore.getStoreState();
      return _.extend({}, storeState, {
        // Used by the UI showcase.
        roomState: this.props.roomState || storeState.roomState
      });
    },

    componentWillMount: function() {
      this.listenTo(this.props.activeRoomStore, "change",
                    this._onActiveRoomStateChanged);
    },

    /**
     * Handles a "change" event on the roomStore, and updates this.state
     * to match the store.
     *
     * @private
     */
    _onActiveRoomStateChanged: function() {
      this.setState(this.props.activeRoomStore.getStoreState());
    },

    /**
     * Returns either the required DOMNode
     *
     * @param {String} className The name of the class to get the element for.
     */
    _getElement: function(className) {
      return this.getDOMNode().querySelector(className);
    },

     /**
     * Returns the required configuration for publishing video on the sdk.
     */
    _getPublisherConfig: function() {
      // height set to 100%" to fix video layout on Google Chrome
      // @see https://bugzilla.mozilla.org/show_bug.cgi?id=1020445
      return {
        insertMode: "append",
        width: "100%",
        height: "100%",
        publishVideo: true,
        style: {
          audioLevelDisplayMode: "off",
          bugDisplayMode: "off",
          buttonDisplayMode: "off",
          nameDisplayMode: "off",
          videoDisabledDisplayMode: "off"
        }
      };
    },

    componentDidMount: function() {
      // Adding a class to the document body element from here to ease styling it.
      document.body.classList.add("is-standalone-room");
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.activeRoomStore);
    },

    /**
     * Watches for when we transition from READY to JOINED room state, so we can
     * request user media access.
     * @param  {Object} nextProps (Unused)
     * @param  {Object} nextState Next state object.
     */
    componentWillUpdate: function(nextProps, nextState) {
      if (this.state.roomState === ROOM_STATES.READY &&
          nextState.roomState === ROOM_STATES.JOINED) {
        this.props.dispatcher.dispatch(new sharedActions.SetupStreamElements({
          publisherConfig: this._getPublisherConfig(),
          getLocalElementFunc: this._getElement.bind(this, ".local"),
          getRemoteElementFunc: this._getElement.bind(this, ".remote")
        }));
      }
    },

    joinRoom: function() {
      this.props.dispatcher.dispatch(new sharedActions.JoinRoom());
    },

    leaveRoom: function() {
      this.props.dispatcher.dispatch(new sharedActions.LeaveRoom());
    },

    /**
     * Toggles streaming status for a given stream type.
     *
     * @param  {String}  type     Stream type ("audio" or "video").
     * @param  {Boolean} enabled  Enabled stream flag.
     */
    publishStream: function(type, enabled) {
      this.props.dispatcher.dispatch(new sharedActions.SetMute({
        type: type,
        enabled: enabled
      }));
    },

    /**
     * Checks if current room is active.
     *
     * @return {Boolean}
     */
    _roomIsActive: function() {
      return this.state.roomState === ROOM_STATES.JOINED            ||
             this.state.roomState === ROOM_STATES.SESSION_CONNECTED ||
             this.state.roomState === ROOM_STATES.HAS_PARTICIPANTS;
    },

    render: function() {
      var localStreamClasses = React.addons.classSet({
        hide: !this._roomIsActive(),
        local: true,
        "local-stream": true,
        "local-stream-audio": false
      });

      return (
        <div className="room-conversation-wrapper">
          <StandaloneRoomHeader />
          <StandaloneRoomInfoArea roomState={this.state.roomState}
                                  failureReason={this.state.failureReason}
                                  joinRoom={this.joinRoom}
                                  helper={this.props.helper} />
          <div className="video-layout-wrapper">
            <div className="conversation room-conversation">
              <h2 className="room-name">{this.state.roomName}</h2>
              <div className="media nested">
                <div className="video_wrapper remote_wrapper">
                  <div className="video_inner remote"></div>
                </div>
                <div className={localStreamClasses}></div>
              </div>
              <sharedViews.ConversationToolbar
                video={{enabled: !this.state.videoMuted,
                        visible: this._roomIsActive()}}
                audio={{enabled: !this.state.audioMuted,
                        visible: this._roomIsActive()}}
                publishStream={this.publishStream}
                hangup={this.leaveRoom}
                hangupButtonLabel={mozL10n.get("rooms_leave_button_label")}
                enableHangup={this._roomIsActive()} />
            </div>
          </div>
          <StandaloneRoomFooter />
        </div>
      );
    }
  });

  return {
    StandaloneRoomView: StandaloneRoomView
  };
})(navigator.mozL10n);
