



let ProjectPanel = {
  
  toggleSidebar: function() {
    document.querySelector("#project-listing-panel").setAttribute("sidebar-displayed", true);
    document.querySelector("#project-listing-splitter").setAttribute("sidebar-displayed", true);
  },

  showPopup: function() {
    let deferred = promise.defer();

    let panelNode = document.querySelector("#project-panel");
    let panelVboxNode = document.querySelector("#project-panel > #project-panel-box");
    let anchorNode = document.querySelector("#project-panel-button > .panel-button-anchor");

    window.setTimeout(() => {
      
      
      
      function onPopupShown() {
        panelNode.removeEventListener("popupshown", onPopupShown);
        deferred.resolve();
      }

      panelNode.addEventListener("popupshown", onPopupShown);
      panelNode.openPopup(anchorNode);
      panelVboxNode.scrollTop = 0;
    }, 0);

    return deferred.promise;
  }
};
