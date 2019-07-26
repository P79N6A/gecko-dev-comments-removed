



XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
                                  "resource:///modules/CustomizableUI.jsm");




const PanelUI = {
  
  get kEvents() ["popupshowing", "popupshown", "popuphiding", "popuphidden"],
  



  get kElements() {
    return {
      clickCapturer: "PanelUI-clickCapturer",
      container: "PanelUI-container",
      contents: "PanelUI-contents",
      mainView: "PanelUI-mainView",
      mainViewSpring: "PanelUI-mainView-spring",
      menuButton: "PanelUI-menu-button",
      panel: "PanelUI-popup",
      subViews: "PanelUI-subViews",
      viewStack: "PanelUI-viewStack"
    };
  },

  



  get _showingSubView() {
    return (this.viewStack.hasAttribute("view") &&
            this.viewStack.getAttribute("view") == "subview");
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

    this.clickCapturer.addEventListener("click", this._onCapturerClick,
                                        true);

    var self = this;
    this.subViews.addEventListener("overflow", function() {
      
      Services.tm.currentThread.dispatch(self._syncContainerWithSubView.bind(self),
        Ci.nsIThread.DISPATCH_NORMAL);
    });

    
    
    this._subViewObserver = new MutationObserver(function(aMutations) {
      this._syncContainerWithSubView();
    }.bind(this));
  },

  uninit: function() {
    for (let event of this.kEvents) {
      this.panel.removeEventListener(event, this);
    }

    this.clickCapturer.removeEventListener("click", this._onCapturerClick,
                                           true);
  },

  







  replaceMainView: function(aMainView) {
    this.viewStack.insertBefore(aMainView, this.viewStack.firstChild);
    this._syncContainerWithMainView();
  },

  







  toggle: function(aEvent) {
    if (this.panel.state == "open") {
      this.hide();
    } else if (this.panel.state == "closed") {
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
        this.ensureRegistered();
        let cstyle = window.getComputedStyle(this.viewStack, null);
        this.container.style.height = cstyle.getPropertyValue("height");
        this.container.style.width = cstyle.getPropertyValue("width");
        
      case "popupshown":
        
      case "popuphiding":
        
      case "popuphidden": {
        this.showMainView();
        this._updatePanelButton(aEvent.target);
        break;
      }
    }
  },

  





  ensureRegistered: function() {
    CustomizableUI.registerMenuPanel(this.contents);
  },

  



  showMainView: function() {
    
    if (this._showingSubView) {
      let viewNode = this._currentSubView;
      let evt = document.createEvent("CustomEvent");
      evt.initCustomEvent("ViewHiding", true, true, viewNode);
      viewNode.dispatchEvent(evt);

      viewNode.removeAttribute("current");
      this._currentSubView = null;
      this._subViewObserver.disconnect();
    }

    this.viewStack.setAttribute("view", "main");
    this._syncContainerWithMainView();

    this._shiftMainView();
  },

  




  showSubView: function(aViewId, aAnchor) {
    let viewNode = document.getElementById(aViewId);
    if (!viewNode) {
      Cu.reportError("Could not show panel subview with id: " + aViewId);
      return;
    }

    if (!aAnchor) {
      Cu.reportError("Expected an anchor when opening subview with id: " + aViewId);
      return;
    }

    let oldHeight = this.mainView.clientHeight;
    viewNode.setAttribute("current", true);
    this._currentSubView = viewNode;

    
    
    let evt = document.createEvent("CustomEvent");
    evt.initCustomEvent("ViewShowing", true, true, viewNode);
    viewNode.dispatchEvent(evt);

    this.viewStack.setAttribute("view", "subview");
    this.mainViewSpring.style.height = this.subViews.scrollHeight - oldHeight + "px";
    this.container.style.height = this.subViews.scrollHeight + "px";

    
    
    
    
    
    
    
    
    
    
    
    
    this._shiftMainView(aAnchor);
    this._subViewObserver.observe(viewNode, {
      attributes: true,
      characterData: true,
      childList: true,
      subtree: true
    });
  },

  




  _syncContainerWithMainView: function() {
    let springHeight = this.mainViewSpring.getBoundingClientRect().height;
    this.container.style.height = (this.mainView.scrollHeight - springHeight) + "px";
    this.mainViewSpring.style.height = "";
  },

  




  _syncContainerWithSubView: function() {
    let springHeight = this.mainViewSpring.getBoundingClientRect().height;
    let mainViewHeight = this.mainView.clientHeight - springHeight;
    this.container.style.height = this.subViews.scrollHeight + "px";
    this.mainViewSpring.style.height = (this.subViews.scrollHeight - mainViewHeight) + "px";
  },

  



  _updatePanelButton: function() {
    this.menuButton.open = this.panel.state == "open" ||
                           this.panel.state == "showing";
  },

  


  anchorElement: null,

  





  _shiftMainView: function(aAnchor) {
    if (aAnchor) {
      
      
      
      let anchorRect = aAnchor.getBoundingClientRect();
      let mainViewRect = this.mainView.getBoundingClientRect();
      let leftEdge = anchorRect.left - mainViewRect.left;
      let center = aAnchor.clientWidth / 2;
      let target = leftEdge + center;
      this.mainView.style.transform = "translateX(-" + target + "px)";
      aAnchor.classList.add("panelui-mainview-anchor");
    } else {
      this.mainView.style.transform = "";
      if (this.anchorElement)
        this.anchorElement.classList.remove("panelui-mainview-anchor");
    }
    this.anchorElement = aAnchor;
  },

  



  _onCapturerClick: function(aEvent) {
    PanelUI.showMainView();
  },
};
