#ifdef 0



#endif





let gPage = {
  




  init: function Page_init(aToolbarSelector, aGridSelector) {
    gToolbar.init(aToolbarSelector);
    this._gridSelector = aGridSelector;

    
    gAllPages.register(this);

    
    function unload() { gAllPages.unregister(this); }
    addEventListener("unload", unload.bind(this), false);

    
    if (gAllPages.enabled)
      this._init();
    else
      this._updateAttributes(false);
  },

  


  observe: function Page_observe() {
    let enabled = gAllPages.enabled;
    this._updateAttributes(enabled);

    
    if (enabled)
      this._init();
  },

  


  update: function Page_update() {
    this.updateModifiedFlag();
    gGrid.refresh();
  },

  


  updateModifiedFlag: function Page_updateModifiedFlag() {
    let node = document.getElementById("toolbar-button-reset");
    let modified = this._isModified();

    if (modified)
      node.setAttribute("modified", "true");
    else
      node.removeAttribute("modified");

    this._updateTabIndices(gAllPages.enabled, modified);
  },

  



  _init: function Page_init() {
    if (this._initialized)
      return;

    this._initialized = true;

    gLinks.populateCache(function () {
      
      this.updateModifiedFlag();

      
      gGrid.init(this._gridSelector);

      
      gDropTargetShim.init();

      
      let doc = document.documentElement;
      doc.addEventListener("dragover", this.onDragOver, false);
      doc.addEventListener("drop", this.onDrop, false);
    }.bind(this));
  },

  



  _updateAttributes: function Page_updateAttributes(aValue) {
    let nodes = document.querySelectorAll("#grid, #scrollbox, #toolbar, .toolbar-button");

    
    for (let i = 0; i < nodes.length; i++) {
      let node = nodes[i];
      if (aValue)
        node.removeAttribute("page-disabled");
      else
        node.setAttribute("page-disabled", "true");
    }

    this._updateTabIndices(aValue, this._isModified());
  },

  



  _isModified: function Page_isModified() {
    
    return !gBlockedLinks.isEmpty();
  },

  




  _updateTabIndices: function Page_updateTabIndices(aEnabled, aModified) {
    function setFocusable(aNode, aFocusable) {
      if (aFocusable)
        aNode.removeAttribute("tabindex");
      else
        aNode.setAttribute("tabindex", "-1");
    }

    
    let nodes = document.querySelectorAll(".site, #toolbar-button-hide");
    for (let i = 0; i < nodes.length; i++)
      setFocusable(nodes[i], aEnabled);

    
    let btnShow = document.getElementById("toolbar-button-show");
    setFocusable(btnShow, !aEnabled);

    
    let btnReset = document.getElementById("toolbar-button-reset");
    setFocusable(btnReset, aEnabled && aModified);
  },

  




  onDragOver: function Page_onDragOver(aEvent) {
    if (gDrag.isValid(aEvent))
      aEvent.preventDefault();
  },

  




  onDrop: function Page_onDrop(aEvent) {
    if (gDrag.isValid(aEvent)) {
      aEvent.preventDefault();
      aEvent.stopPropagation();
    }
  }
};
