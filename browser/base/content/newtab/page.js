#ifdef 0



#endif





let gPage = {
  


  init: function Page_init() {
    
    gAllPages.register(this);

    
    addEventListener("unload", this, false);

    
    let button = document.getElementById("newtab-toggle");
    button.addEventListener("click", this, false);

    
    let enabled = gAllPages.enabled;
    if (enabled)
      this._init();

    this._updateAttributes(enabled);
  },

  


  observe: function Page_observe() {
    let enabled = gAllPages.enabled;
    this._updateAttributes(enabled);

    
    if (enabled)
      this._init();
  },

  


  update: function Page_update() {
    gGrid.refresh();
  },

  



  _init: function Page_init() {
    if (this._initialized)
      return;

    this._initialized = true;

    gLinks.populateCache(function () {
      
      gGrid.init();

      
      gDropTargetShim.init();

#ifdef XP_MACOSX
      
      document.addEventListener("dragover", this, false);
      document.addEventListener("drop", this, false);
#endif
    }.bind(this));
  },

  



  _updateAttributes: function Page_updateAttributes(aValue) {
    let selector = "#newtab-scrollbox, #newtab-toggle, #newtab-grid";
    let nodes = document.querySelectorAll(selector);

    
    for (let i = 0; i < nodes.length; i++) {
      let node = nodes[i];
      if (aValue)
        node.removeAttribute("page-disabled");
      else
        node.setAttribute("page-disabled", "true");
    }

    
    let toggle = document.getElementById("newtab-toggle");
    toggle.setAttribute("title", newTabString(aValue ? "hide" : "show"));
  },

  


  handleEvent: function Page_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "unload":
        gAllPages.unregister(this);
        break;
      case "click":
        gAllPages.enabled = !gAllPages.enabled;
        break;
      case "dragover":
        if (gDrag.isValid(aEvent) && gDrag.draggedSite)
          aEvent.preventDefault();
        break;
      case "drop":
        if (gDrag.isValid(aEvent) && gDrag.draggedSite) {
          aEvent.preventDefault();
          aEvent.stopPropagation();
        }
        break;
    }
  }
};
