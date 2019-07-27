



var loop = loop || {};
loop.roomViews = (function(mozL10n) {
  "use strict";

  var ROOM_STATES = loop.store.ROOM_STATES;
  var SCREEN_SHARE_STATES = loop.shared.utils.SCREEN_SHARE_STATES;
  var sharedActions = loop.shared.actions;
  var sharedMixins = loop.shared.mixins;
  var sharedUtils = loop.shared.utils;
  var sharedViews = loop.shared.views;

  



  var ActiveRoomStoreMixin = {
    mixins: [Backbone.Events],

    propTypes: {
      roomStore: React.PropTypes.instanceOf(loop.store.RoomStore).isRequired
    },

    componentWillMount: function() {
      this.listenTo(this.props.roomStore, "change:activeRoom",
                    this._onActiveRoomStateChanged);
      this.listenTo(this.props.roomStore, "change:error",
                    this._onRoomError);
      this.listenTo(this.props.roomStore, "change:savingContext",
                    this._onRoomSavingContext);
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.roomStore);
    },

    _onActiveRoomStateChanged: function() {
      
      
      
      if (this.isMounted()) {
        this.setState(this.props.roomStore.getStoreState("activeRoom"));
      }
    },

    _onRoomError: function() {
      
      
      
      if (this.isMounted()) {
        this.setState({error: this.props.roomStore.getStoreState("error")});
      }
    },

    _onRoomSavingContext: function() {
      
      
      
      if (this.isMounted()) {
        this.setState({savingContext: this.props.roomStore.getStoreState("savingContext")});
      }
    },

    getInitialState: function() {
      var storeState = this.props.roomStore.getStoreState("activeRoom");
      return _.extend({
        
        roomState: this.props.roomState || storeState.roomState,
        savingContext: false
      }, storeState);
    }
  };

  var SocialShareDropdown = React.createClass({displayName: "SocialShareDropdown",
    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      roomUrl: React.PropTypes.string,
      show: React.PropTypes.bool.isRequired,
      socialShareProviders: React.PropTypes.array
    },

    handleAddServiceClick: function(event) {
      event.preventDefault();

      this.props.dispatcher.dispatch(new sharedActions.AddSocialShareProvider());
    },

    handleProviderClick: function(event) {
      event.preventDefault();

      var origin = event.currentTarget.dataset.provider;
      var provider = this.props.socialShareProviders
                         .filter(function(socialProvider) {
                           return socialProvider.origin == origin;
                         })[0];

      this.props.dispatcher.dispatch(new sharedActions.ShareRoomUrl({
        provider: provider,
        roomUrl: this.props.roomUrl,
        previews: []
      }));
    },

    render: function() {
      
      if (!this.props.socialShareProviders) {
        return null;
      }

      var cx = React.addons.classSet;
      var shareDropdown = cx({
        "share-service-dropdown": true,
        "dropdown-menu": true,
        "visually-hidden": true,
        "hide": !this.props.show
      });

      return (
        React.createElement("ul", {className: shareDropdown}, 
          React.createElement("li", {className: "dropdown-menu-item", onClick: this.handleAddServiceClick}, 
            React.createElement("i", {className: "icon icon-add-share-service"}), 
            React.createElement("span", null, mozL10n.get("share_add_service_button"))
          ), 
          this.props.socialShareProviders.length ? React.createElement("li", {className: "dropdown-menu-separator"}) : null, 
          
            this.props.socialShareProviders.map(function(provider, idx) {
              return (
                React.createElement("li", {className: "dropdown-menu-item", 
                    "data-provider": provider.origin, 
                    key: "provider-" + idx, 
                    onClick: this.handleProviderClick}, 
                  React.createElement("img", {className: "icon", src: provider.iconURL}), 
                  React.createElement("span", null, provider.name)
                )
              );
            }.bind(this))
          
        )
      );
    }
  });

  


  var DesktopRoomInvitationView = React.createClass({displayName: "DesktopRoomInvitationView",
    mixins: [sharedMixins.DropdownMenuMixin(".room-invitation-overlay")],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      error: React.PropTypes.object,
      mozLoop: React.PropTypes.object.isRequired,
      onAddContextClick: React.PropTypes.func,
      onEditContextClose: React.PropTypes.func,
      
      roomData: React.PropTypes.object.isRequired,
      savingContext: React.PropTypes.bool,
      show: React.PropTypes.bool.isRequired,
      showEditContext: React.PropTypes.bool.isRequired,
      socialShareProviders: React.PropTypes.array
    },

    getInitialState: function() {
      return {
        copiedUrl: false,
        newRoomName: ""
      };
    },

    handleEmailButtonClick: function(event) {
      event.preventDefault();

      var roomData = this.props.roomData;
      var contextURL = roomData.roomContextUrls && roomData.roomContextUrls[0];
      this.props.dispatcher.dispatch(
        new sharedActions.EmailRoomUrl({
          roomUrl: roomData.roomUrl,
          roomDescription: contextURL && contextURL.description,
          from: "conversation"
        }));
    },

    handleCopyButtonClick: function(event) {
      event.preventDefault();

      this.props.dispatcher.dispatch(new sharedActions.CopyRoomUrl({
        roomUrl: this.props.roomData.roomUrl,
        from: "conversation"
      }));

      this.setState({copiedUrl: true});
    },

    handleShareButtonClick: function(event) {
      event.preventDefault();

      var providers = this.props.socialShareProviders;
      
      
      if (!providers || !providers.length) {
        this.props.dispatcher.dispatch(new sharedActions.AddSocialShareProvider());
        return;
      }

      this.toggleDropdownMenu();
    },

    handleAddContextClick: function(event) {
      event.preventDefault();

      if (this.props.onAddContextClick) {
        this.props.onAddContextClick();
      }
    },

    handleEditContextClose: function() {
      if (this.props.onEditContextClose) {
        this.props.onEditContextClose();
      }
    },

    render: function() {
      if (!this.props.show) {
        return null;
      }

      var canAddContext = this.props.mozLoop.getLoopPref("contextInConversations.enabled") &&
        
        !this.props.showEditContext &&
        
        !(this.props.roomData.roomContextUrls || this.props.roomData.roomDescription);

      var cx = React.addons.classSet;
      return (
        React.createElement("div", {className: "room-invitation-overlay"}, 
          React.createElement("div", {className: "room-invitation-content"}, 
            React.createElement("p", {className: cx({hide: this.props.showEditContext})}, 
              mozL10n.get("invite_header_text")
            ), 
            React.createElement("a", {className: cx({hide: !canAddContext, "room-invitation-addcontext": true}), 
               onClick: this.handleAddContextClick}, 
              mozL10n.get("context_add_some_label")
            )
          ), 
          React.createElement("div", {className: cx({
            "btn-group": true,
            "call-action-group": true,
            hide: this.props.showEditContext
          })}, 
            React.createElement("button", {className: "btn btn-info btn-email", 
                    onClick: this.handleEmailButtonClick}, 
              mozL10n.get("email_link_button")
            ), 
            React.createElement("button", {className: "btn btn-info btn-copy", 
                    onClick: this.handleCopyButtonClick}, 
              this.state.copiedUrl ? mozL10n.get("copied_url_button") :
                                      mozL10n.get("copy_url_button2")
            ), 
            React.createElement("button", {className: "btn btn-info btn-share", 
                    onClick: this.handleShareButtonClick, 
                    ref: "anchor"}, 
              mozL10n.get("share_button3")
            )
          ), 
          React.createElement(SocialShareDropdown, {
            dispatcher: this.props.dispatcher, 
            ref: "menu", 
            roomUrl: this.props.roomData.roomUrl, 
            show: this.state.showMenu, 
            socialShareProviders: this.props.socialShareProviders}), 
          React.createElement(DesktopRoomEditContextView, {
            dispatcher: this.props.dispatcher, 
            error: this.props.error, 
            mozLoop: this.props.mozLoop, 
            onClose: this.handleEditContextClose, 
            roomData: this.props.roomData, 
            savingContext: this.props.savingContext, 
            show: this.props.showEditContext})
        )
      );
    }
  });

  var DesktopRoomEditContextView = React.createClass({displayName: "DesktopRoomEditContextView",
    mixins: [React.addons.LinkedStateMixin],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      error: React.PropTypes.object,
      mozLoop: React.PropTypes.object.isRequired,
      onClose: React.PropTypes.func,
      
      roomData: React.PropTypes.object.isRequired,
      savingContext: React.PropTypes.bool.isRequired,
      show: React.PropTypes.bool.isRequired
    },

    componentWillMount: function() {
      this._fetchMetadata();
    },

    componentWillReceiveProps: function(nextProps) {
      var newState = {};
      
      
      if (("show" in nextProps) && nextProps.show !== this.props.show) {
        newState.show = nextProps.show;
        if (nextProps.show) {
          this._fetchMetadata();
        }
      }
      
      
      
      if (nextProps.roomData) {
        
        
        if (!this.state.newRoomName && nextProps.roomData.roomName) {
          newState.newRoomName = nextProps.roomData.roomName;
        }
        var url = this._getURL(nextProps.roomData);
        if (url) {
          if (!this.state.newRoomURL && url.location) {
            newState.newRoomURL = url.location;
          }
          if (!this.state.newRoomDescription && url.description) {
            newState.newRoomDescription = url.description;
          }
          if (!this.state.newRoomThumbnail && url.thumbnail) {
            newState.newRoomThumbnail = url.thumbnail;
          }
        }
      }

      
      
      if (("savingContext" in nextProps) && this.props.savingContext &&
          this.props.savingContext !== nextProps.savingContext && this.state.show
          && !this.props.error && !nextProps.error) {
        newState.show = false;
        if (this.props.onClose) {
          this.props.onClose();
        }
      }

      if (Object.getOwnPropertyNames(newState).length) {
        this.setState(newState);
      }
    },

    getInitialState: function() {
      var url = this._getURL();
      return {
        
        availableContext: null,
        show: this.props.show,
        newRoomName: this.props.roomData.roomName || "",
        newRoomURL: url && url.location || "",
        newRoomDescription: url && url.description || "",
        newRoomThumbnail: url && url.thumbnail || ""
      };
    },

    _fetchMetadata: function() {
      this.props.mozLoop.getSelectedTabMetadata(function(metadata) {
        var previewImage = metadata.favicon || "";
        var description = metadata.title || metadata.description;
        var metaUrl = metadata.url;
        this.setState({
          availableContext: {
            previewImage: previewImage,
            description: description,
            url: metaUrl
          }
       });
      }.bind(this));
    },

    handleCloseClick: function(event) {
      event.stopPropagation();
      event.preventDefault();

      this.setState({ show: false });
      if (this.props.onClose) {
        this.props.onClose();
      }
    },

    handleContextClick: function(event) {
      event.stopPropagation();
      event.preventDefault();

      var url = this._getURL();
      if (!url || !url.location) {
        return;
      }

      this.props.mozLoop.openURL(url.location);
    },

    handleCheckboxChange: function(state) {
      if (state.checked) {
        
        
        var context = this.state.availableContext;
        this.setState({
          newRoomURL: context.url,
          newRoomDescription: context.description,
          newRoomThumbnail: context.previewImage
        });
      } else {
        this.setState({
          newRoomURL: "",
          newRoomDescription: "",
          newRoomThumbnail: ""
        });
      }
    },

    handleFormSubmit: function(event) {
      event && event.preventDefault();

      this.props.dispatcher.dispatch(new sharedActions.UpdateRoomContext({
        roomToken: this.props.roomData.roomToken,
        newRoomName: this.state.newRoomName,
        newRoomURL: this.state.newRoomURL,
        newRoomDescription: this.state.newRoomDescription,
        newRoomThumbnail: this.state.newRoomThumbnail
      }));
    },

    handleTextareaKeyDown: function(event) {
      
      
      
      
      if (event.which === 13) {
        this.handleFormSubmit(event);
      }
    },

    







    _getURL: function(roomData) {
      roomData = roomData || this.props.roomData;
      return this.props.roomData.roomContextUrls &&
        this.props.roomData.roomContextUrls[0];
    },

    









    _truncate: function(str, maxLen) {
      if (!maxLen) {
        maxLen = 72;
      }
      return (str.length > maxLen) ? str.substr(0, maxLen) + "â€¦" : str;
    },

    render: function() {
      if (!this.state.show) {
        return null;
      }

      var url = this._getURL();
      var thumbnail = url && url.thumbnail || "loop/shared/img/icons-16x16.svg#globe";
      var urlDescription = url && url.description || "";
      var location = url && url.location || "";

      var cx = React.addons.classSet;
      var availableContext = this.state.availableContext;
      
      
      var checked = !!urlDescription;
      var checkboxLabel = urlDescription || (availableContext && availableContext.url ?
        availableContext.description : "");

      return (
        React.createElement("div", {className: "room-context"}, 
          React.createElement("p", {className: cx({"error": !!this.props.error,
                            "error-display-area": true})}, 
            mozL10n.get("rooms_change_failed_label")
          ), 
          React.createElement("div", {className: "room-context-label"}, mozL10n.get("context_inroom_label")), 
          React.createElement(sharedViews.Checkbox, {
            additionalClass: cx({ hide: !checkboxLabel }), 
            checked: checked, 
            disabled: checked, 
            label: checkboxLabel, 
            onChange: this.handleCheckboxChange, 
            value: location}), 
          React.createElement("form", {onSubmit: this.handleFormSubmit}, 
            React.createElement("input", {className: "room-context-name", 
              onKeyDown: this.handleTextareaKeyDown, 
              placeholder: mozL10n.get("context_edit_name_placeholder"), 
              type: "text", 
              valueLink: this.linkState("newRoomName")}), 
            React.createElement("input", {className: "room-context-url", 
              disabled: availableContext && availableContext.url === this.state.newRoomURL, 
              onKeyDown: this.handleTextareaKeyDown, 
              placeholder: "https://", 
              type: "text", 
              valueLink: this.linkState("newRoomURL")}), 
            React.createElement("textarea", {className: "room-context-comments", 
              onKeyDown: this.handleTextareaKeyDown, 
              placeholder: mozL10n.get("context_edit_comments_placeholder"), 
              rows: "3", type: "text", 
              valueLink: this.linkState("newRoomDescription")})
          ), 
          React.createElement("button", {className: "btn btn-info", 
                  disabled: this.props.savingContext, 
                  onClick: this.handleFormSubmit}, 
            mozL10n.get("context_save_label2")
          ), 
          React.createElement("button", {className: "room-context-btn-close", 
                  onClick: this.handleCloseClick, 
                  title: mozL10n.get("cancel_button")})
        )
      );
    }
  });

  


  var DesktopRoomConversationView = React.createClass({displayName: "DesktopRoomConversationView",
    mixins: [
      ActiveRoomStoreMixin,
      sharedMixins.DocumentTitleMixin,
      sharedMixins.MediaSetupMixin,
      sharedMixins.RoomsAudioMixin,
      sharedMixins.WindowCloseMixin
    ],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      
      localPosterUrl: React.PropTypes.string,
      mozLoop: React.PropTypes.object.isRequired,
      remotePosterUrl: React.PropTypes.string,
      roomStore: React.PropTypes.instanceOf(loop.store.RoomStore).isRequired
    },

    getInitialState: function() {
      return {
        contextEnabled: this.props.mozLoop.getLoopPref("contextInConversations.enabled"),
        showEditContext: false
      };
    },

    componentWillUpdate: function(nextProps, nextState) {
      
      
      
      if (this.state.roomState !== ROOM_STATES.MEDIA_WAIT &&
          nextState.roomState === ROOM_STATES.MEDIA_WAIT) {
        this.props.dispatcher.dispatch(new sharedActions.SetupStreamElements({
          publisherConfig: this.getDefaultPublisherConfig({
            publishVideo: !this.state.videoMuted
          })
        }));
      }
    },

    


    leaveRoom: function() {
      if (this.state.used) {
        this.props.dispatcher.dispatch(new sharedActions.LeaveRoom());
      } else {
        this.closeWindow();
      }
    },

    





    publishStream: function(type, enabled) {
      this.props.dispatcher.dispatch(
        new sharedActions.SetMute({
          type: type,
          enabled: enabled
        }));
    },

    _shouldRenderInvitationOverlay: function() {
      return (this.state.roomState !== ROOM_STATES.HAS_PARTICIPANTS);
    },

    








    shouldRenderRemoteVideo: function() {
      switch(this.state.roomState) {
        case ROOM_STATES.HAS_PARTICIPANTS:
          if (this.state.remoteVideoEnabled) {
            return true;
          }

          if (this.state.mediaConnected) {
            
            
            return false;
          }

          return true;

        case ROOM_STATES.READY:
        case ROOM_STATES.INIT:
        case ROOM_STATES.JOINING:
        case ROOM_STATES.SESSION_CONNECTED:
        case ROOM_STATES.JOINED:
        case ROOM_STATES.MEDIA_WAIT:
          
          
          return true;

        case ROOM_STATES.CLOSING:
          return true;

        default:
          console.warn("DesktopRoomConversationView.shouldRenderRemoteVideo:" +
            " unexpected roomState: ", this.state.roomState);
          return true;
      }
    },

    






    _shouldRenderLocalLoading: function () {
      return this.state.roomState === ROOM_STATES.MEDIA_WAIT &&
             !this.state.localSrcVideoObject;
    },

    






    _shouldRenderRemoteLoading: function() {
      return !!(this.state.roomState === ROOM_STATES.HAS_PARTICIPANTS &&
                !this.state.remoteSrcVideoObject &&
                !this.state.mediaConnected);
    },

    handleAddContextClick: function() {
      this.setState({ showEditContext: true });
    },

    handleEditContextClick: function() {
      this.setState({ showEditContext: !this.state.showEditContext });
    },

    handleEditContextClose: function() {
      this.setState({ showEditContext: false });
    },

    render: function() {
      if (this.state.roomName) {
        this.setTitle(this.state.roomName);
      }

      var localStreamClasses = React.addons.classSet({
        local: true,
        "local-stream": true,
        "local-stream-audio": this.state.videoMuted,
        "room-preview": this.state.roomState !== ROOM_STATES.HAS_PARTICIPANTS
      });

      var screenShareData = {
        state: this.state.screenSharingState || SCREEN_SHARE_STATES.INACTIVE,
        visible: true
      };

      var shouldRenderInvitationOverlay = this._shouldRenderInvitationOverlay();
      var shouldRenderEditContextView = this.state.contextEnabled && this.state.showEditContext;
      var roomData = this.props.roomStore.getStoreState("activeRoom");

      switch(this.state.roomState) {
        case ROOM_STATES.FAILED:
        case ROOM_STATES.FULL: {
          
          
          return (
            React.createElement(loop.conversationViews.GenericFailureView, {
              cancelCall: this.closeWindow, 
              failureReason: this.state.failureReason})
          );
        }
        case ROOM_STATES.ENDED: {
          return (
            React.createElement(sharedViews.FeedbackView, {
              onAfterFeedbackReceived: this.closeWindow})
          );
        }
        default: {

          return (
            React.createElement("div", {className: "room-conversation-wrapper"}, 
              React.createElement("div", {className: "video-layout-wrapper"}, 
                React.createElement("div", {className: "conversation room-conversation"}, 
                  React.createElement("div", {className: "media nested"}, 
                    React.createElement(DesktopRoomInvitationView, {
                      dispatcher: this.props.dispatcher, 
                      error: this.state.error, 
                      mozLoop: this.props.mozLoop, 
                      onAddContextClick: this.handleAddContextClick, 
                      onEditContextClose: this.handleEditContextClose, 
                      roomData: roomData, 
                      savingContext: this.state.savingContext, 
                      show: shouldRenderInvitationOverlay, 
                      showEditContext: shouldRenderInvitationOverlay && shouldRenderEditContextView, 
                      socialShareProviders: this.state.socialShareProviders}), 
                    React.createElement("div", {className: "video_wrapper remote_wrapper"}, 
                      React.createElement("div", {className: "video_inner remote focus-stream"}, 
                        React.createElement(sharedViews.MediaView, {displayAvatar: !this.shouldRenderRemoteVideo(), 
                          isLoading: this._shouldRenderRemoteLoading(), 
                          mediaType: "remote", 
                          posterUrl: this.props.remotePosterUrl, 
                          srcVideoObject: this.state.remoteSrcVideoObject})
                      )
                    ), 
                    React.createElement("div", {className: localStreamClasses}, 
                      React.createElement(sharedViews.MediaView, {displayAvatar: this.state.videoMuted, 
                        isLoading: this._shouldRenderLocalLoading(), 
                        mediaType: "local", 
                        posterUrl: this.props.localPosterUrl, 
                        srcVideoObject: this.state.localSrcVideoObject})
                    )
                  ), 
                  React.createElement(sharedViews.ConversationToolbar, {
                    audio: {enabled: !this.state.audioMuted, visible: true}, 
                    dispatcher: this.props.dispatcher, 
                    edit: { visible: this.state.contextEnabled, enabled: !this.state.showEditContext}, 
                    hangup: this.leaveRoom, 
                    onEditClick: this.handleEditContextClick, 
                    publishStream: this.publishStream, 
                    screenShare: screenShareData, 
                    video: {enabled: !this.state.videoMuted, visible: true}})
                )
              ), 
              React.createElement(DesktopRoomEditContextView, {
                dispatcher: this.props.dispatcher, 
                error: this.state.error, 
                mozLoop: this.props.mozLoop, 
                onClose: this.handleEditContextClose, 
                roomData: roomData, 
                savingContext: this.state.savingContext, 
                show: !shouldRenderInvitationOverlay && shouldRenderEditContextView}), 
              React.createElement(sharedViews.chat.TextChatView, {
                dispatcher: this.props.dispatcher, 
                showAlways: false, 
                showRoomName: false, 
                useDesktopPaths: true})
            )
          );
        }
      }
    }
  });

  return {
    ActiveRoomStoreMixin: ActiveRoomStoreMixin,
    SocialShareDropdown: SocialShareDropdown,
    DesktopRoomEditContextView: DesktopRoomEditContextView,
    DesktopRoomConversationView: DesktopRoomConversationView,
    DesktopRoomInvitationView: DesktopRoomInvitationView
  };

})(document.mozL10n || navigator.mozL10n);
