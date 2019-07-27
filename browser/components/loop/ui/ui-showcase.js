








(function() {
  "use strict";

  
  document.removeEventListener('DOMContentLoaded', loop.panel.init);
  document.removeEventListener('DOMContentLoaded', loop.conversation.init);

  
  
  var PanelView = loop.panel.PanelView;
  
  var IncomingCallView = loop.conversation.IncomingCallView;
  var DesktopPendingConversationView = loop.conversationViews.PendingConversationView;
  var CallFailedView = loop.conversationViews.CallFailedView;

  
  var HomeView = loop.webapp.HomeView;
  var UnsupportedBrowserView  = loop.webapp.UnsupportedBrowserView;
  var UnsupportedDeviceView   = loop.webapp.UnsupportedDeviceView;
  var CallUrlExpiredView      = loop.webapp.CallUrlExpiredView;
  var PendingConversationView = loop.webapp.PendingConversationView;
  var StartConversationView   = loop.webapp.StartConversationView;
  var FailedConversationView  = loop.webapp.FailedConversationView;
  var EndedConversationView   = loop.webapp.EndedConversationView;

  
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

  var dispatcher = new loop.Dispatcher();
  var roomListStore = new loop.store.RoomListStore({
    dispatcher: dispatcher,
    mozLoop: navigator.mozLoop
  });

  

  var mockContact = {
    name: ["Mr Smith"],
    email: [{
      value: "smith@invalid.com"
    }]
  };

  var mockClient = {
    requestCallUrl: noop,
    requestCallUrlInfo: noop
  };

  var mockSDK = {};

  var mockConversationModel = new loop.shared.models.ConversationModel({
    callerId: "Mrs Jones",
    urlCreationDate: (new Date() / 1000).toString()
  }, {
    sdk: mockSDK
  });
  mockConversationModel.startSession = noop;

  var mockWebSocket = new loop.CallConnectionWebSocket({
    url: "fake",
    callId: "fakeId",
    websocketToken: "fakeToken"
  });

  var notifications = new loop.shared.models.NotificationCollection();
  var errNotifications = new loop.shared.models.NotificationCollection();
  errNotifications.add({
    level: "error",
    message: "Could Not Authenticate",
    details: "Did you change your password?",
    detailsButtonLabel: "Retry",
  });

  var Example = React.createClass({displayName: 'Example',
    makeId: function(prefix) {
      return (prefix || "") + this.props.summary.toLowerCase().replace(/\s/g, "-");
    },

    render: function() {
      var cx = React.addons.classSet;
      return (
        React.DOM.div({className: "example"}, 
          React.DOM.h3({id: this.makeId()}, 
            this.props.summary, 
            React.DOM.a({href: this.makeId("#")}, " ¶")
          ), 
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
                         callUrl: "http://invalid.example.url/", 
                         dispatcher: dispatcher, 
                         roomListStore: roomListStore})
            ), 
            Example({summary: "Call URL retrieved - authenticated", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: notifications, 
                         callUrl: "http://invalid.example.url/", 
                         userProfile: {email: "test@example.com"}, 
                         dispatcher: dispatcher, 
                         roomListStore: roomListStore})
            ), 
            Example({summary: "Pending call url retrieval", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: notifications, 
                         dispatcher: dispatcher, 
                         roomListStore: roomListStore})
            ), 
            Example({summary: "Pending call url retrieval - authenticated", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: notifications, 
                         userProfile: {email: "test@example.com"}, 
                         dispatcher: dispatcher, 
                         roomListStore: roomListStore})
            ), 
            Example({summary: "Error Notification", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: errNotifications, 
                         dispatcher: dispatcher, 
                         roomListStore: roomListStore})
            ), 
            Example({summary: "Error Notification - authenticated", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: errNotifications, 
                         userProfile: {email: "test@example.com"}, 
                         dispatcher: dispatcher, 
                         roomListStore: roomListStore})
            ), 
            Example({summary: "Room list tab", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifications: notifications, 
                         userProfile: {email: "test@example.com"}, 
                         dispatcher: dispatcher, 
                         roomListStore: roomListStore, 
                         selectedTab: "rooms"})
            )
          ), 

          Section({name: "IncomingCallView"}, 
            Example({summary: "Default / incoming video call", dashed: "true", style: {width: "260px", height: "254px"}}, 
              React.DOM.div({className: "fx-embedded"}, 
                IncomingCallView({model: mockConversationModel, 
                                  video: true})
              )
            ), 

            Example({summary: "Default / incoming audio only call", dashed: "true", style: {width: "260px", height: "254px"}}, 
              React.DOM.div({className: "fx-embedded"}, 
                IncomingCallView({model: mockConversationModel, 
                                  video: false})
              )
            )
          ), 

          Section({name: "IncomingCallView-ActiveState"}, 
            Example({summary: "Default", dashed: "true", style: {width: "260px", height: "254px"}}, 
              React.DOM.div({className: "fx-embedded"}, 
                IncomingCallView({model: mockConversationModel, 
                                   showMenu: true})
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

          Section({name: "PendingConversationView"}, 
            Example({summary: "Pending conversation view (connecting)", dashed: "true"}, 
              React.DOM.div({className: "standalone"}, 
                PendingConversationView({websocket: mockWebSocket, 
                                         dispatcher: dispatcher})
              )
            ), 
            Example({summary: "Pending conversation view (ringing)", dashed: "true"}, 
              React.DOM.div({className: "standalone"}, 
                PendingConversationView({websocket: mockWebSocket, 
                                         dispatcher: dispatcher, 
                                         callState: "ringing"})
              )
            )
          ), 

          Section({name: "PendingConversationView (Desktop)"}, 
            Example({summary: "Connecting", dashed: "true", 
                     style: {width: "260px", height: "265px"}}, 
              React.DOM.div({className: "fx-embedded"}, 
                DesktopPendingConversationView({callState: "gather", 
                                                contact: mockContact, 
                                                dispatcher: dispatcher})
              )
            )
          ), 

          Section({name: "CallFailedView"}, 
            Example({summary: "Call Failed", dashed: "true", 
                     style: {width: "260px", height: "265px"}}, 
              React.DOM.div({className: "fx-embedded"}, 
                CallFailedView({dispatcher: dispatcher})
              )
            ), 
            Example({summary: "Call Failed — with call URL error", dashed: "true", 
                     style: {width: "260px", height: "265px"}}, 
              React.DOM.div({className: "fx-embedded"}, 
                CallFailedView({dispatcher: dispatcher, emailLinkError: true})
              )
            )
          ), 

          Section({name: "StartConversationView"}, 
            Example({summary: "Start conversation view", dashed: "true"}, 
              React.DOM.div({className: "standalone"}, 
                StartConversationView({conversation: mockConversationModel, 
                                       client: mockClient, 
                                       notifications: notifications})
              )
            )
          ), 

          Section({name: "FailedConversationView"}, 
            Example({summary: "Failed conversation view", dashed: "true"}, 
              React.DOM.div({className: "standalone"}, 
                FailedConversationView({conversation: mockConversationModel, 
                                        client: mockClient, 
                                        notifications: notifications})
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
              React.DOM.strong(null, "Note:"), " For the useable demo, you can access submitted data at ", 
              React.DOM.a({href: "https://input.allizom.org/"}, "input.allizom.org"), "."
            ), 
            Example({summary: "Default (useable demo)", dashed: "true", style: {width: "260px"}}, 
              FeedbackView({feedbackApiClient: stageFeedbackApiClient})
            ), 
            Example({summary: "Detailed form", dashed: "true", style: {width: "260px"}}, 
              FeedbackView({feedbackApiClient: stageFeedbackApiClient, step: "form"})
            ), 
            Example({summary: "Thank you!", dashed: "true", style: {width: "260px"}}, 
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

          Section({name: "EndedConversationView"}, 
            Example({summary: "Displays the feedback form"}, 
              React.DOM.div({className: "standalone"}, 
                EndedConversationView({sdk: mockSDK, 
                                       video: {enabled: true}, 
                                       audio: {enabled: true}, 
                                       conversation: mockConversationModel, 
                                       feedbackApiClient: stageFeedbackApiClient, 
                                       onAfterFeedbackReceived: noop})
              )
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
    React.renderComponent(App(null), document.body);

    _renderComponentsInIframes();

    
    document.title = "Loop UI Components Showcase";
  });

})();
