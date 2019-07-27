








var loop = loop || {};
loop.panel = (function(_, mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views;
  var sharedModels = loop.shared.models;
  var sharedMixins = loop.shared.mixins;
  var sharedActions = loop.shared.actions;
  var sharedUtils = loop.shared.utils;
  var Button = sharedViews.Button;
  var ButtonGroup = sharedViews.ButtonGroup;
  var ContactsList = loop.contacts.ContactsList;
  var ContactDetailsForm = loop.contacts.ContactDetailsForm;

  var TabView = React.createClass({displayName: 'TabView',
    propTypes: {
      buttonsHidden: React.PropTypes.bool,
      
      selectedTab: React.PropTypes.string
    },

    getDefaultProps: function() {
      return {
        buttonsHidden: false
      };
    },

    getInitialState: function() {
      
      
      
      return {
        selectedTab: this.props.selectedTab ||
          (navigator.mozLoop.getLoopBoolPref("rooms.enabled") ?
            "rooms" : "call")
      };
    },

    handleSelectTab: function(event) {
      var tabName = event.target.dataset.tabName;
      this.setState({selectedTab: tabName});
    },

    render: function() {
      var cx = React.addons.classSet;
      var tabButtons = [];
      var tabs = [];
      React.Children.forEach(this.props.children, function(tab, i) {
        
        if (!tab) {
          return;
        }
        var tabName = tab.props.name;
        var isSelected = (this.state.selectedTab == tabName);
        if (!tab.props.hidden) {
          tabButtons.push(
            React.DOM.li({className: cx({selected: isSelected}), 
                key: i, 
                'data-tab-name': tabName, 
                onClick: this.handleSelectTab})
          );
        }
        tabs.push(
          React.DOM.div({key: i, className: cx({tab: true, selected: isSelected})}, 
            tab.props.children
          )
        );
      }, this);
      return (
        React.DOM.div({className: "tab-view-container"}, 
          !this.props.buttonsHidden
            ? React.DOM.ul({className: "tab-view"}, tabButtons)
            : null, 
          tabs
        )
      );
    }
  });

  var Tab = React.createClass({displayName: 'Tab',
    render: function() {
      return null;
    }
  });

  


  var AvailabilityDropdown = React.createClass({displayName: 'AvailabilityDropdown',
    mixins: [sharedMixins.DropdownMenuMixin],

    getInitialState: function() {
      return {
        doNotDisturb: navigator.mozLoop.doNotDisturb
      };
    },

    
    
    
    changeAvailability: function(newAvailabilty) {
      return function(event) {
        
        switch (newAvailabilty) {
          case 'available':
            this.setState({doNotDisturb: false});
            navigator.mozLoop.doNotDisturb = false;
            break;
          case 'do-not-disturb':
            this.setState({doNotDisturb: true});
            navigator.mozLoop.doNotDisturb = true;
            break;
        }
        this.hideDropdownMenu();
      }.bind(this);
    },

    render: function() {
      
      var cx = React.addons.classSet;
      var availabilityStatus = cx({
        'status': true,
        'status-dnd': this.state.doNotDisturb,
        'status-available': !this.state.doNotDisturb
      });
      var availabilityDropdown = cx({
        'dropdown-menu': true,
        'hide': !this.state.showMenu
      });
      var availabilityText = this.state.doNotDisturb ?
                              mozL10n.get("display_name_dnd_status") :
                              mozL10n.get("display_name_available_status");

      return (
        React.DOM.div({className: "dropdown"}, 
          React.DOM.p({className: "dnd-status", onClick: this.showDropdownMenu}, 
            React.DOM.span(null, availabilityText), 
            React.DOM.i({className: availabilityStatus})
          ), 
          React.DOM.ul({className: availabilityDropdown, 
              onMouseLeave: this.hideDropdownMenu}, 
            React.DOM.li({onClick: this.changeAvailability("available"), 
                className: "dropdown-menu-item dnd-make-available"}, 
              React.DOM.i({className: "status status-available"}), 
              React.DOM.span(null, mozL10n.get("display_name_available_status"))
            ), 
            React.DOM.li({onClick: this.changeAvailability("do-not-disturb"), 
                className: "dropdown-menu-item dnd-make-unavailable"}, 
              React.DOM.i({className: "status status-dnd"}), 
              React.DOM.span(null, mozL10n.get("display_name_dnd_status"))
            )
          )
        )
      );
    }
  });

  var GettingStartedView = React.createClass({displayName: 'GettingStartedView',
    componentDidMount: function() {
      navigator.mozLoop.setLoopBoolPref("gettingStarted.seen", true);
    },

    handleButtonClick: function() {
      navigator.mozLoop.openGettingStartedTour();
    },

    render: function() {
      if (navigator.mozLoop.getLoopBoolPref("gettingStarted.seen")) {
        return null;
      }
      return (
        React.DOM.div({id: "fte-getstarted"}, 
          React.DOM.header({id: "fte-title"}, 
            mozL10n.get("first_time_experience_title", {
              "clientShortname": mozL10n.get("clientShortname2")
            })
          ), 
          Button({htmlId: "fte-button", 
                  onClick: this.handleButtonClick, 
                  caption: mozL10n.get("first_time_experience_button_label")})
        )
      );
    }
  });

  var ToSView = React.createClass({displayName: 'ToSView',
    getInitialState: function() {
      return {seenToS: navigator.mozLoop.getLoopCharPref('seenToS')};
    },

    render: function() {
      if (this.state.seenToS == "unseen") {
        var locale = mozL10n.getLanguage();
        var terms_of_use_url = navigator.mozLoop.getLoopCharPref('legal.ToS_url');
        var privacy_notice_url = navigator.mozLoop.getLoopCharPref('legal.privacy_url');
        var tosHTML = mozL10n.get("legal_text_and_links3", {
          "clientShortname": mozL10n.get("clientShortname2"),
          "terms_of_use": React.renderComponentToStaticMarkup(
            React.DOM.a({href: terms_of_use_url, target: "_blank"}, 
              mozL10n.get("legal_text_tos")
            )
          ),
          "privacy_notice": React.renderComponentToStaticMarkup(
            React.DOM.a({href: privacy_notice_url, target: "_blank"}, 
              mozL10n.get("legal_text_privacy")
            )
          ),
        });
        return React.DOM.div(null, 
          React.DOM.p({id: "powered-by"}, 
            mozL10n.get("powered_by_beforeLogo"), 
            React.DOM.img({id: "powered-by-logo", className: locale}), 
            mozL10n.get("powered_by_afterLogo")
          ), 
          React.DOM.p({className: "terms-service", 
             dangerouslySetInnerHTML: {__html: tosHTML}})
         );
      } else {
        return React.DOM.div(null);
      }
    }
  });

  


  var SettingsDropdownEntry = React.createClass({displayName: 'SettingsDropdownEntry',
    propTypes: {
      onClick: React.PropTypes.func.isRequired,
      label: React.PropTypes.string.isRequired,
      icon: React.PropTypes.string,
      displayed: React.PropTypes.bool
    },

    getDefaultProps: function() {
      return {displayed: true};
    },

    render: function() {
      if (!this.props.displayed) {
        return null;
      }
      return (
        React.DOM.li({onClick: this.props.onClick, className: "dropdown-menu-item"}, 
          this.props.icon ?
            React.DOM.i({className: "icon icon-" + this.props.icon}) :
            null, 
          React.DOM.span(null, this.props.label)
        )
      );
    }
  });

  


  var SettingsDropdown = React.createClass({displayName: 'SettingsDropdown',
    mixins: [sharedMixins.DropdownMenuMixin],

    handleClickSettingsEntry: function() {
      
    },

    handleClickAccountEntry: function() {
      navigator.mozLoop.openFxASettings();
    },

    handleClickAuthEntry: function() {
      if (this._isSignedIn()) {
        navigator.mozLoop.logOutFromFxA();
      } else {
        navigator.mozLoop.logInToFxA();
      }
    },

    _isSignedIn: function() {
      return !!navigator.mozLoop.userProfile;
    },

    render: function() {
      var cx = React.addons.classSet;

      
      if (!navigator.mozLoop.fxAEnabled) {
        return null;
      }

      return (
        React.DOM.div({className: "settings-menu dropdown"}, 
          React.DOM.a({className: "button-settings", onClick: this.showDropdownMenu, 
             title: mozL10n.get("settings_menu_button_tooltip")}), 
          React.DOM.ul({className: cx({"dropdown-menu": true, hide: !this.state.showMenu}), 
              onMouseLeave: this.hideDropdownMenu}, 
            SettingsDropdownEntry({label: mozL10n.get("settings_menu_item_settings"), 
                                   onClick: this.handleClickSettingsEntry, 
                                   displayed: false, 
                                   icon: "settings"}), 
            SettingsDropdownEntry({label: mozL10n.get("settings_menu_item_account"), 
                                   onClick: this.handleClickAccountEntry, 
                                   icon: "account", 
                                   displayed: this._isSignedIn()}), 
            SettingsDropdownEntry({label: this._isSignedIn() ?
                                          mozL10n.get("settings_menu_item_signout") :
                                          mozL10n.get("settings_menu_item_signin"), 
                                   onClick: this.handleClickAuthEntry, 
                                   displayed: navigator.mozLoop.fxAEnabled, 
                                   icon: this._isSignedIn() ? "signout" : "signin"})
          )
        )
      );
    }
  });

  


  var CallUrlResult = React.createClass({displayName: 'CallUrlResult',
    mixins: [sharedMixins.DocumentVisibilityMixin],

    propTypes: {
      callUrl:        React.PropTypes.string,
      callUrlExpiry:  React.PropTypes.number,
      notifications:  React.PropTypes.object.isRequired,
      client:         React.PropTypes.object.isRequired
    },

    getInitialState: function() {
      return {
        pending: false,
        copied: false,
        callUrl: this.props.callUrl || "",
        callUrlExpiry: 0
      };
    },

    



    onDocumentVisible: function() {
      this._fetchCallUrl();
    },

    componentDidMount: function() {
      
      
      if (this.state.callUrl.length) {
        return;
      }

      this._fetchCallUrl();
    },

    


    _fetchCallUrl: function() {
      this.setState({pending: true});
      
      
      this.props.client.requestCallUrl("",
                                       this._onCallUrlReceived);
    },

    _onCallUrlReceived: function(err, callUrlData) {
      if (err) {
        if (err.code != 401) {
          
          
          this.props.notifications.errorL10n("unable_retrieve_url");
        }
        this.setState(this.getInitialState());
      } else {
        try {
          var callUrl = new window.URL(callUrlData.callUrl);
          
          
          var token = callUrlData.callToken ||
                      callUrl.pathname.split('/').pop();

          
          this.linkExfiltrated = false;

          this.setState({pending: false, copied: false,
                         callUrl: callUrl.href,
                         callUrlExpiry: callUrlData.expiresAt});
        } catch(e) {
          console.log(e);
          this.props.notifications.errorL10n("unable_retrieve_url");
          this.setState(this.getInitialState());
        }
      }
    },

    handleEmailButtonClick: function(event) {
      this.handleLinkExfiltration(event);

      sharedUtils.composeCallUrlEmail(this.state.callUrl);
    },

    handleCopyButtonClick: function(event) {
      this.handleLinkExfiltration(event);
      
      
      navigator.mozLoop.copyString(this.state.callUrl);
      this.setState({copied: true});
    },

    linkExfiltrated: false,

    handleLinkExfiltration: function(event) {
      
      if (!this.linkExfiltrated) {
        this.linkExfiltrated = true;
        try {
          navigator.mozLoop.telemetryAdd("LOOP_CLIENT_CALL_URL_SHARED", true);
        } catch (err) {
          console.error("Error recording telemetry", err);
        }
      }

      
      if (this.state.callUrlExpiry) {
        navigator.mozLoop.noteCallUrlExpiry(this.state.callUrlExpiry);
      }
    },

    render: function() {
      
      
      
      
      var cx = React.addons.classSet;
      return (
        React.DOM.div({className: "generate-url"}, 
          React.DOM.header({id: "share-link-header"}, mozL10n.get("share_link_header_text")), 
          React.DOM.div({className: "generate-url-stack"}, 
            React.DOM.input({type: "url", value: this.state.callUrl, readOnly: "true", 
                   onCopy: this.handleLinkExfiltration, 
                   className: cx({"generate-url-input": true,
                                  pending: this.state.pending,
                                  
                                  
                                  callUrl: !this.state.pending})}), 
            React.DOM.div({className: cx({"generate-url-spinner": true,
                                spinner: true,
                                busy: this.state.pending})})
          ), 
          ButtonGroup({additionalClass: "url-actions"}, 
            Button({additionalClass: "button-email", 
                    disabled: !this.state.callUrl, 
                    onClick: this.handleEmailButtonClick, 
                    caption: mozL10n.get("share_button")}), 
            Button({additionalClass: "button-copy", 
                    disabled: !this.state.callUrl, 
                    onClick: this.handleCopyButtonClick, 
                    caption: this.state.copied ? mozL10n.get("copied_url_button") :
                                                 mozL10n.get("copy_url_button")})
          )
        )
      );
    }
  });

  


  var AuthLink = React.createClass({displayName: 'AuthLink',
    handleSignUpLinkClick: function() {
      navigator.mozLoop.logInToFxA();
    },

    render: function() {
      if (!navigator.mozLoop.fxAEnabled || navigator.mozLoop.userProfile) {
        return null;
      }
      return (
        React.DOM.p({className: "signin-link"}, 
          React.DOM.a({href: "#", onClick: this.handleSignUpLinkClick}, 
            mozL10n.get("panel_footer_signin_or_signup_link")
          )
        )
      );
    }
  });

  


  var UserIdentity = React.createClass({displayName: 'UserIdentity',
    render: function() {
      return (
        React.DOM.p({className: "user-identity"}, 
          this.props.displayName
        )
      );
    }
  });

  


  var RoomEntry = React.createClass({displayName: 'RoomEntry',
    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      room:       React.PropTypes.instanceOf(loop.store.Room).isRequired
    },

    getInitialState: function() {
      return { urlCopied: false };
    },

    shouldComponentUpdate: function(nextProps, nextState) {
      return (nextProps.room.ctime > this.props.room.ctime) ||
        (nextState.urlCopied !== this.state.urlCopied);
    },

    handleClickRoomUrl: function(event) {
      event.preventDefault();
      this.props.dispatcher.dispatch(new sharedActions.OpenRoom({
        roomToken: this.props.room.roomToken
      }));
    },

    handleCopyButtonClick: function(event) {
      event.preventDefault();
      this.props.dispatcher.dispatch(new sharedActions.CopyRoomUrl({
        roomUrl: this.props.room.roomUrl
      }));
      this.setState({urlCopied: true});
    },

    handleDeleteButtonClick: function(event) {
      event.preventDefault();
      
      this.props.dispatcher.dispatch(new sharedActions.DeleteRoom({
        roomToken: this.props.room.roomToken
      }));
    },

    handleMouseLeave: function(event) {
      this.setState({urlCopied: false});
    },

    _isActive: function() {
      return this.props.room.participants.length > 0;
    },

    render: function() {
      var room = this.props.room;
      var roomClasses = React.addons.classSet({
        "room-entry": true,
        "room-active": this._isActive()
      });
      var copyButtonClasses = React.addons.classSet({
        "copy-link": true,
        "checked": this.state.urlCopied
      });

      return (
        React.DOM.div({className: roomClasses, onMouseLeave: this.handleMouseLeave}, 
          React.DOM.h2(null, 
            React.DOM.span({className: "room-notification"}), 
            room.roomName, 
            React.DOM.button({className: copyButtonClasses, 
              onClick: this.handleCopyButtonClick}), 
            React.DOM.button({className: "delete-link", 
              onClick: this.handleDeleteButtonClick})
          ), 
          React.DOM.p(null, 
            React.DOM.a({href: "#", onClick: this.handleClickRoomUrl}, 
              room.roomUrl
            )
          )
        )
      );
    }
  });

  


  var RoomList = React.createClass({displayName: 'RoomList',
    mixins: [Backbone.Events],

    propTypes: {
      store: React.PropTypes.instanceOf(loop.store.RoomStore).isRequired,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      userDisplayName: React.PropTypes.string.isRequired  
    },

    getInitialState: function() {
      return this.props.store.getStoreState();
    },

    componentDidMount: function() {
      this.listenTo(this.props.store, "change", this._onStoreStateChanged);

      
      
      
      this.props.dispatcher.dispatch(new sharedActions.GetAllRooms());
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.store);
    },

    _onStoreStateChanged: function() {
      this.setState(this.props.store.getStoreState());
    },

    _getListHeading: function() {
      var numRooms = this.state.rooms.length;
      if (numRooms === 0) {
        return mozL10n.get("rooms_list_no_current_conversations");
      }
      return mozL10n.get("rooms_list_current_conversations", {num: numRooms});
    },

    _hasPendingOperation: function() {
      return this.state.pendingCreation || this.state.pendingInitialRetrieval;
    },

    handleCreateButtonClick: function() {
      this.props.dispatcher.dispatch(new sharedActions.CreateRoom({
        nameTemplate: mozL10n.get("rooms_default_room_name_template"),
        roomOwner: this.props.userDisplayName
      }));
    },

    render: function() {
      if (this.state.error) {
        
        console.error("RoomList error", this.state.error);
      }

      return (
        React.DOM.div({className: "rooms"}, 
          React.DOM.h1(null, this._getListHeading()), 
          React.DOM.div({className: "room-list"}, 
            this.state.rooms.map(function(room, i) {
              return RoomEntry({
                key: room.roomToken, 
                dispatcher: this.props.dispatcher, 
                room: room}
              );
            }, this)
          ), 
          React.DOM.p(null, 
            React.DOM.button({className: "btn btn-info", 
                    onClick: this.handleCreateButtonClick, 
                    disabled: this._hasPendingOperation()}, 
              mozL10n.get("rooms_new_room_button_label")
            )
          )
        )
      );
    }
  });

  


  var PanelView = React.createClass({displayName: 'PanelView',
    propTypes: {
      notifications: React.PropTypes.object.isRequired,
      client: React.PropTypes.object.isRequired,
      
      callUrl: React.PropTypes.string,
      userProfile: React.PropTypes.object,
      showTabButtons: React.PropTypes.bool,
      selectedTab: React.PropTypes.string,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      roomStore:
        React.PropTypes.instanceOf(loop.store.RoomStore).isRequired
    },

    getInitialState: function() {
      return {
        userProfile: this.props.userProfile || navigator.mozLoop.userProfile,
      };
    },

    _serviceErrorToShow: function() {
      if (!navigator.mozLoop.errors || !Object.keys(navigator.mozLoop.errors).length) {
        return null;
      }
      
      var firstErrorKey = Object.keys(navigator.mozLoop.errors)[0];
      return {
        type: firstErrorKey,
        error: navigator.mozLoop.errors[firstErrorKey],
      };
    },

    updateServiceErrors: function() {
      var serviceError = this._serviceErrorToShow();
      if (serviceError) {
        this.props.notifications.set({
          id: "service-error",
          level: "error",
          message: serviceError.error.friendlyMessage,
          details: serviceError.error.friendlyDetails,
          detailsButtonLabel: serviceError.error.friendlyDetailsButtonLabel,
          detailsButtonCallback: serviceError.error.friendlyDetailsButtonCallback,
        });
      } else {
        this.props.notifications.remove(this.props.notifications.get("service-error"));
      }
    },

    _roomsEnabled: function() {
      return navigator.mozLoop.getLoopBoolPref("rooms.enabled");
    },

    _onStatusChanged: function() {
      var profile = navigator.mozLoop.userProfile;
      var currUid = this.state.userProfile ? this.state.userProfile.uid : null;
      var newUid = profile ? profile.uid : null;
      if (currUid != newUid) {
        
        this.selectTab(this._roomsEnabled() ? "rooms" : "call");
        this.setState({userProfile: profile});
      }
      this.updateServiceErrors();
    },

    



    _renderRoomsOrCallTab: function() {
      if (!this._roomsEnabled()) {
        return (
          Tab({name: "call"}, 
            React.DOM.div({className: "content-area"}, 
              GettingStartedView(null), 
              CallUrlResult({client: this.props.client, 
                             notifications: this.props.notifications, 
                             callUrl: this.props.callUrl}), 
              ToSView(null)
            )
          )
        );
      }

      return (
        Tab({name: "rooms"}, 
          GettingStartedView(null), 
          RoomList({dispatcher: this.props.dispatcher, 
                    store: this.props.roomStore, 
                    userDisplayName: this._getUserDisplayName()}), 
          ToSView(null)
        )
      );
    },

    startForm: function(name, contact) {
      this.refs[name].initForm(contact);
      this.selectTab(name);
    },

    selectTab: function(name) {
      this.refs.tabView.setState({ selectedTab: name });
    },

    componentWillMount: function() {
      this.updateServiceErrors();
    },

    componentDidMount: function() {
      window.addEventListener("LoopStatusChanged", this._onStatusChanged);
    },

    componentWillUnmount: function() {
      window.removeEventListener("LoopStatusChanged", this._onStatusChanged);
    },

    _getUserDisplayName: function() {
      return this.state.userProfile && this.state.userProfile.email ||
             mozL10n.get("display_name_guest");
    },

    render: function() {
      var NotificationListView = sharedViews.NotificationListView;

      return (
        React.DOM.div(null, 
          NotificationListView({notifications: this.props.notifications, 
                                clearOnDocumentHidden: true}), 
          TabView({ref: "tabView", selectedTab: this.props.selectedTab, 
            buttonsHidden: !this.state.userProfile && !this.props.showTabButtons}, 
            this._renderRoomsOrCallTab(), 
            Tab({name: "contacts"}, 
              ContactsList({selectTab: this.selectTab, 
                            startForm: this.startForm})
            ), 
            Tab({name: "contacts_add", hidden: true}, 
              ContactDetailsForm({ref: "contacts_add", mode: "add", 
                                  selectTab: this.selectTab})
            ), 
            Tab({name: "contacts_edit", hidden: true}, 
              ContactDetailsForm({ref: "contacts_edit", mode: "edit", 
                                  selectTab: this.selectTab})
            ), 
            Tab({name: "contacts_import", hidden: true}, 
              ContactDetailsForm({ref: "contacts_import", mode: "import", 
                                  selectTab: this.selectTab})
            )
          ), 
          React.DOM.div({className: "footer"}, 
            React.DOM.div({className: "user-details"}, 
              UserIdentity({displayName: this._getUserDisplayName()}), 
              AvailabilityDropdown(null)
            ), 
            React.DOM.div({className: "signin-details"}, 
              AuthLink(null), 
              React.DOM.div({className: "footer-signin-separator"}), 
              SettingsDropdown(null)
            )
          )
        )
      );
    }
  });

  


  function init() {
    
    
    mozL10n.initialize(navigator.mozLoop);

    var client = new loop.Client();
    var notifications = new sharedModels.NotificationCollection();
    var dispatcher = new loop.Dispatcher();
    var roomStore = new loop.store.RoomStore(dispatcher, {
      mozLoop: navigator.mozLoop
    });

    React.renderComponent(PanelView({
      client: client, 
      notifications: notifications, 
      roomStore: roomStore, 
      dispatcher: dispatcher}
    ), document.querySelector("#main"));

    document.body.setAttribute("dir", mozL10n.getDirection());

    
    var evtObject = document.createEvent('Event');
    evtObject.initEvent('loopPanelInitialized', true, false);
    window.dispatchEvent(evtObject);
  }

  return {
    init: init,
    AuthLink: AuthLink,
    AvailabilityDropdown: AvailabilityDropdown,
    CallUrlResult: CallUrlResult,
    GettingStartedView: GettingStartedView,
    PanelView: PanelView,
    RoomEntry: RoomEntry,
    RoomList: RoomList,
    SettingsDropdown: SettingsDropdown,
    ToSView: ToSView,
    UserIdentity: UserIdentity,
  };
})(_, document.mozL10n);

document.addEventListener('DOMContentLoaded', loop.panel.init);
