








(function() {
  "use strict";

  
  
  var PanelView = loop.panel.PanelView;
  
  var IncomingCallView = loop.conversation.IncomingCallView;

  
  var HomeView = loop.webapp.HomeView;
  var UnsupportedBrowserView = loop.webapp.UnsupportedBrowserView;
  var UnsupportedDeviceView = loop.webapp.UnsupportedDeviceView;
  var CallUrlExpiredView    = loop.webapp.CallUrlExpiredView;
  var StartConversationView = loop.webapp.StartConversationView;

  
  var ConversationToolbar = loop.shared.views.ConversationToolbar;
  var ConversationView = loop.shared.views.ConversationView;
  var FeedbackView = loop.shared.views.FeedbackView;

  
  function returnTrue() {
    return true;
  }

  function returnFalse() {
    return false;
  }

  function noop(){}

  
  
  var stageFeedbackApiClient = new loop.FeedbackAPIClient(
    "https://input.allizom.org/api/v1/feedback", {
      product: "Loop"
    }
  );

  

  var mockClient = {
    requestCallUrl: noop,
    requestCallUrlInfo: noop
  };

  var mockSDK = {};

  var mockConversationModel = new loop.shared.models.ConversationModel({}, {
    sdk: mockSDK
  });
  mockConversationModel.startSession = noop;

  var notifications = new loop.shared.models.NotificationCollection();
  var errNotifications = new loop.shared.models.NotificationCollection();
  errNotifications.error("Error!");

  var Example = React.createClass({displayName: 'Example',
    render: function() {
      var cx = React.addons.classSet;
      return (
        React.DOM.div({className: "example"}, 
          React.DOM.h3(null, this.props.summary), 
          React.DOM.div({className: cx({comp: true, dashed: this.props.dashed}), 
               style: this.props.style || {}}, 
            this.props.children
          )
        )
      );
    }
  });

  var Section = React.createClass({displayName: 'Section',
    render: function() {
      return (
        React.DOM.section({id: this.props.name}, 
          React.DOM.h1(null, this.props.name), 
          this.props.children
        )
      );
    }
  });

  var ShowCase = React.createClass({displayName: 'ShowCase',
    render: function() {
      return (
        React.DOM.div({className: "showcase"}, 
          React.DOM.header(null, 
            React.DOM.h1(null, "Loop UI Components Showcase"), 
            React.DOM.nav({className: "showcase-menu"}, 
              React.Children.map(this.props.children, function(section) {
                return (
                  React.DOM.a({className: "btn btn-info", href: "#" + section.props.name}, 
                    section.props.name
                  )
                );
              })
            )
          ), 
          this.props.children
        )
      );
    }
  });

  var App = React.createClass({displayName: 'App',
    render: function() {
      return (
        ShowCase(null, 
          Section({name: "PanelView"}, 
            React.DOM.p({className: "note"}, 
              React.DOM.strong(null, "Note:"), " 332px wide."
            ), 
            Example({summary: "Call URL retrieved", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: notifications, 
                         callUrl: "http://invalid.example.url/"})
            ), 
            Example({summary: "Call URL retrieved - authenticated", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: notifications, 
                         callUrl: "http://invalid.example.url/",
                         userProfile: {email: "test@example.com"}})
            ), 
            Example({summary: "Pending call url retrieval", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: notifications})
            ), 
            Example({summary: "Pending call url retrieval - authenticated", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: notifications,
                         userProfile: {email: "test@example.com"}})
            ), 
            Example({summary: "Error Notification", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: errNotifications})
            ),
            Example({summary: "Error Notification - authenticated", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: errNotifications,
                         userProfile: {email: "test@example.com"}})
            )
          ), 

          Section({name: "IncomingCallView"}, 
            Example({summary: "Default", dashed: "true", style: {width: "280px"}}, 
              React.DOM.div({className: "fx-embedded"}, 
                IncomingCallView({model: mockConversationModel})
              )
            )
          ), 

          Section({name: "IncomingCallView-ActiveState"}, 
            Example({summary: "Default", dashed: "true", style: {width: "280px"}}, 
              React.DOM.div({className: "fx-embedded"}, 
                IncomingCallView({model: mockConversationModel, showDeclineMenu: true})
              )
            )
          ), 

          Section({name: "ConversationToolbar"}, 
            React.DOM.h2(null, "Desktop Conversation Window"), 
            React.DOM.div({className: "fx-embedded override-position"}, 
              Example({summary: "Default (260x265)", dashed: "true"}, 
                ConversationToolbar({video: {enabled: true}, 
                                     audio: {enabled: true}, 
                                     hangup: noop, 
                                     publishStream: noop})
              ), 
              Example({summary: "Video muted"}, 
                ConversationToolbar({video: {enabled: false}, 
                                     audio: {enabled: true}, 
                                     hangup: noop, 
                                     publishStream: noop})
              ), 
              Example({summary: "Audio muted"}, 
                ConversationToolbar({video: {enabled: true}, 
                                     audio: {enabled: false}, 
                                     hangup: noop, 
                                     publishStream: noop})
              )
            ), 

            React.DOM.h2(null, "Standalone"), 
            React.DOM.div({className: "standalone override-position"}, 
              Example({summary: "Default"}, 
                ConversationToolbar({video: {enabled: true}, 
                                     audio: {enabled: true}, 
                                     hangup: noop, 
                                     publishStream: noop})
              ), 
              Example({summary: "Video muted"}, 
                ConversationToolbar({video: {enabled: false}, 
                                     audio: {enabled: true}, 
                                     hangup: noop, 
                                     publishStream: noop})
              ), 
              Example({summary: "Audio muted"}, 
                ConversationToolbar({video: {enabled: true}, 
                                     audio: {enabled: false}, 
                                     hangup: noop, 
                                     publishStream: noop})
              )
            )
          ), 

          Section({name: "StartConversationView"}, 
            Example({summary: "Start conversation view", dashed: "true"}, 
              React.DOM.div({className: "standalone"}, 
                StartConversationView({model: mockConversationModel, 
                                       client: mockClient, 
                                       notifications: notifications, 
                                       showCallOptionsMenu: true})
              )
            )
          ), 

          Section({name: "ConversationView"}, 
            Example({summary: "Desktop conversation window", dashed: "true", 
                     style: {width: "260px", height: "265px"}}, 
              React.DOM.div({className: "fx-embedded"}, 
                ConversationView({sdk: mockSDK, 
                                  model: mockConversationModel, 
                                  video: {enabled: true}, 
                                  audio: {enabled: true}})
              )
            ), 

            Example({summary: "Desktop conversation window large", dashed: "true"}, 
              React.DOM.div({className: "breakpoint", 'data-breakpoint-width': "800px", 
                'data-breakpoint-height': "600px"}, 
                React.DOM.div({className: "fx-embedded"}, 
                  ConversationView({sdk: mockSDK, 
                    video: {enabled: true}, 
                    audio: {enabled: true}, 
                    model: mockConversationModel})
                )
              )
            ), 

            Example({summary: "Desktop conversation window local audio stream", 
                     dashed: "true", style: {width: "260px", height: "265px"}}, 
              React.DOM.div({className: "fx-embedded"}, 
                ConversationView({sdk: mockSDK, 
                                  video: {enabled: false}, 
                                  audio: {enabled: true}, 
                                  model: mockConversationModel})
              )
            ), 

            Example({summary: "Standalone version"}, 
              React.DOM.div({className: "standalone"}, 
                ConversationView({sdk: mockSDK, 
                                  video: {enabled: true}, 
                                  audio: {enabled: true}, 
                                  model: mockConversationModel})
              )
            )
          ), 

          Section({name: "ConversationView-640"}, 
            Example({summary: "640px breakpoint for conversation view"}, 
              React.DOM.div({className: "breakpoint", 
                   style: {"text-align":"center"}, 
                   'data-breakpoint-width': "400px", 
                   'data-breakpoint-height': "780px"}, 
                React.DOM.div({className: "standalone"}, 
                  ConversationView({sdk: mockSDK, 
                                    video: {enabled: true}, 
                                    audio: {enabled: true}, 
                                    model: mockConversationModel})
                )
              )
            )
          ), 

          Section({name: "ConversationView-LocalAudio"}, 
            Example({summary: "Local stream is audio only"}, 
              React.DOM.div({className: "standalone"}, 
                ConversationView({sdk: mockSDK, 
                                  video: {enabled: false}, 
                                  audio: {enabled: true}, 
                                  model: mockConversationModel})
              )
            )
          ), 

          Section({name: "FeedbackView"}, 
            React.DOM.p({className: "note"}, 
              React.DOM.strong(null, "Note:"), " For the useable demo, you can access submitted data atÂ ", 
              React.DOM.a({href: "https://input.allizom.org/"}, "input.allizom.org"), "."
            ), 
            Example({summary: "Default (useable demo)", dashed: "true", style: {width: "280px"}}, 
              FeedbackView({feedbackApiClient: stageFeedbackApiClient})
            ), 
            Example({summary: "Detailed form", dashed: "true", style: {width: "280px"}}, 
              FeedbackView({feedbackApiClient: stageFeedbackApiClient, step: "form"})
            ), 
            Example({summary: "Thank you!", dashed: "true", style: {width: "280px"}}, 
              FeedbackView({feedbackApiClient: stageFeedbackApiClient, step: "finished"})
            )
          ), 

          Section({name: "CallUrlExpiredView"}, 
            Example({summary: "Firefox User"}, 
              CallUrlExpiredView({helper: {isFirefox: returnTrue}})
            ), 
            Example({summary: "Non-Firefox User"}, 
              CallUrlExpiredView({helper: {isFirefox: returnFalse}})
            )
          ), 

          Section({name: "AlertMessages"}, 
            Example({summary: "Various alerts"}, 
              React.DOM.div({className: "alert alert-warning"}, 
                React.DOM.button({className: "close"}), 
                React.DOM.p({className: "message"}, 
                  "The person you were calling has ended the conversation."
                )
              ), 
              React.DOM.br(null), 
              React.DOM.div({className: "alert alert-error"}, 
                React.DOM.button({className: "close"}), 
                React.DOM.p({className: "message"}, 
                  "The person you were calling has ended the conversation."
                )
              )
            )
          ), 

          Section({name: "HomeView"}, 
            Example({summary: "Standalone Home View"}, 
              React.DOM.div({className: "standalone"}, 
                HomeView(null)
              )
            )
          ), 


          Section({name: "UnsupportedBrowserView"}, 
            Example({summary: "Standalone Unsupported Browser"}, 
              React.DOM.div({className: "standalone"}, 
                UnsupportedBrowserView(null)
              )
            )
          ), 

          Section({name: "UnsupportedDeviceView"}, 
            Example({summary: "Standalone Unsupported Device"}, 
              React.DOM.div({className: "standalone"}, 
                UnsupportedDeviceView(null)
              )
            )
          )

        )
      );
    }
  });

  



  function _renderComponentsInIframes() {
    var parents = document.querySelectorAll('.breakpoint');
    [].forEach.call(parents, appendChildInIframe);

    




    function appendChildInIframe(parent) {
      var styles     = document.querySelector('head').children;
      var component  = parent.children[0];
      var iframe     = document.createElement('iframe');
      var width      = parent.dataset.breakpointWidth;
      var height     = parent.dataset.breakpointHeight;

      iframe.style.width  = width;
      iframe.style.height = height;

      parent.appendChild(iframe);
      iframe.src    = "about:blank";
      
      iframe.onload = function () {
        var iframeHead = iframe.contentDocument.querySelector('head');
        iframe.contentDocument.documentElement.querySelector('body')
                                              .appendChild(component);

        [].forEach.call(styles, function(style) {
          iframeHead.appendChild(style.cloneNode(true));
        });

      };
    }
  }

  window.addEventListener("DOMContentLoaded", function() {
    var body = document.body;
    body.className = loop.shared.utils.getTargetPlatform();

    React.renderComponent(App(null), body);

    _renderComponentsInIframes();
  });

})();
