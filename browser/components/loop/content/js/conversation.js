








var loop = loop || {};
loop.conversation = (function(mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views;
  var sharedMixins = loop.shared.mixins;
  var sharedModels = loop.shared.models;
  var sharedActions = loop.shared.actions;

  var IncomingConversationView = loop.conversationViews.IncomingConversationView;
  var OutgoingConversationView = loop.conversationViews.OutgoingConversationView;
  var CallIdentifierView = loop.conversationViews.CallIdentifierView;
  var DesktopRoomConversationView = loop.roomViews.DesktopRoomConversationView;
  var GenericFailureView = loop.conversationViews.GenericFailureView;

  



  var AppControllerView = React.createClass({displayName: "AppControllerView",
    mixins: [Backbone.Events, sharedMixins.WindowCloseMixin],

    propTypes: {
      
      client: React.PropTypes.instanceOf(loop.Client).isRequired,
      conversation: React.PropTypes.instanceOf(sharedModels.ConversationModel)
                         .isRequired,
      sdk: React.PropTypes.object.isRequired,

      
      conversationAppStore: React.PropTypes.instanceOf(
        loop.store.ConversationAppStore).isRequired,
      conversationStore: React.PropTypes.instanceOf(loop.store.ConversationStore)
                              .isRequired,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      roomStore: React.PropTypes.instanceOf(loop.store.RoomStore),
      mozLoop: React.PropTypes.object.isRequired,
    },

    getInitialState: function() {
      return this.props.conversationAppStore.getStoreState();
    },

    componentWillMount: function() {
      this.listenTo(this.props.conversationAppStore, "change", function() {
        this.setState(this.props.conversationAppStore.getStoreState());
      }, this);
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.conversationAppStore);
    },

    render: function() {
      switch(this.state.windowType) {
        case "incoming": {
          return (React.createElement(IncomingConversationView, {
            client: this.props.client, 
            conversation: this.props.conversation, 
            sdk: this.props.sdk, 
            conversationAppStore: this.props.conversationAppStore}
          ));
        }
        case "outgoing": {
          return (React.createElement(OutgoingConversationView, {
            store: this.props.conversationStore, 
            dispatcher: this.props.dispatcher}
          ));
        }
        case "room": {
          return (React.createElement(DesktopRoomConversationView, {
            dispatcher: this.props.dispatcher, 
            mozLoop: this.props.mozLoop, 
            roomStore: this.props.roomStore}
          ));
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
      dispatcher: dispatcher,
      sdk: OT
    });
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
      mozLoop: navigator.mozLoop,
      sdkDriver: sdkDriver
    });
    var activeRoomStore = new loop.store.ActiveRoomStore(dispatcher, {
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

    loop.store.StoreMixin.register({feedbackStore: feedbackStore});

    
    
    var conversation = new sharedModels.ConversationModel({}, {
      sdk: window.OT,
      mozLoop: navigator.mozLoop
    });

    
    var locationHash = loop.shared.utils.locationData().hash;
    var windowId;

    var hash = locationHash.match(/#(.*)/);
    if (hash) {
      windowId = hash[1];
    }

    conversation.set({windowId: windowId});

    window.addEventListener("unload", function(event) {
      
      
      
      navigator.mozLoop.calls.clearCallInProgress(windowId);

      dispatcher.dispatch(new sharedActions.WindowUnload());
    });

    React.render(React.createElement(AppControllerView, {
      conversationAppStore: conversationAppStore, 
      roomStore: roomStore, 
      conversationStore: conversationStore, 
      client: client, 
      conversation: conversation, 
      dispatcher: dispatcher, 
      sdk: window.OT, 
      mozLoop: navigator.mozLoop}
    ), document.querySelector('#main'));

    dispatcher.dispatch(new sharedActions.GetWindowData({
      windowId: windowId
    }));
  }

  return {
    AppControllerView: AppControllerView,
    init: init
  };
})(document.mozL10n);

document.addEventListener('DOMContentLoaded', loop.conversation.init);
