





var loop = loop || {};
loop.panel = (function(_, mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views,
      
      __ = mozL10n.get;

  



  var router;

  


  var DoNotDisturbView = sharedViews.BaseView.extend({
    template: _.template([
      '<label>',
      '  <input type="checkbox" <%- checked %>>',
      '  <span data-l10n-id="do_not_disturb"></span>',
      '</label>',
    ].join('')),

    events: {
      "click input[type=checkbox]": "toggle"
    },

    


    toggle: function() {
      navigator.mozLoop.doNotDisturb = !navigator.mozLoop.doNotDisturb;
      this.render();
    },

    render: function() {
      this.$el.html(this.template({
        checked: navigator.mozLoop.doNotDisturb ? "checked" : ""
      }));
      return this;
    }
  });

  


  var PanelView = sharedViews.BaseView.extend({
    template: _.template([
      '<div class="description">',
      '  <p data-l10n-id="get_link_to_share"></p>',
      '</div>',
      '<div class="action">',
      '  <form class="invite">',
      '    <input type="text" name="caller" data-l10n-id="caller" required>',
      '    <button type="submit" class="get-url btn btn-success"',
      '       data-l10n-id="get_a_call_url"></button>',
      '  </form>',
      '  <p class="result hide">',
      '    <input id="call-url" type="url" readonly>',
      '    <a class="go-back btn btn-info" href="" data-l10n-id="new_url"></a>',
      '  </p>',
      '  <p class="dnd"></p>',
      '</div>',
    ].join("")),

    className: "share generate-url",

    



    dndView: undefined,

    events: {
      "keyup input[name=caller]": "changeButtonState",
      "submit form.invite": "getCallUrl",
      "click a.go-back": "goBack"
    },

    initialize: function(options) {
      options = options || {};
      if (!options.notifier) {
        throw new Error("missing required notifier");
      }
      this.notifier = options.notifier;
      this.client = new loop.Client();
    },

    getNickname: function() {
      return this.$("input[name=caller]").val();
    },

    getCallUrl: function(event) {
      this.notifier.clear();
      event.preventDefault();
      var callback = function(err, callUrlData) {
        this.clearPending();
        if (err) {
          this.notifier.errorL10n("unable_retrieve_url");
          this.render();
          return;
        }
        this.onCallUrlReceived(callUrlData);
      }.bind(this);

      this.setPending();
      this.client.requestCallUrl(this.getNickname(), callback);
    },

    goBack: function(event) {
      event.preventDefault();
      this.$(".action .result").hide();
      this.$(".action .invite").show();
      this.$(".description p").text(__("get_link_to_share"));
      this.changeButtonState();
    },

    onCallUrlReceived: function(callUrlData) {
      this.notifier.clear();
      this.$(".action .invite").hide();
      this.$(".action .invite input").val("");
      this.$(".action .result input").val(callUrlData.call_url);
      this.$(".action .result").show();
      this.$(".description p").text(__("share_link_url"));
    },

    setPending: function() {
      this.$("[name=caller]").addClass("pending");
      this.$(".get-url").addClass("disabled").attr("disabled", "disabled");
    },

    clearPending: function() {
      this.$("[name=caller]").removeClass("pending");
      this.changeButtonState();
    },

    changeButtonState: function() {
      var enabled = !!this.$("input[name=caller]").val();
      if (enabled) {
        this.$(".get-url").removeClass("disabled")
            .removeAttr("disabled", "disabled");
      } else {
        this.$(".get-url").addClass("disabled").attr("disabled", "disabled");
      }
    },

    render: function() {
      this.$el.html(this.template());
      
      this.dndView = new DoNotDisturbView({el: this.$(".dnd")}).render();
      return this;
    }
  });

  var PanelRouter = loop.desktopRouter.DesktopRouter.extend({
    



    document: undefined,

    routes: {
      "": "home"
    },

    initialize: function(options) {
      options = options || {};
      if (!options.document) {
        throw new Error("missing required document");
      }
      this.document = options.document;

      this._registerVisibilityChangeEvent();

      this.on("panel:open panel:closed", this.reset, this);
    },

    








    _registerVisibilityChangeEvent: function() {
      this.document.addEventListener("visibilitychange", function(event) {
        this.trigger(event.currentTarget.hidden ? "panel:closed"
                                                : "panel:open");
      }.bind(this));
    },

    


    home: function() {
      this.reset();
    },

    


    reset: function() {
      
      this._notifier.clear();
      
      this.loadView(new PanelView({notifier: this._notifier}));
    }
  });

  


  function init() {
    
    
    mozL10n.initialize(navigator.mozLoop);

    router = new PanelRouter({
      document: document,
      notifier: new sharedViews.NotificationListView({el: "#messages"})
    });
    Backbone.history.start();

    
    var evtObject = document.createEvent('Event');
    evtObject.initEvent('loopPanelInitialized', true, false);
    window.dispatchEvent(evtObject);
  }

  return {
    init: init,
    PanelView: PanelView,
    DoNotDisturbView: DoNotDisturbView,
    PanelRouter: PanelRouter
  };
})(_, document.mozL10n);
