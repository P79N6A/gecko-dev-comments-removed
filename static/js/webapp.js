





var loop = loop || {};
loop.webapp = (function($, TB) {
  "use strict";

  






  var baseApiUrl = "http://localhost:5000";

  



  var router;

  



  var conversation;

  


  var ConversationModel = Backbone.Model.extend({
    defaults: {
      loopToken:    undefined, 
      sessionId:    undefined, 
      sessionToken: undefined, 
      apiKey:       undefined  
    },

    













    initiate: function(options) {
      options = options || {};

      if (!this.get("loopToken")) {
        throw new Error("missing required attribute loopToken");
      }

      
      if (this.isSessionReady()) {
        return this.trigger("session:ready", this);
      }

      var request = $.ajax({
        url:         baseApiUrl + "/calls/" + this.get("loopToken"),
        method:      "POST",
        contentType: "application/json",
        data:        JSON.stringify({}),
        dataType:    "json"
      });

      request.done(this.setReady.bind(this));

      request.fail(function(xhr, _, statusText) {
        var serverError = xhr.status + " " + statusText;
        if (typeof xhr.responseJSON === "object" && xhr.responseJSON.error) {
          serverError += "; " + xhr.responseJSON.error;
        }
        this.trigger("session:error", new Error(
          "Retrieval of session information failed: HTTP " + serverError));
      }.bind(this));
    },

    




    isSessionReady: function() {
      return !!this.get("sessionId");
    },

    




    setReady: function(sessionData) {
      
      this.set({
        sessionId:    sessionData.sessionId,
        sessionToken: sessionData.sessionToken,
        apiKey:       sessionData.apiKey
      }).trigger("session:ready", this);
      return this;
    }
  });

  


  var BaseView = Backbone.View.extend({
    




    hide: function() {
      this.$el.hide();
      return this;
    },

    




    show: function() {
      this.$el.show();
      return this;
    }
  });

  


  var HomeView = BaseView.extend({
    el: "#home"
  });

  



  var ConversationFormView = BaseView.extend({
    el: "#conversation-form",

    events: {
      "submit": "initiate"
    },

    initialize: function() {
      this.listenTo(this.model, "session:error", function(error) {
        
        
        alert(error);
      });
    },

    initiate: function(event) {
      event.preventDefault();
      this.model.initiate();
    }
  });

  


  var ConversationView = BaseView.extend({
    el: "#conversation",

    


    initialize: function() {
      this.videoStyles = { width: "100%", height: "100%" };
      
      
      
      this.session   = TB.initSession(this.model.get("sessionId"));
      this.publisher = TB.initPublisher(this.model.get("apiKey"), "outgoing",
                                        this.videoStyles);

      this.session.connect(this.model.get("apiKey"),
                           this.model.get("sessionToken"));

      this.listenTo(this.session, "sessionConnected", this._sessionConnected);
      this.listenTo(this.session, "streamCreated", this._streamCreated);
      this.listenTo(this.session, "connectionDestroyed", this._sessionEnded);
    },

    _sessionConnected: function(event) {
      this.session.publish(this.publisher);
      this._subscribeToStreams(event.streams);
    },

    _streamCreated: function(event) {
      this._subscribeToStreams(event.streams);
    },

    _sessionEnded: function(event) {
      
      alert("Your session has ended. Reason: " + event.reason);
      this.model.trigger("session:ended");
    },

    _subscribeToStreams: function(streams) {
      streams.forEach(function(stream) {
        if (stream.connection.connectionId !==
            this.session.connection.connectionId) {
          this.session.subscribe(stream, "incoming", this.videoStyles);
        }
      }.bind(this));
    }
  });

  




  var Router = Backbone.Router.extend({
    _conversation: undefined,
    activeView: undefined,

    routes: {
      "": "home",
      "call/ongoing": "conversation",
      "call/:token": "initiate"
    },

    initialize: function(options) {
      options = options || {};
      if (!options.conversation) {
        throw new Error("missing required conversation");
      }
      this._conversation = options.conversation;

      this.listenTo(this._conversation, "session:ready", this._onSessionReady);
      this.listenTo(this._conversation, "session:ended", this._onSessionEnded);

      
      this.loadView(new HomeView());
    },

    


    _onSessionReady: function() {
      this.navigate("call/ongoing", {trigger: true});
    },

    


    _onSessionEnded: function() {
      this.navigate("call/" + this._conversation.get("token"), {trigger: true});
    },

    




    loadView : function(view) {
      if (this.activeView) {
        this.activeView.hide();
      }
      this.activeView = view.render().show();
    },

    


    home: function() {
      this.loadView(new HomeView());
    },

    





    initiate: function(loopToken) {
      this._conversation.set("loopToken", loopToken);
      this.loadView(new ConversationFormView({model: this._conversation}));
    },

    



    conversation: function() {
      if (!this._conversation.isSessionReady()) {
        var loopToken = this._conversation.get("loopToken");
        if (loopToken) {
          return this.navigate("call/" + loopToken, {trigger: true});
        } else {
          
          return this.navigate("home", {trigger: true});
        }
      }
      this.loadView(new ConversationView({model: this._conversation}));
    }
  });

  


  function init() {
    conversation = new ConversationModel();
    router = new Router({conversation: conversation});
    Backbone.history.start();
  }

  return {
    BaseView: BaseView,
    ConversationFormView: ConversationFormView,
    ConversationModel: ConversationModel,
    ConversationView: ConversationView,
    HomeView: HomeView,
    init: init,
    Router: Router
  };
})(jQuery, window.TB);
