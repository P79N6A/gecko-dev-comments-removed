/** @jsx React.DOM */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global loop:true, React */
/* jshint newcap:false, maxlen:false */

var loop = loop || {};
loop.standaloneRoomViews = (function(mozL10n) {
  "use strict";

  var FAILURE_DETAILS = loop.shared.utils.FAILURE_DETAILS;
  var ROOM_STATES = loop.store.ROOM_STATES;
  var sharedActions = loop.shared.actions;
  var sharedMixins = loop.shared.mixins;
  var sharedViews = loop.shared.views;

  var StandaloneRoomInfoArea = React.createClass({
    propTypes: {
      helper: React.PropTypes.instanceOf(loop.shared.utils.Helper).isRequired,
      activeRoomStore: React.PropTypes.oneOfType([
        React.PropTypes.instanceOf(loop.store.ActiveRoomStore),
        React.PropTypes.instanceOf(loop.store.FxOSActiveRoomStore)
      ]).isRequired
    },

    onFeedbackSent: function() {
      // We pass a tick to prevent React warnings regarding nested updates.
      setTimeout(function() {
        this.props.activeRoomStore.dispatchAction(new sharedActions.FeedbackComplete());
      }.bind(this));
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
        case FAILURE_DETAILS.MEDIA_DENIED:
          return mozL10n.get("rooms_media_denied_message");
        case FAILURE_DETAILS.EXPIRED_OR_INVALID:
          return mozL10n.get("rooms_unavailable_notification_message");
        default:
          return mozL10n.get("status_error");
      }
    },

    render: function() {
      switch(this.props.roomState) {
        case ROOM_STATES.INIT:
        case ROOM_STATES.READY: {
          // XXX: In ENDED state, we should rather display the feedback form.
          return (
            <div className="room-inner-info-area">
              <button className="btn btn-join btn-info"
                      onClick={this.props.joinRoom}>
                {mozL10n.get("rooms_room_join_label")}
              </button>
            </div>
          );
        }
        case ROOM_STATES.MEDIA_WAIT: {
          var msg = mozL10n.get("call_progress_getting_media_description",
                                {clientShortname: mozL10n.get("clientShortname2")});
          // XXX Bug 1047040 will add images to help prompt the user.
          return (
            <div className="room-inner-info-area">
              <p className="prompt-media-message">
                {msg}
              </p>
            </div>
          );
        }
        case ROOM_STATES.JOINING:
        case ROOM_STATES.JOINED:
        case ROOM_STATES.SESSION_CONNECTED: {
          return (
            <div className="room-inner-info-area">
              <p className="empty-room-message">
                {mozL10n.get("rooms_only_occupant_label")}
              </p>
            </div>
          );
        }
        case ROOM_STATES.FULL: {
          return (
            <div className="room-inner-info-area">
              <p className="full-room-message">
                {mozL10n.get("rooms_room_full_label")}
              </p>
              <p>{this._renderCallToActionLink()}</p>
            </div>
          );
        }
        case ROOM_STATES.ENDED: {
          if (this.props.roomUsed)
            return (
              <div className="ended-conversation">
                <sharedViews.FeedbackView
                  onAfterFeedbackReceived={this.onFeedbackSent}
                />
              </div>
            );

          // In case the room was not used (no one was here), we
          // bypass the feedback form.
          this.onFeedbackSent();
          return null;
        }
        case ROOM_STATES.FAILED: {
          return (
            <div className="room-inner-info-area">
              <p className="failed-room-message">
                {this._getFailureString()}
              </p>
              <button className="btn btn-join btn-info"
                      onClick={this.props.joinRoom}>
                {mozL10n.get("retry_call_button")}
              </button>
            </div>
          );
        }
        default: {
          return null;
        }
      }
    }
  });

  var StandaloneRoomHeader = React.createClass({
    render: function() {
      return (
        <header>
          <h1>{mozL10n.get("clientShortname2")}</h1>
          <a target="_blank" href={loop.config.generalSupportUrl}>
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
        "terms_of_use_url": React.renderToStaticMarkup(
          <a href={loop.config.legalWebsiteUrl} target="_blank">
            {mozL10n.get("terms_of_use_link_text")}
          </a>
        ),
        "privacy_notice_url": React.renderToStaticMarkup(
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
      sharedMixins.MediaSetupMixin,
      sharedMixins.RoomsAudioMixin
    ],

    propTypes: {
      activeRoomStore: React.PropTypes.oneOfType([
        React.PropTypes.instanceOf(loop.store.ActiveRoomStore),
        React.PropTypes.instanceOf(loop.store.FxOSActiveRoomStore)
      ]).isRequired,
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
      var state = this.props.activeRoomStore.getStoreState();
      this.updateVideoDimensions(state.localVideoDimensions, state.remoteVideoDimensions);
      this.setState(state);
    },

    componentDidMount: function() {
      // Adding a class to the document body element from here to ease styling it.
      document.body.classList.add("is-standalone-room");
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.activeRoomStore);
    },

    /**
     * Watches for when we transition to MEDIA_WAIT room state, so we can request
     * user media access.
     *
     * @param  {Object} nextProps (Unused)
     * @param  {Object} nextState Next state object.
     */
    componentWillUpdate: function(nextProps, nextState) {
      if (this.state.roomState !== ROOM_STATES.MEDIA_WAIT &&
          nextState.roomState === ROOM_STATES.MEDIA_WAIT) {
        this.props.dispatcher.dispatch(new sharedActions.SetupStreamElements({
          publisherConfig: this.getDefaultPublisherConfig({publishVideo: true}),
          getLocalElementFunc: this._getElement.bind(this, ".local"),
          getRemoteElementFunc: this._getElement.bind(this, ".remote"),
          getScreenShareElementFunc: this._getElement.bind(this, ".screen")
        }));
      }

      if (this.state.roomState !== ROOM_STATES.JOINED &&
          nextState.roomState === ROOM_STATES.JOINED) {
        // This forces the video size to update - creating the publisher
        // first, and then connecting to the session doesn't seem to set the
        // initial size correctly.
        this.updateVideoContainer();
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
     * Specifically updates the local camera stream size and position, depending
     * on the size and position of the remote video stream.
     * This method gets called from `updateVideoContainer`, which is defined in
     * the `MediaSetupMixin`.
     *
     * @param  {Object} ratio Aspect ratio of the local camera stream
     */
    updateLocalCameraPosition: function(ratio) {
      var node = this._getElement(".local");
      var parent = node.offsetParent || this._getElement(".media");
      // The local camera view should be a sixth of the size of its offset parent
      // and positioned to overlap with the remote stream at a quarter of its width.
      var parentWidth = parent.offsetWidth;
      var targetWidth = parentWidth / 6;

      node.style.right = "auto";
      if (window.matchMedia && window.matchMedia("screen and (max-width:640px)").matches) {
        targetWidth = 180;
        node.style.left = "auto";
      } else {
        // Now position the local camera view correctly with respect to the remote
        // video stream.
        var remoteVideoDimensions = this.getRemoteVideoDimensions();
        var offsetX = (remoteVideoDimensions.streamWidth + remoteVideoDimensions.offsetX);
        // The horizontal offset of the stream, and the width of the resulting
        // pillarbox, is determined by the height exponent of the aspect ratio.
        // Therefore we multiply the width of the local camera view by the height
        // ratio.
        node.style.left = (offsetX - ((targetWidth * ratio.height) / 4)) + "px";
      }
      node.style.width = (targetWidth * ratio.width) + "px";
      node.style.height = (targetWidth * ratio.height) + "px";
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
        "local-stream-audio": this.state.videoMuted
      });

      var remoteStreamClasses = React.addons.classSet({
        "video_inner": true,
        "remote": true,
        "remote-stream": true,
        hide: this.state.receivingScreenShare
      });

      var screenShareStreamClasses = React.addons.classSet({
        "screen": true,
        "remote-stream": true,
        hide: !this.state.receivingScreenShare
      });

      return (
        <div className="room-conversation-wrapper">
          <div className="beta-logo" />
          <StandaloneRoomHeader />
          <StandaloneRoomInfoArea roomState={this.state.roomState}
                                  failureReason={this.state.failureReason}
                                  joinRoom={this.joinRoom}
                                  helper={this.props.helper}
                                  activeRoomStore={this.props.activeRoomStore}
                                  roomUsed={this.state.used} />
          <div className="video-layout-wrapper">
            <div className="conversation room-conversation">
              <h2 className="room-name">{this.state.roomName}</h2>
              <div className="media nested">
                <span className="self-view-hidden-message">
                  {mozL10n.get("self_view_hidden_message")}
                </span>
                <div className="video_wrapper remote_wrapper">
                  <div className={remoteStreamClasses}></div>
                  <div className={screenShareStreamClasses}></div>
                </div>
                <div className={localStreamClasses}></div>
              </div>
              <sharedViews.ConversationToolbar
                dispatcher={this.props.dispatcher}
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
          <loop.fxOSMarketplaceViews.FxOSHiddenMarketplaceView
            marketplaceSrc={this.state.marketplaceSrc}
            onMarketplaceMessage={this.state.onMarketplaceMessage} />
          <StandaloneRoomFooter />
        </div>
      );
    }
  });

  return {
    StandaloneRoomView: StandaloneRoomView
  };
})(navigator.mozL10n);
