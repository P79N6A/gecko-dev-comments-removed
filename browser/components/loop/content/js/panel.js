








var loop = loop || {};
loop.panel = (function(_, mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views,
      
      __ = mozL10n.get;

  



  var router;

  


  var AvailabilityDropdown = React.createClass({displayName: 'AvailabilityDropdown',
    getInitialState: function() {
      return {
        doNotDisturb: navigator.mozLoop.doNotDisturb,
        showMenu: false
      };
    },

    showDropdownMenu: function() {
      this.setState({showMenu: true});
    },

    hideDropdownMenu: function() {
      this.setState({showMenu: false});
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
        'dnd-menu': true,
        'hide': !this.state.showMenu
      });
      var availabilityText = this.state.doNotDisturb ?
                              __("display_name_dnd_status") :
                              __("display_name_available_status");

      return (
        React.DOM.div({className: "do-not-disturb"}, 
          React.DOM.p({className: "dnd-status", onClick: this.showDropdownMenu}, 
            React.DOM.span(null, availabilityText), 
            React.DOM.i({className: availabilityStatus})
          ), 
          React.DOM.ul({className: availabilityDropdown, 
              onMouseLeave: this.hideDropdownMenu}, 
            React.DOM.li({onClick: this.changeAvailability("available"), 
                className: "dnd-menu-item dnd-make-available"}, 
              React.DOM.i({className: "status status-available"}), 
              React.DOM.span(null, __("display_name_available_status"))
            ), 
            React.DOM.li({onClick: this.changeAvailability("do-not-disturb"), 
                className: "dnd-menu-item dnd-make-unavailable"}, 
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
        var tosHTML = __("legal_text_and_links2", {
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
      callUrl:  React.PropTypes.string,
      notifier: React.PropTypes.object.isRequired,
      client:   React.PropTypes.object.isRequired
    },

    getInitialState: function() {
      return {
        pending: false,
        copied: false,
        callUrl: this.props.callUrl || ""
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

          this.setState({pending: false, copied: false, callUrl: callUrl.href});
        } catch(e) {
          console.log(e);
          this.props.notifier.errorL10n("unable_retrieve_url");
          this.setState(this.getInitialState());
        }
      }
    },

    _generateMailTo: function() {
      return encodeURI([
        "mailto:?subject=" + __("share_email_subject2") + "&",
        "body=" + __("share_email_body2", {callUrl: this.state.callUrl})
      ].join(""));
    },

    handleEmailButtonClick: function(event) {
      
      document.location = event.target.dataset.mailto;
    },

    handleCopyButtonClick: function(event) {
      
      
      navigator.mozLoop.copyString(this.state.callUrl);
      this.setState({copied: true});
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

  


  var PanelView = React.createClass({displayName: 'PanelView',
    propTypes: {
      notifier: React.PropTypes.object.isRequired,
      client: React.PropTypes.object.isRequired,
      
      callUrl: React.PropTypes.string
    },

    handleSignUpLinkClick: function() {
      navigator.mozLoop.logInToFxA();
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
            React.DOM.a({className: "signin-link", href: "#", onClick: this.handleSignUpLinkClick}, 
              __("panel_footer_signin_or_signup_link")
            )
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
    ToSView: ToSView
  };
})(_, document.mozL10n);
