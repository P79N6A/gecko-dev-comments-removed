








var loop = loop || {};
loop.conversation = (function(mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views;
  var sharedMixins = loop.shared.mixins;
  var sharedModels = loop.shared.models;
  var sharedActions = loop.shared.actions;

  var CallControllerView = loop.conversationViews.CallControllerView;
  var CallIdentifierView = loop.conversationViews.CallIdentifierView;
  var DesktopRoomConversationView = loop.roomViews.DesktopRoomConversationView;
  var GenericFailureView = loop.conversationViews.GenericFailureView;

  



  var AppControllerView = React.createClass({displayName: "AppControllerView",
    mixins: [
      Backbone.Events,
      loop.store.StoreMixin("conversationAppStore"),
      sharedMixins.WindowCloseMixin
    ],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      roomStore: React.PropTypes.instanceOf(loop.store.RoomStore),
      mozLoop: React.PropTypes.object.isRequired
    },

    getInitialState: function() {
      return this.getStoreState();
    },

    render: function() {
      switch(this.state.windowType) {
        
        case "incoming":
        case "outgoing": {
          return (React.createElement(CallControllerView, {
            dispatcher: this.props.dispatcher, 
            mozLoop: this.props.mozLoop}));
        }
        case "room": {
          return (React.createElement(DesktopRoomConversationView, {
            dispatcher: this.props.dispatcher, 
            mozLoop: this.props.mozLoop, 
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

    var dispatcher = new loop.Dispatcher();
    var client = new loop.Client();
    var sdkDriver = new loop.OTSdkDriver({
      isDesktop: true,
      dispatcher: dispatcher,
      sdk: OT,
      mozLoop: navigator.mozLoop
    });

    
    loop.conversation._sdkDriver = sdkDriver;

    var appVersionInfo = navigator.mozLoop.appVersionInfo;
    var feedbackClient = new loop.FeedbackAPIClient(
      navigator.mozLoop.getLoopPref("feedback.baseUrl"), {
      product: navigator.mozLoop.getLoopPref("feedback.product"),
      platform: appVersionInfo.OS,
      channel: appVersionInfo.channel,
      version: appVersionInfo.version
    });

    
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
    var feedbackStore = new loop.store.FeedbackStore(dispatcher, {
      feedbackClient: feedbackClient
    });

    loop.store.StoreMixin.register({
      conversationAppStore: conversationAppStore,
      conversationStore: conversationStore,
      feedbackStore: feedbackStore,
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

    React.render(React.createElement(AppControllerView, {
      roomStore: roomStore, 
      dispatcher: dispatcher, 
      mozLoop: navigator.mozLoop}
    ), document.querySelector('#main'));

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

document.addEventListener('DOMContentLoaded', loop.conversation.init);
