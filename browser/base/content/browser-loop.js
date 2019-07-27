




let LoopUI;

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI", "resource:///modules/loop/MozLoopAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService", "resource:///modules/loop/MozLoopService.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PanelFrame", "resource:///modules/PanelFrame.jsm");


(function() {

  LoopUI = {
    





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

    



    initialize: function() {
      let buttonNode = CustomizableUI.getWidget("loop-call-button").forWindow(window).node;

      if (!Services.prefs.getBoolPref("loop.enabled")) {
        buttonNode.hidden = true;
        return;
      }

      if (Services.prefs.getBoolPref("loop.throttled")) {
        buttonNode.hidden = true;
        MozLoopService.checkSoftStart(buttonNode);
        return;
      }

      MozLoopService.initialize();
    },
  };
})();
