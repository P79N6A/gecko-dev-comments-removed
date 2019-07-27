



var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.views = loop.shared.views || {};
loop.shared.views.TextChatView = (function(mozl10n) {
  var sharedActions = loop.shared.actions;
  var CHAT_MESSAGE_TYPES = loop.store.CHAT_MESSAGE_TYPES;
  var CHAT_CONTENT_TYPES = loop.store.CHAT_CONTENT_TYPES;

  


  var TextChatEntry = React.createClass({displayName: "TextChatEntry",
    mixins: [React.addons.PureRenderMixin],

    propTypes: {
      message: React.PropTypes.string.isRequired,
      type: React.PropTypes.string.isRequired
    },

    render: function() {
      var classes = React.addons.classSet({
        "text-chat-entry": true,
        "received": this.props.type === CHAT_MESSAGE_TYPES.RECEIVED
      });

      return (
        React.createElement("div", {className: classes}, 
          React.createElement("span", null, this.props.message)
        )
      );
    }
  });

  




  var TextChatEntriesView = React.createClass({displayName: "TextChatEntriesView",
    mixins: [React.addons.PureRenderMixin],

    propTypes: {
      messageList: React.PropTypes.array.isRequired
    },

    componentWillUpdate: function() {
      var node = this.getDOMNode();
      if (!node) {
        return;
      }
      
      this.shouldScroll = node.scrollHeight === node.scrollTop + node.clientHeight;
    },

    componentDidUpdate: function() {
      if (this.shouldScroll) {
        
        window.requestAnimationFrame(function() {
          var node = this.getDOMNode();
          node.scrollTop = node.scrollHeight - node.clientHeight;
        }.bind(this));
      }
    },

    render: function() {
      if (!this.props.messageList.length) {
        return null;
      }

      return (
        React.createElement("div", {className: "text-chat-entries"}, 
          React.createElement("div", {className: "text-chat-scroller"}, 
            
              this.props.messageList.map(function(entry, i) {
                return (
                  React.createElement(TextChatEntry, {key: i, 
                                 message: entry.message, 
                                 type: entry.type})
                );
              }, this)
            
          )
        )
      );
    }
  });

  



  var TextChatView = React.createClass({displayName: "TextChatView",
    mixins: [
      React.addons.LinkedStateMixin,
      loop.store.StoreMixin("textChatStore")
    ],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired
    },

    getInitialState: function() {
      return _.extend({
        messageDetail: ""
      }, this.getStoreState());
    },

    





    handleKeyDown: function(event) {
      if (event.which === 13) {
        this.handleFormSubmit(event);
      }
    },

    




    handleFormSubmit: function(event) {
      event.preventDefault();

      this.props.dispatcher.dispatch(new sharedActions.SendTextChatMessage({
        contentType: CHAT_CONTENT_TYPES.TEXT,
        message: this.state.messageDetail
      }));

      
      this.setState({ messageDetail: "" });
    },

    render: function() {
      if (!this.state.textChatEnabled) {
        return null;
      }

      var messageList = this.state.messageList;

      return (
        React.createElement("div", {className: "text-chat-view"}, 
          React.createElement(TextChatEntriesView, {messageList: messageList}), 
          React.createElement("div", {className: "text-chat-box"}, 
            React.createElement("form", {onSubmit: this.handleFormSubmit}, 
              React.createElement("input", {type: "text", 
                     placeholder: messageList.length ? "" : mozl10n.get("chat_textbox_placeholder"), 
                     onKeyDown: this.handleKeyDown, 
                     valueLink: this.linkState("messageDetail")})
            )
          )
        )
      );
    }
  });

  return TextChatView;
})(navigator.mozL10n || document.mozL10n);
