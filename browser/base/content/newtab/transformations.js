#ifdef 0



#endif






let gTransformation = {
  




  getNodePosition: function Transformation_getNodePosition(aNode) {
    let {left, top, width, height} = aNode.getBoundingClientRect();
    return new Rect(left + scrollX, top + scrollY, width, height);
  },

  




  fadeNodeIn: function Transformation_fadeNodeIn(aNode, aCallback) {
    this._setNodeOpacity(aNode, 1, function () {
      
      aNode.style.opacity = "";

      if (aCallback)
        aCallback();
    });
  },

  




  fadeNodeOut: function Transformation_fadeNodeOut(aNode, aCallback) {
    this._setNodeOpacity(aNode, 0, aCallback);
  },

  




  showSite: function Transformation_showSite(aSite, aCallback) {
    this.fadeNodeIn(aSite.node, aCallback);
  },

  




  hideSite: function Transformation_hideSite(aSite, aCallback) {
    this.fadeNodeOut(aSite.node, aCallback);
  },

  




  setSitePosition: function Transformation_setSitePosition(aSite, aPosition) {
    let style = aSite.node.style;
    let {top, left} = aPosition;

    style.top = top + "px";
    style.left = left + "px";
  },

  



  freezeSitePosition: function Transformation_freezeSitePosition(aSite) {
    aSite.node.setAttribute("frozen", "true");
    this.setSitePosition(aSite, this.getNodePosition(aSite.node));
  },

  



  unfreezeSitePosition: function Transformation_unfreezeSitePosition(aSite) {
    let style = aSite.node.style;
    style.left = style.top = "";
    aSite.node.removeAttribute("frozen");
  },

  







  slideSiteTo: function Transformation_slideSiteTo(aSite, aTarget, aOptions) {
    let currentPosition = this.getNodePosition(aSite.node);
    let targetPosition = this.getNodePosition(aTarget.node)
    let callback = aOptions && aOptions.callback;

    let self = this;

    function finish() {
      if (aOptions && aOptions.unfreeze)
        self.unfreezeSitePosition(aSite);

      if (callback)
        callback();
    }

    
    if (currentPosition.equals(targetPosition)) {
      finish();
    } else {
      this.setSitePosition(aSite, targetPosition);
      this._whenTransitionEnded(aSite.node, finish);
    }
  },

  







  rearrangeSites: function Transformation_rearrangeSites(aSites, aOptions) {
    let cells = gGrid.cells;
    let callback = aOptions && aOptions.callback;
    let unfreeze = aOptions && aOptions.unfreeze;

    let batch = new Batch(function () {
      if (aOptions && "callback" in aOptions)
        aOptions.callback();

      gGrid.unlock();
    });

    if (callback) {
      batch = new Batch(callback);
      callback = function () batch.pop();
    }

    gGrid.lock();

    aSites.forEach(function (aSite, aIndex) {
      
      if (!aSite || aSite == gDrag.draggedSite)
        return;

      if (batch)
        batch.push();

      if (!cells[aIndex])
        
        this.hideSite(aSite, callback);
      else if (this._getNodeOpacity(aSite.node) != 1)
        
        this.showSite(aSite, callback);
      else
        
        this._moveSite(aSite, aIndex, {unfreeze: unfreeze, callback: callback});
    }, this);

    if (batch)
      batch.close();
  },

  





  _whenTransitionEnded:
    function Transformation_whenTransitionEnded(aNode, aCallback) {

    aNode.addEventListener("transitionend", function onEnd() {
      aNode.removeEventListener("transitionend", onEnd, false);
      aCallback();
    }, false);
  },

  




  _getNodeOpacity: function Transformation_getNodeOpacity(aNode) {
    let cstyle = window.getComputedStyle(aNode, null);
    return cstyle.getPropertyValue("opacity");
  },

  





  _setNodeOpacity:
    function Transformation_setNodeOpacity(aNode, aOpacity, aCallback) {

    if (this._getNodeOpacity(aNode) == aOpacity) {
      if (aCallback)
        aCallback();
    } else {
      if (aCallback)
        this._whenTransitionEnded(aNode, aCallback);

      aNode.style.opacity = aOpacity;
    }
  },

  





  _moveSite: function Transformation_moveSite(aSite, aIndex, aOptions) {
    this.freezeSitePosition(aSite);
    this.slideSiteTo(aSite, gGrid.cells[aIndex], aOptions);
  }
};
