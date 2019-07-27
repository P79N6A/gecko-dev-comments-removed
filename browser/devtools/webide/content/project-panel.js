



let ProjectPanel = {
  
  toggle: function(sidebarsEnabled, triggerPopup) {
    let deferred = promise.defer();
    let doc = document;

    if (sidebarsEnabled) {
      doc.querySelector("#project-listing-panel").setAttribute("sidebar-displayed", true);
      doc.querySelector("#project-listing-splitter").setAttribute("sidebar-displayed", true);
      deferred.resolve();
    } else if (triggerPopup) {
      let panelNode = doc.querySelector("#project-panel");
      let panelVboxNode = doc.querySelector("#project-panel > #project-panel-box");
      let anchorNode = doc.querySelector("#project-panel-button > .panel-button-anchor");

      window.setTimeout(() => {
        
        
        
        function onPopupShown() {
          panelNode.removeEventListener("popupshown", onPopupShown);
          deferred.resolve();
        }

        panelNode.addEventListener("popupshown", onPopupShown);
        panelNode.openPopup(anchorNode);
        panelVboxNode.scrollTop = 0;
      }, 0);
    } else {
      deferred.resolve();
    }

    return deferred.promise;
  }
};
