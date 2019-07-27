




let LoopUI;

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI", "resource:///modules/loop/MozLoopAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService", "resource:///modules/loop/MozLoopService.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PanelFrame", "resource:///modules/PanelFrame.jsm");


(function() {

  LoopUI = {
    get toolbarButton() {
      delete this.toolbarButton;
      return this.toolbarButton = CustomizableUI.getWidget("loop-call-button").forWindow(window);
    },

    





    openCallPanel: function(event) {
      let callback = iframe => {
        iframe.addEventListener("DOMContentLoaded", function documentDOMLoaded() {
          iframe.removeEventListener("DOMContentLoaded", documentDOMLoaded, true);
          injectLoopAPI(iframe.contentWindow);
        }, true);
      };

      PanelFrame.showPopup(window, event.target, "loop", null,
                           "about:looppanel", null, callback);
    },

    



    init: function() {
      if (!Services.prefs.getBoolPref("loop.enabled")) {
        this.toolbarButton.node.hidden = true;
        return;
      }

      
      Services.obs.addObserver(this, "loop-status-changed", false);


      MozLoopService.initialize();
      this.updateToolbarState();
    },

    uninit: function() {
      Services.obs.removeObserver(this, "loop-status-changed");
    },

    
    observe: function(subject, topic, data) {
      if (topic != "loop-status-changed") {
        return;
      }
      this.updateToolbarState();
    },

    updateToolbarState: function() {
      let state = "";
      if (MozLoopService.errors.size) {
        state = "error";
      } else if (MozLoopService.doNotDisturb) {
        state = "disabled";
      }
      this.toolbarButton.node.setAttribute("state", state);
    },
  };
})();
