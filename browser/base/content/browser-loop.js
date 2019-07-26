




let LoopUI;

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI", "resource:///modules/loop/MozLoopAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService", "resource:///modules/loop/MozLoopService.jsm");


(function() {

  LoopUI = {
    





    openCallPanel: function(event) {
      let panel = document.getElementById("loop-panel");
      let anchor = event.target;
      let iframe = document.getElementById("loop-panel-frame");

      if (!iframe) {
        
        iframe = document.createElement("iframe");
        iframe.setAttribute("id", "loop-panel-frame");
        iframe.setAttribute("type", "content");
        iframe.setAttribute("class", "loop-frame social-panel-frame");
        iframe.setAttribute("flex", "1");
        panel.appendChild(iframe);
      }

      
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
