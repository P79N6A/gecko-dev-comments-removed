



XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
                                  "resource:///modules/CustomizableUI.jsm");




const PanelUI = {
  
  get kEvents() ["popupshowing", "popupshown", "popuphiding", "popuphidden"],
  



  get kElements() {
    return {
      contents: "PanelUI-contents",
      mainView: "PanelUI-mainView",
      multiView: "PanelUI-multiView",
      menuButton: "PanelUI-menu-button",
      panel: "PanelUI-popup",
      zoomResetButton: "PanelUI-zoomReset-btn"
    };
  },

  init: function() {
    for (let [k, v] of Iterator(this.kElements)) {
      
      let getKey = k;
      let id = v;
      this.__defineGetter__(getKey, function() {
        delete this[getKey];
        return this[getKey] = document.getElementById(id);
      });
    }

    for (let event of this.kEvents) {
      this.panel.addEventListener(event, this);
    }

    
    Services.obs.addObserver(this, "browser-fullZoom:zoomChange", false);
    Services.obs.addObserver(this, "browser-fullZoom:zoomReset", false);

    this._updateZoomResetButton();
  },

  uninit: function() {
    for (let event of this.kEvents) {
      this.panel.removeEventListener(event, this);
    }
    Services.obs.removeObserver(this, "browser-fullZoom:zoomChange");
    Services.obs.removeObserver(this, "browser-fullZoom:zoomReset");
  },

  







  replaceMainView: function(aMainView) {
    this.multiView.insertBefore(aMainView, this.multiView.firstChild);
  },

  







  toggle: function(aEvent) {
    if (this.panel.state == "open") {
      this.hide();
    } else if (this.panel.state == "closed") {
      this.ensureRegistered();

      let anchor = aEvent.target;
      let iconAnchor =
        document.getAnonymousElementByAttribute(anchor, "class",
                                                "toolbarbutton-icon");
      this.panel.openPopup(iconAnchor || anchor, "bottomcenter topright");
    }
  },

  


  hide: function() {
    this.panel.hidePopup();
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "popupshowing":
        
      case "popupshown":
        
      case "popuphiding":
        
      case "popuphidden": {
        this._updatePanelButton(aEvent.target);
        break;
      }
    }
  },

  


  observe: function (aSubject, aTopic, aData) {
    this._updateZoomResetButton();
  },

  





  ensureRegistered: function() {
    CustomizableUI.registerMenuPanel(this.contents);
  },

  



  showMainView: function() {
    this.multiView.showMainView();
  },

  






  showSubView: function(aViewId, aAnchor, aPlacementArea) {
    let viewNode = document.getElementById(aViewId);
    if (!viewNode) {
      Cu.reportError("Could not show panel subview with id: " + aViewId);
      return;
    }

    if (!aAnchor) {
      Cu.reportError("Expected an anchor when opening subview with id: " + aViewId);
      return;
    }

    if (aPlacementArea == CustomizableUI.AREA_PANEL) {
      this.multiView.showSubView(aViewId, aAnchor);
    } else {
      
      
      let evt = document.createEvent("CustomEvent");
      evt.initCustomEvent("ViewShowing", true, true, viewNode);
      viewNode.dispatchEvent(evt);

      let tempPanel = document.createElement("panel");
      tempPanel.appendChild(viewNode);
      tempPanel.setAttribute("type", "arrow");
      tempPanel.setAttribute("id", "customizationui-widget-panel");
      document.getElementById(CustomizableUI.AREA_NAVBAR).appendChild(tempPanel);
      tempPanel.addEventListener("popuphidden", function panelRemover() {
        tempPanel.removeEventListener("popuphidden", panelRemover);
        this.multiView.appendChild(viewNode);
        tempPanel.parentElement.removeChild(tempPanel);
      }.bind(this));

      tempPanel.openPopup(aAnchor, "bottomcenter topright");
    }
  },

  



  _updatePanelButton: function() {
    this.menuButton.open = this.panel.state == "open" ||
                           this.panel.state == "showing";
  },

  _updateZoomResetButton: function() {
    this.zoomResetButton.setAttribute("label", gNavigatorBundle
      .getFormattedString("zoomReset.label", [Math.floor(ZoomManager.zoom * 100)]));
  }
};
