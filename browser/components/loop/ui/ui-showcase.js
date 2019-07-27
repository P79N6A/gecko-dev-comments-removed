








(function() {
  "use strict";

  
  
  var PanelView = loop.panel.PanelView;
  
  var IncomingCallView = loop.conversation.IncomingCallView;

  
  var CallUrlExpiredView = loop.webapp.CallUrlExpiredView;

  
  var ConversationToolbar = loop.shared.views.ConversationToolbar;
  var ConversationView = loop.shared.views.ConversationView;
  var FeedbackView = loop.shared.views.FeedbackView;

  
  function returnTrue() {
    return true;
  }

  function returnFalse() {
    return false;
  }

  
  var fakeFeedbackApiClient = {
    send: function(fields, cb) {
      alert("Sent feedback data: " + JSON.stringify(fields));
      cb();
    }
  };

  var Example = React.createClass({displayName: 'Example',
    render: function() {
      var cx = React.addons.classSet;
      return (
        React.DOM.div( {className:"example"}, 
          React.DOM.h3(null, this.props.summary),
          React.DOM.div( {className:cx({comp: true, dashed: this.props.dashed}),
               style:this.props.style || {}}, 
            this.props.children
          )
        )
      );
    }
  });

  var Section = React.createClass({displayName: 'Section',
    render: function() {
      return (
        React.DOM.section( {id:this.props.name}, 
          React.DOM.h1(null, this.props.name),
          this.props.children
        )
      );
    }
  });

  var ShowCase = React.createClass({displayName: 'ShowCase',
    render: function() {
      return (
        React.DOM.div( {className:"showcase"}, 
          React.DOM.header(null, 
            React.DOM.h1(null, "Loop UI Components Showcase"),
            React.DOM.nav( {className:"menu"}, 
              React.Children.map(this.props.children, function(section) {
                return (
                  React.DOM.a( {className:"btn btn-info", href:"#" + section.props.name}, 
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
          Section( {name:"PanelView"}, 
            Example( {summary:"332px wide", dashed:"true", style:{width: "332px"}}, 
              PanelView(null )
            )
          ),

          Section( {name:"IncomingCallView"}, 
            Example( {summary:"Default", dashed:"true", style:{width: "280px"}}, 
              IncomingCallView(null )
            )
          ),

          Section( {name:"ConversationToolbar"}, 
            Example( {summary:"Default"}, 
              ConversationToolbar( {video:{enabled: true}, audio:{enabled: true}} )
            ),
            Example( {summary:"Video muted"}, 
              ConversationToolbar( {video:{enabled: false}, audio:{enabled: true}} )
            ),
            Example( {summary:"Audio muted"}, 
              ConversationToolbar( {video:{enabled: true}, audio:{enabled: false}} )
            )
          ),

          Section( {name:"ConversationView"}, 
            Example( {summary:"Default"}, 
              ConversationView( {video:{enabled: true}, audio:{enabled: true}} )
            )
          ),

          Section( {name:"FeedbackView"}, 
            Example( {summary:"Default (useable demo)", dashed:"true", style:{width: "280px"}}, 
              FeedbackView( {feedbackApiClient:fakeFeedbackApiClient} )
            ),
            Example( {summary:"Detailed form", dashed:"true", style:{width: "280px"}}, 
              FeedbackView( {step:"form"} )
            ),
            Example( {summary:"Thank you!", dashed:"true", style:{width: "280px"}}, 
              FeedbackView( {step:"finished"} )
            )
          ),

          Section( {name:"CallUrlExpiredView"}, 
            Example( {summary:"Firefox User"}, 
              CallUrlExpiredView( {helper:{isFirefox: returnTrue}} )
            ),
            Example( {summary:"Non-Firefox User"}, 
              CallUrlExpiredView( {helper:{isFirefox: returnFalse}} )
            )
          )
        )
      );
    }
  });

  window.addEventListener("DOMContentLoaded", function() {
    React.renderComponent(App(null ), document.body);
  });
})();
