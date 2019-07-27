








var loop = loop || {};
loop.panel = (function(_, mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views,
      
      __ = mozL10n.get;

  



  var router;

  



  var DropdownMenuMixin = {
    getInitialState: function() {
      return {showMenu: false};
    },

    _onBodyClick: function() {
      this.setState({showMenu: false});
    },

    componentDidMount: function() {
      document.body.addEventListener("click", this._onBodyClick);
    },

    componentWillUnmount: function() {
      document.body.removeEventListener("click", this._onBodyClick);
    },

    showDropdownMenu: function() {
      this.setState({showMenu: true});
    },

    hideDropdownMenu: function() {
      this.setState({showMenu: false});
    }
  };

  


  var AvailabilityDropdown = React.createClass({displayName: 'AvailabilityDropdown',
    mixins: [DropdownMenuMixin],

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
    mixins: [DropdownMenuMixin],

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
      
      return !!navigator.mozLoop.loggedInToFxA;
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
    propTypes: {
      callUrl:        React.PropTypes.string,
      callUrlExpiry:  React.PropTypes.number,
      notifier:       React.PropTypes.object.isRequired,
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

    




    conversationIdentifier: function() {
      return Math.random().toString(36).substring(5);
    },

    componentDidMount: function() {
      
      
      if (this.state.callUrl.length) {
        return;
      }

      this.setState({pending: true});
      this.props.client.requestCallUrl(this.conversationIdentifier(),
                                       this._onCallUrlReceived);
    },

    _onCallUrlReceived: function(err, callUrlData) {
      this.props.notifier.clear();

      if (err) {
        this.props.notifier.errorL10n("unable_retrieve_url");
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
          this.props.notifier.errorL10n("unable_retrieve_url");
          this.setState(this.getInitialState());
        }
      }
    },

    _generateMailTo: function() {
      return encodeURI([
        "mailto:?subject=" + __("share_email_subject3") + "&",
        "body=" + __("share_email_body3", {callUrl: this.state.callUrl})
      ].join(""));
    },

    handleEmailButtonClick: function(event) {
      this.handleLinkExfiltration(event);
      
      document.location = event.target.dataset.mailto;
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
                onClick: this.handleEmailButtonClick, 
                'data-mailto': this._generateMailTo()}, 
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

  


  var PanelView = React.createClass({displayName: 'PanelView',
    propTypes: {
      notifier: React.PropTypes.object.isRequired,
      client: React.PropTypes.object.isRequired,
      
      callUrl: React.PropTypes.string
    },

    render: function() {
      return (
        React.DOM.div(null, 
          CallUrlResult({client: this.props.client, 
                         notifier: this.props.notifier, 
                         callUrl: this.props.callUrl}), 
          ToSView(null), 
          React.DOM.div({className: "footer"}, 
            AvailabilityDropdown(null), 
            AuthLink(null), 
            SettingsDropdown(null)
          )
        )
      );
    }
  });

  var PanelRouter = loop.desktopRouter.DesktopRouter.extend({
    



    document: undefined,

    routes: {
      "": "home"
    },

    initialize: function(options) {
      options = options || {};
      if (!options.document) {
        throw new Error("missing required document");
      }
      this.document = options.document;

      this._registerVisibilityChangeEvent();

      this.on("panel:open panel:closed", this.clearNotifications, this);
      this.on("panel:open", this.reset, this);
    },

    








    _registerVisibilityChangeEvent: function() {
      
      
      this.document.addEventListener("visibilitychange", function(event) {
        this.trigger(event.currentTarget.hidden ? "panel:closed"
                                                : "panel:open");
      }.bind(this));
    },

    


    home: function() {
      this.reset();
    },

    clearNotifications: function() {
      this._notifier.clear();
    },

    


    reset: function() {
      this._notifier.clear();
      var client = new loop.Client({
        baseServerUrl: navigator.mozLoop.serverUrl
      });
      this.loadReactComponent(PanelView({client: client, 
                                         notifier: this._notifier}));
    }
  });

  


  function init() {
    
    
    mozL10n.initialize(navigator.mozLoop);

    router = new PanelRouter({
      document: document,
      notifier: new sharedViews.NotificationListView({el: "#messages"})
    });
    Backbone.history.start();

    document.body.classList.add(loop.shared.utils.getTargetPlatform());
    document.body.setAttribute("dir", mozL10n.getDirection());

    
    var evtObject = document.createEvent('Event');
    evtObject.initEvent('loopPanelInitialized', true, false);
    window.dispatchEvent(evtObject);
  }

  return {
    init: init,
    AvailabilityDropdown: AvailabilityDropdown,
    CallUrlResult: CallUrlResult,
    PanelView: PanelView,
    PanelRouter: PanelRouter,
    SettingsDropdown: SettingsDropdown,
    ToSView: ToSView
  };
})(_, document.mozL10n);
