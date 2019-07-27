



var loop = loop || {};
loop.conversation = (function(mozL10n) {
  "use strict";

  var sharedMixins = loop.shared.mixins;
  var sharedActions = loop.shared.actions;

  var CallControllerView = loop.conversationViews.CallControllerView;
  var DesktopRoomConversationView = loop.roomViews.DesktopRoomConversationView;
  var FeedbackView = loop.feedbackViews.FeedbackView;
  var GenericFailureView = loop.conversationViews.GenericFailureView;

  



  var AppControllerView = React.createClass({displayName: "AppControllerView",
    mixins: [
      Backbone.Events,
      loop.store.StoreMixin("conversationAppStore"),
      sharedMixins.DocumentTitleMixin,
      sharedMixins.WindowCloseMixin
    ],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      mozLoop: React.PropTypes.object.isRequired,
      roomStore: React.PropTypes.instanceOf(loop.store.RoomStore)
    },

    getInitialState: function() {
      return this.getStoreState();
    },

    _renderFeedbackForm: function() {
      this.setTitle(mozL10n.get("conversation_has_ended"));

      return (React.createElement(FeedbackView, {
        mozLoop: this.props.mozLoop, 
        onAfterFeedbackReceived: this.closeWindow}));
    },

    



    handleCallTerminated: function() {
      var delta = new Date() - new Date(this.state.feedbackTimestamp);

      
      
      if (this.state.feedbackTimestamp === 0 ||
          delta >= this.state.feedbackPeriod) {
        this.props.dispatcher.dispatch(new sharedActions.ShowFeedbackForm());
        return;
      }

      this.closeWindow();
    },

    render: function() {
      if (this.state.showFeedbackForm) {
        return this._renderFeedbackForm();
      }

      switch(this.state.windowType) {
        
        case "incoming":
        case "outgoing": {
          return (React.createElement(CallControllerView, {
            dispatcher: this.props.dispatcher, 
            mozLoop: this.props.mozLoop, 
            onCallTerminated: this.handleCallTerminated}));
        }
        case "room": {
          return (React.createElement(DesktopRoomConversationView, {
            dispatcher: this.props.dispatcher, 
            mozLoop: this.props.mozLoop, 
            onCallTerminated: this.handleCallTerminated, 
            roomStore: this.props.roomStore}));
        }
        case "failed": {
          return React.createElement(GenericFailureView, {cancelCall: this.closeWindow});
        }
        default: {
          
          
          return null;
        }
      }
    }
  });

  


  function init() {
    
    
    mozL10n.initialize(navigator.mozLoop);

    
    
    window.OT.overrideGuidStorage({
      get: function(callback) {
        callback(null, navigator.mozLoop.getLoopPref("ot.guid"));
      },
      set: function(guid, callback) {
        
        const PREF_STRING = 32;
        navigator.mozLoop.setLoopPref("ot.guid", guid, PREF_STRING);
        callback(null);
      }
    });

    
    var useDataChannels = loop.shared.utils.getBoolPreference("textChat.enabled");

    var dispatcher = new loop.Dispatcher();
    var client = new loop.Client();
    var sdkDriver = new loop.OTSdkDriver({
      isDesktop: true,
      useDataChannels: useDataChannels,
      dispatcher: dispatcher,
      sdk: OT,
      mozLoop: navigator.mozLoop
    });

    
    loop.conversation._sdkDriver = sdkDriver;

    
    var conversationAppStore = new loop.store.ConversationAppStore({
      dispatcher: dispatcher,
      mozLoop: navigator.mozLoop
    });
    var conversationStore = new loop.store.ConversationStore(dispatcher, {
      client: client,
      isDesktop: true,
      mozLoop: navigator.mozLoop,
      sdkDriver: sdkDriver
    });
    var activeRoomStore = new loop.store.ActiveRoomStore(dispatcher, {
      isDesktop: true,
      mozLoop: navigator.mozLoop,
      sdkDriver: sdkDriver
    });
    var roomStore = new loop.store.RoomStore(dispatcher, {
      mozLoop: navigator.mozLoop,
      activeRoomStore: activeRoomStore
    });
    var textChatStore = new loop.store.TextChatStore(dispatcher, {
      sdkDriver: sdkDriver
    });

    loop.store.StoreMixin.register({
      conversationAppStore: conversationAppStore,
      conversationStore: conversationStore,
      textChatStore: textChatStore
    });

    
    var locationHash = loop.shared.utils.locationData().hash;
    var windowId;

    var hash = locationHash.match(/#(.*)/);
    if (hash) {
      windowId = hash[1];
    }

    window.addEventListener("unload", function(event) {
      dispatcher.dispatch(new sharedActions.WindowUnload());
    });

    React.render(
      React.createElement(AppControllerView, {
        dispatcher: dispatcher, 
        mozLoop: navigator.mozLoop, 
        roomStore: roomStore}), document.querySelector("#main"));

    document.documentElement.setAttribute("lang", mozL10n.getLanguage());
    document.documentElement.setAttribute("dir", mozL10n.getDirection());
    document.body.setAttribute("platform", loop.shared.utils.getPlatform());

    dispatcher.dispatch(new sharedActions.GetWindowData({
      windowId: windowId
    }));
  }

  return {
    AppControllerView: AppControllerView,
    init: init,

    





    _sdkDriver: null
  };
})(document.mozL10n);

document.addEventListener("DOMContentLoaded", loop.conversation.init);
