




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

          
          
          iframe.contentWindow.addEventListener("loopPanelInitialized",
            function documentLoaded() {
              iframe.contentWindow.removeEventListener("loopPanelInitialized",
                                                       documentLoaded, true);
            }, true);

        }, true);
      };

      PanelFrame.showPopup(window, PanelUI, event.target, "loop", null,
                           "about:looppanel", null, callback);
    },

    



    initialize: function() {
      MozLoopService.initialize();
    },
  };
})();
