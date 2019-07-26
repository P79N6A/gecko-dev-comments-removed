




let LoopUI;

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI", "resource:///modules/loop/MozLoopAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService", "resource:///modules/loop/MozLoopService.jsm");


(function() {

  LoopUI = {
    





    openCallPanel: function(event) {
      let panel = document.getElementById("loop-panel");
      let anchor = event.target;
      let iframe = document.getElementById("loop-panel-frame");

      
      iframe.addEventListener("DOMContentLoaded", function documentDOMLoaded() {
        iframe.removeEventListener("DOMContentLoaded", documentDOMLoaded, true);
        injectLoopAPI(iframe.contentWindow);

        
        
        iframe.contentWindow.addEventListener("loopPanelInitialized",
          function documentLoaded() {
            iframe.contentWindow.removeEventListener("loopPanelInitialized",
                                                     documentLoaded, true);
            
            
            sizeSocialPanelToContent(panel, iframe);
          }, true);

      }, true);

      iframe.setAttribute("src", "about:looppanel");
      panel.hidden = false;
      panel.openPopup(anchor, "bottomcenter topright", 0, 0, false, false);
    },

    


    initialize: function() {
      var observer = function observer(sbject, topic, data) {
        if (topic == "browser-delayed-startup-finished") {
          Services.obs.removeObserver(observer, "browser-delayed-startup-finished");
          MozLoopService.initialize();
        }
      };
      Services.obs.addObserver(observer,
                               "browser-delayed-startup-finished", false);
    }

  };

  LoopUI.initialize();
})();
