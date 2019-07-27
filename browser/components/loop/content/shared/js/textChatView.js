



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
      contentType: React.PropTypes.string.isRequired,
      message: React.PropTypes.string.isRequired,
      type: React.PropTypes.string.isRequired
    },

    render: function() {
      var classes = React.addons.classSet({
        "text-chat-entry": true,
        "received": this.props.type === CHAT_MESSAGE_TYPES.RECEIVED,
        "special": this.props.type === CHAT_MESSAGE_TYPES.SPECIAL,
        "room-name": this.props.contentType === CHAT_CONTENT_TYPES.ROOM_NAME
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
                                 contentType: entry.contentType, 
                                 message: entry.message, 
                                 type: entry.type})
                );
              }, this)
            
          )
        )
      );
    }
  });

  







  var TextChatInputView = React.createClass({displayName: "TextChatInputView",
    mixins: [
      React.addons.LinkedStateMixin,
      React.addons.PureRenderMixin
    ],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      showPlaceholder: React.PropTypes.bool.isRequired,
      textChatEnabled: React.PropTypes.bool.isRequired
    },

    getInitialState: function() {
      return {
        messageDetail: ""
      };
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
      if (!this.props.textChatEnabled) {
        return null;
      }

      return (
        React.createElement("div", {className: "text-chat-box"}, 
          React.createElement("form", {onSubmit: this.handleFormSubmit}, 
            React.createElement("input", {type: "text", 
                   placeholder: this.props.showPlaceholder ? mozl10n.get("chat_textbox_placeholder") : "", 
                   onKeyDown: this.handleKeyDown, 
                   valueLink: this.linkState("messageDetail")})
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
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      showAlways: React.PropTypes.bool.isRequired,
      showRoomName: React.PropTypes.bool.isRequired
    },

    getInitialState: function() {
      return this.getStoreState();
    },

    render: function() {
      var messageList;
      var hasNonSpecialMessages;

      if (this.props.showRoomName) {
        messageList = this.state.messageList;
        hasNonSpecialMessages = messageList.some(function(item) {
          return item.type !== CHAT_MESSAGE_TYPES.SPECIAL;
        });
      } else {
        
        messageList = this.state.messageList.filter(function(item) {
          return item.type !== CHAT_MESSAGE_TYPES.SPECIAL;
        });
        hasNonSpecialMessages = !!messageList.length;
      }

      if (!this.props.showAlways && !this.state.textChatEnabled && !messageList.length) {
        return null;
      }

      return (
        React.createElement("div", {className: "text-chat-view"}, 
          React.createElement(TextChatEntriesView, {messageList: messageList}), 
          React.createElement(TextChatInputView, {
            dispatcher: this.props.dispatcher, 
            showPlaceholder: !hasNonSpecialMessages, 
            textChatEnabled: this.state.textChatEnabled})
        )
      );
    }
  });

  return TextChatView;
})(navigator.mozL10n || document.mozL10n);
