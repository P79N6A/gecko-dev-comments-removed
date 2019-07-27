



let RuntimePanel = {
  
  toggleSidebar: function() {
    document.querySelector("#runtime-listing-panel").setAttribute("sidebar-displayed", true);
    document.querySelector("#runtime-listing-splitter").setAttribute("sidebar-displayed", true);
  },

  showPopup: function() {
    let deferred = promise.defer();
    let panel = document.querySelector("#runtime-panel");
    let anchor = document.querySelector("#runtime-panel-button > .panel-button-anchor");

    function onPopupShown() {
      panel.removeEventListener("popupshown", onPopupShown);
      deferred.resolve();
    }

    panel.addEventListener("popupshown", onPopupShown);
    panel.openPopup(anchor);
    return deferred.promise;
  }
};
