








var loop = loop || {};
loop.panel = (function(_, mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views;
  var sharedModels = loop.shared.models;
  var sharedMixins = loop.shared.mixins;
  var __ = mozL10n.get; 

  



  var router;

  var TabView = React.createClass({displayName: 'TabView',
    getInitialState: function() {
      return {
        selectedTab: "call"
      };
    },

    handleSelectTab: function(event) {
      var tabName = event.target.dataset.tabName;
      this.setState({selectedTab: tabName});

      if (this.props.onSelect) {
        this.props.onSelect(tabName);
      }
    },

    render: function() {
      var cx = React.addons.classSet;
      var tabButtons = [];
      var tabs = [];
      React.Children.forEach(this.props.children, function(tab, i) {
        var tabName = tab.props.name;
        var isSelected = (this.state.selectedTab == tabName);
        tabButtons.push(
          React.DOM.li({className: cx({selected: isSelected}), 
              key: i, 
              'data-tab-name': tabName, 
              onClick: this.handleSelectTab}
          )
        );
        tabs.push(
          React.DOM.div({key: i, className: cx({tab: true, selected: isSelected})}, 
            tab.props.children
          )
        );
      }, this);
      return (
        React.DOM.div({className: "tab-view-container"}, 
          React.DOM.ul({className: "tab-view"}, tabButtons), 
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
                              __("display_name_dnd_status") :
                              __("display_name_available_status");

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
              React.DOM.span(null, __("display_name_available_status"))
            ), 
            React.DOM.li({onClick: this.changeAvailability("do-not-disturb"), 
                className: "dropdown-menu-item dnd-make-unavailable"}, 
              React.DOM.i({className: "status status-dnd"}), 
              React.DOM.span(null, __("display_name_dnd_status"))
            )
          )
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
        var terms_of_use_url = navigator.mozLoop.getLoopCharPref('legal.ToS_url');
        var privacy_notice_url = navigator.mozLoop.getLoopCharPref('legal.privacy_url');
        var tosHTML = __("legal_text_and_links3", {
          "clientShortname": __("client_shortname_fallback"),
          "terms_of_use": React.renderComponentToStaticMarkup(
            React.DOM.a({href: terms_of_use_url, target: "_blank"}, 
              __("legal_text_tos")
            )
          ),
          "privacy_notice": React.renderComponentToStaticMarkup(
            React.DOM.a({href: privacy_notice_url, target: "_blank"}, 
              __("legal_text_privacy")
            )
          ),
        });
        return React.DOM.p({className: "terms-service", 
                  dangerouslySetInnerHTML: {__html: tosHTML}});
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
      return (
        React.DOM.div({className: "settings-menu dropdown"}, 
          React.DOM.a({className: "btn btn-settings", onClick: this.showDropdownMenu, 
             title: __("settings_menu_button_tooltip")}), 
          React.DOM.ul({className: cx({"dropdown-menu": true, hide: !this.state.showMenu}), 
              onMouseLeave: this.hideDropdownMenu}, 
            SettingsDropdownEntry({label: __("settings_menu_item_settings"), 
                                   onClick: this.handleClickSettingsEntry, 
                                   icon: "settings"}), 
            SettingsDropdownEntry({label: __("settings_menu_item_account"), 
                                   onClick: this.handleClickAccountEntry, 
                                   icon: "account", 
                                   displayed: this._isSignedIn()}), 
            SettingsDropdownEntry({label: this._isSignedIn() ?
                                          __("settings_menu_item_signout") :
                                          __("settings_menu_item_signin"), 
                                   onClick: this.handleClickAuthEntry, 
                                   icon: this._isSignedIn() ? "signout" : "signin"})
          )
        )
      );
    }
  });

  


  var PanelLayout = React.createClass({displayName: 'PanelLayout',
    propTypes: {
      summary: React.PropTypes.string.isRequired
    },

    render: function() {
      return (
        React.DOM.div({className: "share generate-url"}, 
          React.DOM.div({className: "description"}, this.props.summary), 
          React.DOM.div({className: "action"}, 
            this.props.children
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

    




    conversationIdentifier: function() {
      return Math.random().toString(36).substring(5);
    },

    componentDidMount: function() {
      
      
      if (this.state.callUrl.length) {
        return;
      }

      this._fetchCallUrl();
    },

    


    _fetchCallUrl: function() {
      this.setState({pending: true});
      this.props.client.requestCallUrl(this.conversationIdentifier(),
                                       this._onCallUrlReceived);
    },

    _onCallUrlReceived: function(err, callUrlData) {
      this.props.notifications.reset();

      if (err) {
        this.props.notifications.errorL10n("unable_retrieve_url");
        this.setState(this.getInitialState());
      } else {
        try {
          var callUrl = new window.URL(callUrlData.callUrl);
          
          
          var token = callUrlData.callToken ||
                      callUrl.pathname.split('/').pop();

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

      navigator.mozLoop.composeEmail(__("share_email_subject3"),
        __("share_email_body3", { callUrl: this.state.callUrl }));
    },

    handleCopyButtonClick: function(event) {
      this.handleLinkExfiltration(event);
      
      
      navigator.mozLoop.copyString(this.state.callUrl);
      this.setState({copied: true});
    },

    handleLinkExfiltration: function(event) {
      
      if (this.state.callUrlExpiry) {
        navigator.mozLoop.noteCallUrlExpiry(this.state.callUrlExpiry);
      }
    },

    render: function() {
      
      
      
      
      var cx = React.addons.classSet;
      var inputCSSClass = cx({
        "pending": this.state.pending,
        
        
         "callUrl": !this.state.pending
      });
      return (
        PanelLayout({summary: __("share_link_header_text")}, 
          React.DOM.div({className: "invite"}, 
            React.DOM.input({type: "url", value: this.state.callUrl, readOnly: "true", 
                   onCopy: this.handleLinkExfiltration, 
                   className: inputCSSClass}), 
            React.DOM.p({className: "btn-group url-actions"}, 
              React.DOM.button({className: "btn btn-email", disabled: !this.state.callUrl, 
                onClick: this.handleEmailButtonClick}, 
                __("share_button")
              ), 
              React.DOM.button({className: "btn btn-copy", disabled: !this.state.callUrl, 
                onClick: this.handleCopyButtonClick}, 
                this.state.copied ? __("copied_url_button") :
                                     __("copy_url_button")
              )
            )
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
      if (navigator.mozLoop.loggedInToFxA) { 
        return null;
      }
      return (
        React.DOM.p({className: "signin-link"}, 
          React.DOM.a({href: "#", onClick: this.handleSignUpLinkClick}, 
            __("panel_footer_signin_or_signup_link")
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

  


  var PanelView = React.createClass({displayName: 'PanelView',
    propTypes: {
      notifications: React.PropTypes.object.isRequired,
      client: React.PropTypes.object.isRequired,
      
      callUrl: React.PropTypes.string,
      userProfile: React.PropTypes.object,
    },

    getInitialState: function() {
      return {
        userProfile: this.props.userProfile || navigator.mozLoop.userProfile,
      };
    },

    _onAuthStatusChange: function() {
      this.setState({userProfile: navigator.mozLoop.userProfile});
    },

    componentDidMount: function() {
      window.addEventListener("LoopStatusChanged", this._onAuthStatusChange);
    },

    componentWillUnmount: function() {
      window.removeEventListener("LoopStatusChanged", this._onAuthStatusChange);
    },

    render: function() {
      var NotificationListView = sharedViews.NotificationListView;
      var displayName = this.state.userProfile && this.state.userProfile.email ||
                        __("display_name_guest");
      return (
        React.DOM.div(null, 
          NotificationListView({notifications: this.props.notifications, 
                                clearOnDocumentHidden: true}), 
          TabView({onSelect: this.selectTab}, 
            Tab({name: "call"}, 
              CallUrlResult({client: this.props.client, 
                             notifications: this.props.notifications, 
                             callUrl: this.props.callUrl}), 
              ToSView(null)
            ), 
            Tab({name: "contacts"}, 
              React.DOM.span(null, "contacts")
            )
          ), 
          React.DOM.div({className: "footer"}, 
            React.DOM.div({className: "user-details"}, 
              UserIdentity({displayName: displayName}), 
              AvailabilityDropdown(null)
            ), 
            AuthLink(null), 
            SettingsDropdown(null)
          )
        )
      );
    }
  });

  


  function init() {
    
    
    mozL10n.initialize(navigator.mozLoop);

    var client = new loop.Client();
    var notifications = new sharedModels.NotificationCollection()

    React.renderComponent(PanelView({
      client: client, 
      notifications: notifications}), document.querySelector("#main"));

    document.body.classList.add(loop.shared.utils.getTargetPlatform());
    document.body.setAttribute("dir", mozL10n.getDirection());

    
    var evtObject = document.createEvent('Event');
    evtObject.initEvent('loopPanelInitialized', true, false);
    window.dispatchEvent(evtObject);
  }

  return {
    init: init,
    UserIdentity: UserIdentity,
    AvailabilityDropdown: AvailabilityDropdown,
    CallUrlResult: CallUrlResult,
    PanelView: PanelView,
    SettingsDropdown: SettingsDropdown,
    ToSView: ToSView
  };
})(_, document.mozL10n);

document.addEventListener('DOMContentLoaded', loop.panel.init);
