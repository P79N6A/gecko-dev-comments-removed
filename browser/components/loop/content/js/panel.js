





Components.utils.import("resource://gre/modules/Services.jsm");

var loop = loop || {};
loop.panel = (function(TB, mozl10n) {
  "use strict";

  var baseServerUrl = Services.prefs.getCharPref("loop.server"),
      panelView,
      
      __ = mozl10n.get;

  




  var PanelView = Backbone.View.extend({
    el: "#default-view",

    events: {
      "keyup input[name=caller]": "changeButtonState",
      "click a.get-url": "getCallUrl",
      "click a.go-back": "goBack"
    },

    initialize: function() {
      this.client = new loop.shared.Client({
        baseServerUrl: baseServerUrl
      });
      this.notifier = new loop.shared.views.NotificationListView({
        el: this.$(".messages")
      }).render();
    },

    getCallUrl: function(event) {
      event.preventDefault();
      var nickname = this.$("input[name=caller]").val();
      var callback = function(err, callUrl) {
        if (err) {
          this.notifier.notify({
            message: __("unable_retrieve_url"),
            level: "error"
          });
          return;
        }
        this.onCallUrlReceived(callUrl);
      }.bind(this);

      this.client.requestCallUrl(nickname, callback);
    },

    goBack: function(event) {
      this.$(".action .result").hide();
      this.$(".action .invite").show();
      this.$(".description p").text(__("get_link_to_share"));
    },

    onCallUrlReceived: function(callUrl) {
      this.notifier.clear();
      this.$(".action .invite").hide();
      this.$(".action .invite input").val("");
      this.$(".action .result input").val(callUrl);
      this.$(".action .result").show();
      this.$(".description p").text(__("share_link_url"));
    },

    changeButtonState: function() {
      var enabled = !!this.$("input[name=caller]").val();
      if (enabled)
        this.$("a.get-url").removeClass("disabled");
      else
        this.$("a.get-url").addClass("disabled");
    }
  });

  


  function init() {
    panelView = new PanelView();
    panelView.render();
  }

  return {
    init: init,
    PanelView: PanelView
  };
})(_, document.mozL10n);
