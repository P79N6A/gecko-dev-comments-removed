



XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
                                  "resource:///modules/CustomizableUI.jsm");




const PanelUI = {
  
  get kEvents() ["popupshowing", "popupshown", "popuphiding", "popuphidden"],
  



  get kElements() {
    return {
      contents: "PanelUI-contents",
      mainView: "PanelUI-mainView",
      multiView: "PanelUI-multiView",
      helpView: "PanelUI-help",
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

    this.helpView.addEventListener("ViewShowing", this._onHelpViewShow, false);
    this.helpView.addEventListener("ViewHiding", this._onHelpViewHide, false);
  },

  uninit: function() {
    for (let event of this.kEvents) {
      this.panel.removeEventListener(event, this);
    }
    this.helpView.removeEventListener("ViewShowing", this._onHelpViewShow);
    this.helpView.removeEventListener("ViewHiding", this._onHelpViewHide);
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

  



  showHelpView: function(aAnchor) {
    this.multiView.showSubView("PanelUI-help", aAnchor);
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
  },

  
  _onHelpViewClick: function(aEvent) {
    if (aEvent.button == 0 && !aEvent.target.hasAttribute("disabled")) {
      PanelUI.hide();
    }
  },

  _onHelpViewShow: function(aEvent) {
    
    buildHelpMenu();

    let helpMenu = document.getElementById("menu_HelpPopup");
    let items = this.getElementsByTagName("vbox")[0];
    let attrs = ["oncommand", "onclick", "label", "key", "disabled"];
    let NSXUL = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

    
    while (items.firstChild) {
      items.removeChild(items.firstChild);
    }

    
    let menuItems = Array.prototype.slice.call(helpMenu.getElementsByTagName("menuitem"));
    let fragment = document.createDocumentFragment();
    for (let node of menuItems) {
      if (node.hidden)
        continue;
      let button = document.createElementNS(NSXUL, "toolbarbutton");
      
      for (let attrName of attrs) {
        if (!node.hasAttribute(attrName))
          continue;
        button.setAttribute(attrName, node.getAttribute(attrName));
      }
      fragment.appendChild(button);
    }
    items.appendChild(fragment);

    this.addEventListener("click", PanelUI._onHelpViewClick, false);
  },

  _onHelpViewHide: function(aEvent) {
    this.removeEventListener("click", PanelUI._onHelpViewClick);
  }
};
