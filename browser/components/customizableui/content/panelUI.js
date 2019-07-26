



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
      panel: "PanelUI-popup"
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
  },

  _eventListenersAdded: false,
  _ensureEventListenersAdded: function() {
    if (this._eventListenersAdded)
      return;
    this._addEventListeners();
  },

  _addEventListeners: function() {
    for (let event of this.kEvents) {
      this.panel.addEventListener(event, this);
    }

    this.helpView.addEventListener("ViewShowing", this._onHelpViewShow, false);
    this.helpView.addEventListener("ViewHiding", this._onHelpViewHide, false);
    this._eventListenersAdded = true;
  },

  uninit: function() {
    for (let event of this.kEvents) {
      this.panel.removeEventListener(event, this);
    }
    this.helpView.removeEventListener("ViewShowing", this._onHelpViewShow);
    this.helpView.removeEventListener("ViewHiding", this._onHelpViewHide);
  },

  







  setMainView: function(aMainView) {
    this._ensureEventListenersAdded();
    this.multiView.setMainView(aMainView);
  },

  







  toggle: function(aEvent) {
    this._ensureEventListenersAdded();
    if (this.panel.state == "open") {
      this.hide();
    } else if (this.panel.state == "closed") {
      this.ensureRegistered();
      this.panel.hidden = false;
      let editControlPlacement = CustomizableUI.getPlacementOfWidget("edit-controls");
      if (editControlPlacement && editControlPlacement.area == CustomizableUI.AREA_PANEL) {
        updateEditUIVisibility();
      }

      let anchor;
      if (aEvent.type == "command") {
        anchor = this.menuButton;
      } else {
        anchor = aEvent.target;
      }
      let iconAnchor =
        document.getAnonymousElementByAttribute(anchor, "class",
                                                "toolbarbutton-icon");

      
      
      let keyboardOpened = aEvent.sourceEvent &&
                           aEvent.sourceEvent.target.localName == "key";
      this.panel.setAttribute("noautofocus", !keyboardOpened);
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

  









  ensureRegistered: function(aCustomizing=false) {
    if (aCustomizing) {
      CustomizableUI.registerMenuPanel(this.contents);
    } else {
      this.beginBatchUpdate();
      CustomizableUI.registerMenuPanel(this.contents);
      this.endBatchUpdate();
    }
  },

  



  showMainView: function() {
    this._ensureEventListenersAdded();
    this.multiView.showMainView();
  },

  



  showHelpView: function(aAnchor) {
    this._ensureEventListenersAdded();
    this.multiView.showSubView("PanelUI-help", aAnchor);
  },

  






  showSubView: function(aViewId, aAnchor, aPlacementArea) {
    this._ensureEventListenersAdded();
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
    } else if (!aAnchor.open) {
      aAnchor.open = true;
      
      
      let evt = document.createEvent("CustomEvent");
      evt.initCustomEvent("ViewShowing", true, true, viewNode);
      viewNode.dispatchEvent(evt);
      if (evt.defaultPrevented) {
        return;
      }

      let tempPanel = document.createElement("panel");
      tempPanel.setAttribute("type", "arrow");
      tempPanel.setAttribute("id", "customizationui-widget-panel");
      tempPanel.setAttribute("level", "top");
      document.getElementById(CustomizableUI.AREA_NAVBAR).appendChild(tempPanel);

      let multiView = document.createElement("panelmultiview");
      tempPanel.appendChild(multiView);
      multiView.setMainView(viewNode);
      CustomizableUI.addPanelCloseListeners(tempPanel);

      let panelRemover = function() {
        tempPanel.removeEventListener("popuphidden", panelRemover);
        CustomizableUI.removePanelCloseListeners(tempPanel);
        let evt = new CustomEvent("ViewHiding", {detail: viewNode});
        viewNode.dispatchEvent(evt);
        aAnchor.open = false;

        this.multiView.appendChild(viewNode);
        tempPanel.parentElement.removeChild(tempPanel);
      }.bind(this);
      tempPanel.addEventListener("popuphidden", panelRemover);

      let iconAnchor =
        document.getAnonymousElementByAttribute(aAnchor, "class",
                                                "toolbarbutton-icon");

      tempPanel.openPopup(iconAnchor || aAnchor, "bottomcenter topright");
    }
  },

  



  onCommandHandler: function(aEvent) {
    if (!aEvent.originalTarget.hasAttribute("noautoclose")) {
      PanelUI.hide();
    }
  },

  



  beginBatchUpdate: function() {
    this._ensureEventListenersAdded();
    this.multiView.ignoreMutations = true;
  },

  




  endBatchUpdate: function(aReason) {
    this._ensureEventListenersAdded();
    this.multiView.ignoreMutations = false;
  },

  



  _updatePanelButton: function() {
    this.menuButton.open = this.panel.state == "open" ||
                           this.panel.state == "showing";
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

    this.addEventListener("command", PanelUI.onCommandHandler);
  },

  _onHelpViewHide: function(aEvent) {
    this.removeEventListener("command", PanelUI.onCommandHandler);
  }
};
