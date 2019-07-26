





var loop = loop || {};
loop.webapp = (function() {
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
    el: "#conversation"
  });

  




  var Router = Backbone.Router.extend({
    _conversation: undefined,
    activeView: undefined,

    routes: {
      "": "home",
      "call/:token": "initiate"
    },

    initialize: function(options) {
      options = options || {};
      if (!options.conversation) {
        throw new Error("missing required conversation");
      }
      this._conversation = options.conversation;
      this.listenTo(this._conversation, "session:ready",
                    this._onConversationSessionReady);

      
      this.loadView(new HomeView());
    },

    




    _onConversationSessionReady: function(conversation) {
      
      
      
      alert("conversation session ready");
      console.log("conversation session info", conversation);
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
    HomeView: HomeView,
    init: init,
    Router: Router
  };
})();
