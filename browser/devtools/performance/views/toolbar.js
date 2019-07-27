


"use strict";




let ToolbarView = {
  


  initialize: Task.async(function *() {
    this._onFilterPopupShowing = this._onFilterPopupShowing.bind(this);
    this._onFilterPopupHiding = this._onFilterPopupHiding.bind(this);
    this._onHiddenMarkersChanged = this._onHiddenMarkersChanged.bind(this);
    this._onPrefChanged = this._onPrefChanged.bind(this);
    this._popup = $("#performance-options-menupopup");

    this.optionsView = new OptionsView({
      branchName: BRANCH_NAME,
      menupopup: this._popup
    });

    
    
    let experimentalEnabled = PerformanceController.getOption("experimental");
    this._toggleExperimentalUI(experimentalEnabled);

    yield this.optionsView.initialize();
    this.optionsView.on("pref-changed", this._onPrefChanged);

    this._buildMarkersFilterPopup();
    this._updateHiddenMarkersPopup();
    $("#performance-filter-menupopup").addEventListener("popupshowing", this._onFilterPopupShowing);
    $("#performance-filter-menupopup").addEventListener("popuphiding",  this._onFilterPopupHiding);
  }),

  


  destroy: function () {
    $("#performance-filter-menupopup").removeEventListener("popupshowing", this._onFilterPopupShowing);
    $("#performance-filter-menupopup").removeEventListener("popuphiding",  this._onFilterPopupHiding);
    this._popup = null

    this.optionsView.off("pref-changed", this._onPrefChanged);
    this.optionsView.destroy();
  },

  


  _buildMarkersFilterPopup: function() {
    for (let [markerName, markerDetails] of Iterator(TIMELINE_BLUEPRINT)) {
      let menuitem = document.createElement("menuitem");
      menuitem.setAttribute("closemenu", "none");
      menuitem.setAttribute("type", "checkbox");
      menuitem.setAttribute("align", "center");
      menuitem.setAttribute("flex", "1");
      menuitem.setAttribute("label", MarkerUtils.getMarkerClassName(markerName));
      menuitem.setAttribute("marker-type", markerName);
      menuitem.className = `marker-color-${markerDetails.colorName}`;

      menuitem.addEventListener("command", this._onHiddenMarkersChanged);

      $("#performance-filter-menupopup").appendChild(menuitem);
    }
  },

  


  _updateHiddenMarkersPopup: function() {
    let menuItems = $$("#performance-filter-menupopup menuitem[marker-type]");
    let hiddenMarkers = PerformanceController.getPref("hidden-markers");

    for (let menuitem of menuItems) {
      if (~hiddenMarkers.indexOf(menuitem.getAttribute("marker-type"))) {
        menuitem.removeAttribute("checked");
      } else {
        menuitem.setAttribute("checked", "true");
      }
    }
  },

  












  _toggleExperimentalUI: function (isEnabled) {
    if (isEnabled) {
      $(".theme-body").classList.add("experimental-enabled");
      this._popup.classList.add("experimental-enabled");
    } else {
      $(".theme-body").classList.remove("experimental-enabled");
      this._popup.classList.remove("experimental-enabled");
    }
  },

  


  _onFilterPopupShowing: function() {
    $("#filter-button").setAttribute("open", "true");
  },

  


  _onFilterPopupHiding: function() {
    $("#filter-button").removeAttribute("open");
  },

  


  _onHiddenMarkersChanged: function() {
    let checkedMenuItems = $$("#performance-filter-menupopup menuitem[marker-type]:not([checked])");
    let hiddenMarkers = Array.map(checkedMenuItems, e => e.getAttribute("marker-type"));
    PerformanceController.setPref("hidden-markers", hiddenMarkers);
  },

  



  _onPrefChanged: function (_, prefName) {
    let value = PerformanceController.getOption(prefName);

    if (prefName === "experimental") {
      this._toggleExperimentalUI(value);
    }

    this.emit(EVENTS.PREF_CHANGED, prefName, value);
  },

  toString: () => "[object ToolbarView]"
};

EventEmitter.decorate(ToolbarView);
