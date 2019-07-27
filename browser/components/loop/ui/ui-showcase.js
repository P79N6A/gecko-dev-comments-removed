








(function() {
  "use strict";

  
  
  var PanelView = loop.panel.PanelView;
  
  var IncomingCallView = loop.conversation.IncomingCallView;

  
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

  var mockConversationModel = new loop.shared.models.ConversationModel({}, {sdk: {}});

  
  var mockNotifier = {};

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
              PanelView({client: mockClient, notifier: mockNotifier, 
                         callUrl: "http://invalid.example.url/"})
            ), 
            Example({summary: "Pending call url retrieval", dashed: "true", style: {width: "332px"}}, 
              PanelView({client: mockClient, notifier: mockNotifier})
            )
          ), 

          Section({name: "IncomingCallView"}, 
            Example({summary: "Default", dashed: "true", style: {width: "280px"}}, 
              IncomingCallView({model: mockConversationModel})
            )
          ), 

          Section({name: "ConversationToolbar"}, 
            React.DOM.h3(null, "Desktop Conversation Window"), 
            React.DOM.div({className: "conversation-window"}, 
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

            React.DOM.h3(null, "Standalone"), 
            React.DOM.div({className: "standalone"}, 
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
                                       notifier: mockNotifier})
              )
            )
          ), 

          Section({name: "ConversationView"}, 
            Example({summary: "Desktop conversation window", dashed: "true", 
                     style: {width: "260px", height: "265px"}}, 
              React.DOM.div({className: "conversation-window"}, 
                ConversationView({sdk: {}, 
                                  model: mockConversationModel, 
                                  video: {enabled: true}, 
                                  audio: {enabled: true}})
              )
            ), 
            Example({summary: "Standalone version"}, 
              React.DOM.div({className: "standalone"}, 
                ConversationView({sdk: {}, 
                                  model: mockConversationModel, 
                                  video: {enabled: true}, 
                                  audio: {enabled: true}})
              )
            ), 
            Example({summary: "Default"}, 
              ConversationView({sdk: {}, 
                                model: mockConversationModel, 
                                video: {enabled: true}, 
                                audio: {enabled: true}})
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
          )
        )
      );
    }
  });

  window.addEventListener("DOMContentLoaded", function() {
    var body = document.body;
    body.className = loop.shared.utils.getTargetPlatform();
    React.renderComponent(App(null), document.body);
  });
})();
