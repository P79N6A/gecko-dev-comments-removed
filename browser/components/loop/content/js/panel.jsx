/** @jsx React.DOM */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/*jshint newcap:false*/
/*global loop:true, React */

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
  var __ = mozL10n.get; // aliasing translation function as __ for concision

  var TabView = React.createClass({
    propTypes: {
      buttonsHidden: React.PropTypes.bool,
      // The selectedTab prop is used by the UI showcase.
      selectedTab: React.PropTypes.string
    },

    getDefaultProps: function() {
      return {
        buttonsHidden: false,
        selectedTab: "call"
      };
    },

    getInitialState: function() {
      return {selectedTab: this.props.selectedTab};
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
        // Filter out null tabs (eg. rooms when the feature is disabled)
        if (!tab) {
          return;
        }
        var tabName = tab.props.name;
        var isSelected = (this.state.selectedTab == tabName);
        if (!tab.props.hidden) {
          tabButtons.push(
            <li className={cx({selected: isSelected})}
                key={i}
                data-tab-name={tabName}
                onClick={this.handleSelectTab} />
          );
        }
        tabs.push(
          <div key={i} className={cx({tab: true, selected: isSelected})}>
            {tab.props.children}
          </div>
        );
      }, this);
      return (
        <div className="tab-view-container">
          {!this.props.buttonsHidden
            ? <ul className="tab-view">{tabButtons}</ul>
            : null}
          {tabs}
        </div>
      );
    }
  });

  var Tab = React.createClass({
    render: function() {
      return null;
    }
  });

  /**
   * Availability drop down menu subview.
   */
  var AvailabilityDropdown = React.createClass({
    mixins: [sharedMixins.DropdownMenuMixin],

    getInitialState: function() {
      return {
        doNotDisturb: navigator.mozLoop.doNotDisturb
      };
    },

    // XXX target event can either be the li, the span or the i tag
    // this makes it easier to figure out the target by making a
    // closure with the desired status already passed in.
    changeAvailability: function(newAvailabilty) {
      return function(event) {
        // Note: side effect!
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
      // XXX https://github.com/facebook/react/issues/310 for === htmlFor
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
                              __("display_name_dnd_status") :
                              __("display_name_available_status");

      return (
        <div className="dropdown">
          <p className="dnd-status" onClick={this.showDropdownMenu}>
            <span>{availabilityText}</span>
            <i className={availabilityStatus}></i>
          </p>
          <ul className={availabilityDropdown}
              onMouseLeave={this.hideDropdownMenu}>
            <li onClick={this.changeAvailability("available")}
                className="dropdown-menu-item dnd-make-available">
              <i className="status status-available"></i>
              <span>{__("display_name_available_status")}</span>
            </li>
            <li onClick={this.changeAvailability("do-not-disturb")}
                className="dropdown-menu-item dnd-make-unavailable">
              <i className="status status-dnd"></i>
              <span>{__("display_name_dnd_status")}</span>
            </li>
          </ul>
        </div>
      );
    }
  });

  var ToSView = React.createClass({
    getInitialState: function() {
      return {seenToS: navigator.mozLoop.getLoopCharPref('seenToS')};
    },

    render: function() {
      if (this.state.seenToS == "unseen") {
        var terms_of_use_url = navigator.mozLoop.getLoopCharPref('legal.ToS_url');
        var privacy_notice_url = navigator.mozLoop.getLoopCharPref('legal.privacy_url');
        var tosHTML = __("legal_text_and_links3", {
          "clientShortname": __("clientShortname2"),
          "terms_of_use": React.renderComponentToStaticMarkup(
            <a href={terms_of_use_url} target="_blank">
              {__("legal_text_tos")}
            </a>
          ),
          "privacy_notice": React.renderComponentToStaticMarkup(
            <a href={privacy_notice_url} target="_blank">
              {__("legal_text_privacy")}
            </a>
          ),
        });
        return <p className="terms-service"
                  dangerouslySetInnerHTML={{__html: tosHTML}}></p>;
      } else {
        return <div />;
      }
    }
  });

  /**
   * Panel settings (gear) menu entry.
   */
  var SettingsDropdownEntry = React.createClass({
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
        <li onClick={this.props.onClick} className="dropdown-menu-item">
          {this.props.icon ?
            <i className={"icon icon-" + this.props.icon}></i> :
            null}
          <span>{this.props.label}</span>
        </li>
      );
    }
  });

  /**
   * Panel settings (gear) menu.
   */
  var SettingsDropdown = React.createClass({
    mixins: [sharedMixins.DropdownMenuMixin],

    handleClickSettingsEntry: function() {
      // XXX to be implemented at the same time as unhiding the entry
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

      // For now all of the menu entries require FxA so hide the whole gear if FxA is disabled.
      if (!navigator.mozLoop.fxAEnabled) {
        return null;
      }

      return (
        <div className="settings-menu dropdown">
          <a className="button-settings" onClick={this.showDropdownMenu}
             title={__("settings_menu_button_tooltip")} />
          <ul className={cx({"dropdown-menu": true, hide: !this.state.showMenu})}
              onMouseLeave={this.hideDropdownMenu}>
            <SettingsDropdownEntry label={__("settings_menu_item_settings")}
                                   onClick={this.handleClickSettingsEntry}
                                   displayed={false}
                                   icon="settings" />
            <SettingsDropdownEntry label={__("settings_menu_item_account")}
                                   onClick={this.handleClickAccountEntry}
                                   icon="account"
                                   displayed={this._isSignedIn()} />
            <SettingsDropdownEntry label={this._isSignedIn() ?
                                          __("settings_menu_item_signout") :
                                          __("settings_menu_item_signin")}
                                   onClick={this.handleClickAuthEntry}
                                   displayed={navigator.mozLoop.fxAEnabled}
                                   icon={this._isSignedIn() ? "signout" : "signin"} />
          </ul>
        </div>
      );
    }
  });

  /**
   * Call url result view.
   */
  var CallUrlResult = React.createClass({
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

    /**
     * Provided by DocumentVisibilityMixin. Schedules retrieval of a new call
     * URL everytime the panel is reopened.
     */
    onDocumentVisible: function() {
      this._fetchCallUrl();
    },

    componentDidMount: function() {
      // If we've already got a callURL, don't bother requesting a new one.
      // As of this writing, only used for visual testing in the UI showcase.
      if (this.state.callUrl.length) {
        return;
      }

      this._fetchCallUrl();
    },

    /**
     * Fetches a call URL.
     */
    _fetchCallUrl: function() {
      this.setState({pending: true});
      // XXX This is an empty string as a conversation identifier. Bug 1015938 implements
      // a user-set string.
      this.props.client.requestCallUrl("",
                                       this._onCallUrlReceived);
    },

    _onCallUrlReceived: function(err, callUrlData) {
      if (err) {
        if (err.code != 401) {
          // 401 errors are already handled in hawkRequest and show an error
          // message about the session.
          this.props.notifications.errorL10n("unable_retrieve_url");
        }
        this.setState(this.getInitialState());
      } else {
        try {
          var callUrl = new window.URL(callUrlData.callUrl);
          // XXX the current server vers does not implement the callToken field
          // but it exists in the API. This workaround should be removed in the future
          var token = callUrlData.callToken ||
                      callUrl.pathname.split('/').pop();

          // Now that a new URL is available, indicate it has not been shared.
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
      // XXX the mozLoop object should be passed as a prop, to ease testing and
      //     using a fake implementation in UI components showcase.
      navigator.mozLoop.copyString(this.state.callUrl);
      this.setState({copied: true});
    },

    linkExfiltrated: false,

    handleLinkExfiltration: function(event) {
      // Update the count of shared URLs only once per generated URL.
      if (!this.linkExfiltrated) {
        this.linkExfiltrated = true;
        try {
          navigator.mozLoop.telemetryAdd("LOOP_CLIENT_CALL_URL_SHARED", true);
        } catch (err) {
          console.error("Error recording telemetry", err);
        }
      }

      // Note URL expiration every time it is shared.
      if (this.state.callUrlExpiry) {
        navigator.mozLoop.noteCallUrlExpiry(this.state.callUrlExpiry);
      }
    },

    render: function() {
      // XXX setting elem value from a state (in the callUrl input)
      // makes it immutable ie read only but that is fine in our case.
      // readOnly attr will suppress a warning regarding this issue
      // from the react lib.
      var cx = React.addons.classSet;
      var inputCSSClass = cx({
        "pending": this.state.pending,
        // Used in functional testing, signals that
        // call url was received from loop server
        "callUrl": !this.state.pending
      });
      return (
        <div className="generate-url">
          <header>{__("share_link_header_text")}</header>
          <input type="url" value={this.state.callUrl} readOnly="true"
                 onCopy={this.handleLinkExfiltration}
                 className={inputCSSClass} />
          <ButtonGroup additionalClass="url-actions">
            <Button additionalClass="button-email"
                    disabled={!this.state.callUrl}
                    onClick={this.handleEmailButtonClick}
                    caption={mozL10n.get("share_button")} />
            <Button additionalClass="button-copy"
                    disabled={!this.state.callUrl}
                    onClick={this.handleCopyButtonClick}
                    caption={this.state.copied ? mozL10n.get("copied_url_button") :
                                                 mozL10n.get("copy_url_button")} />
          </ButtonGroup>
        </div>
      );
    }
  });

  /**
   * FxA sign in/up link component.
   */
  var AuthLink = React.createClass({
    handleSignUpLinkClick: function() {
      navigator.mozLoop.logInToFxA();
    },

    render: function() {
      if (!navigator.mozLoop.fxAEnabled || navigator.mozLoop.userProfile) {
        return null;
      }
      return (
        <p className="signin-link">
          <a href="#" onClick={this.handleSignUpLinkClick}>
            {__("panel_footer_signin_or_signup_link")}
          </a>
        </p>
      );
    }
  });

  /**
   * FxA user identity (guest/authenticated) component.
   */
  var UserIdentity = React.createClass({
    render: function() {
      return (
        <p className="user-identity">
          {this.props.displayName}
        </p>
      );
    }
  });

  /**
   * Room list entry.
   */
  var RoomEntry = React.createClass({
    propTypes: {
      openRoom: React.PropTypes.func.isRequired,
      room:     React.PropTypes.instanceOf(loop.store.Room).isRequired
    },

    shouldComponentUpdate: function(nextProps, nextState) {
      return nextProps.room.ctime > this.props.room.ctime;
    },

    handleClickRoom: function(event) {
      event.preventDefault();
      this.props.openRoom(this.props.room);
    },

    _isActive: function() {
      // XXX bug 1074679 will implement this properly
      return this.props.room.currSize > 0;
    },

    render: function() {
      var room = this.props.room;
      var roomClasses = React.addons.classSet({
        "room-entry": true,
        "room-active": this._isActive()
      });

      return (
        <div className={roomClasses}>
          <h2>
            <span className="room-notification" />
            {room.roomName}
          </h2>
          <p>
            <a ref="room" href="#" onClick={this.handleClickRoom}>
              {room.roomUrl}
            </a>
          </p>
        </div>
      );
    }
  });

  /**
   * Room list.
   */
  var RoomList = React.createClass({
    mixins: [Backbone.Events],

    propTypes: {
      store: React.PropTypes.instanceOf(loop.store.RoomListStore).isRequired,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      rooms: React.PropTypes.array
    },

    getInitialState: function() {
      var storeState = this.props.store.getStoreState();
      return {
        error: this.props.error || storeState.error,
        rooms: this.props.rooms || storeState.rooms,
      };
    },

    componentWillMount: function() {
      this.listenTo(this.props.store, "change", this._onRoomListChanged);

      this.props.dispatcher.dispatch(new sharedActions.GetAllRooms());
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.store);
    },

    _onRoomListChanged: function() {
      this.setState(this.props.store.getStoreState());
    },

    _getListHeading: function() {
      var numRooms = this.state.rooms.length;
      if (numRooms === 0) {
        return mozL10n.get("rooms_list_no_current_conversations");
      }
      return mozL10n.get("rooms_list_current_conversations", {num: numRooms});
    },

    openRoom: function(room) {
      // XXX implement me; see bug 1074678
    },

    render: function() {
      if (this.state.error) {
        // XXX Better end user reporting of errors.
        console.error(this.state.error);
      }

      return (
        <div className="room-list">
          <h1>{this._getListHeading()}</h1>
          {
            this.state.rooms.map(function(room, i) {
              return <RoomEntry key={i} room={room} openRoom={this.openRoom} />;
            }, this)
          }
        </div>
      );
    }
  });

  /**
   * Panel view.
   */
  var PanelView = React.createClass({
    propTypes: {
      notifications: React.PropTypes.object.isRequired,
      client: React.PropTypes.object.isRequired,
      // Mostly used for UI components showcase and unit tests
      callUrl: React.PropTypes.string,
      userProfile: React.PropTypes.object,
      showTabButtons: React.PropTypes.bool,
      selectedTab: React.PropTypes.string,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      roomListStore:
        React.PropTypes.instanceOf(loop.store.RoomListStore).isRequired
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
      // Just get the first error for now since more than one should be rare.
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
        });
      } else {
        this.props.notifications.remove(this.props.notifications.get("service-error"));
      }
    },

    _onStatusChanged: function() {
      var profile = navigator.mozLoop.userProfile;
      var currUid = this.state.userProfile ? this.state.userProfile.uid : null;
      var newUid = profile ? profile.uid : null;
      if (currUid != newUid) {
        // On profile change (login, logout), switch back to the default tab.
        this.selectTab("call");
      }
      this.setState({userProfile: profile});
      this.updateServiceErrors();
    },

    /**
     * The rooms feature is hidden by default for now. Once it gets mainstream,
     * this method can be safely removed.
     */
    _renderRoomsTab: function() {
      if (!navigator.mozLoop.getLoopBoolPref("rooms.enabled")) {
        return null;
      }
      return (
        <Tab name="rooms">
          <RoomList dispatcher={this.props.dispatcher}
                    store={this.props.roomListStore} />
        </Tab>
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

    render: function() {
      var NotificationListView = sharedViews.NotificationListView;
      var displayName = this.state.userProfile && this.state.userProfile.email ||
                        __("display_name_guest");
      return (
        <div>
          <NotificationListView notifications={this.props.notifications}
                                clearOnDocumentHidden={true} />
          <TabView ref="tabView" selectedTab={this.props.selectedTab}
            buttonsHidden={!this.state.userProfile && !this.props.showTabButtons}>
            <Tab name="call">
              <div className="content-area">
                <CallUrlResult client={this.props.client}
                               notifications={this.props.notifications}
                               callUrl={this.props.callUrl} />
                <ToSView />
              </div>
            </Tab>
            {this._renderRoomsTab()}
            <Tab name="contacts">
              <ContactsList selectTab={this.selectTab}
                            startForm={this.startForm} />
            </Tab>
            <Tab name="contacts_add" hidden={true}>
              <ContactDetailsForm ref="contacts_add" mode="add"
                                  selectTab={this.selectTab} />
            </Tab>
            <Tab name="contacts_edit" hidden={true}>
              <ContactDetailsForm ref="contacts_edit" mode="edit"
                                  selectTab={this.selectTab} />
            </Tab>
            <Tab name="contacts_import" hidden={true}>
              <ContactDetailsForm ref="contacts_import" mode="import"
                                  selectTab={this.selectTab}/>
            </Tab>
          </TabView>
          <div className="footer">
            <div className="user-details">
              <UserIdentity displayName={displayName} />
              <AvailabilityDropdown />
            </div>
            <div className="signin-details">
              <AuthLink />
              <div className="footer-signin-separator" />
              <SettingsDropdown />
            </div>
          </div>
        </div>
      );
    }
  });

  /**
   * Panel initialisation.
   */
  function init() {
    // Do the initial L10n setup, we do this before anything
    // else to ensure the L10n environment is setup correctly.
    mozL10n.initialize(navigator.mozLoop);

    var client = new loop.Client();
    var notifications = new sharedModels.NotificationCollection();
    var dispatcher = new loop.Dispatcher();
    var roomListStore = new loop.store.RoomListStore({
      mozLoop: navigator.mozLoop,
      dispatcher: dispatcher
    });

    React.renderComponent(<PanelView
      client={client}
      notifications={notifications}
      roomListStore={roomListStore}
      dispatcher={dispatcher}
    />, document.querySelector("#main"));

    document.body.setAttribute("dir", mozL10n.getDirection());

    // Notify the window that we've finished initalization and initial layout
    var evtObject = document.createEvent('Event');
    evtObject.initEvent('loopPanelInitialized', true, false);
    window.dispatchEvent(evtObject);
  }

  return {
    init: init,
    UserIdentity: UserIdentity,
    AuthLink: AuthLink,
    AvailabilityDropdown: AvailabilityDropdown,
    CallUrlResult: CallUrlResult,
    PanelView: PanelView,
    RoomList: RoomList,
    SettingsDropdown: SettingsDropdown,
    ToSView: ToSView
  };
})(_, document.mozL10n);

document.addEventListener('DOMContentLoaded', loop.panel.init);
