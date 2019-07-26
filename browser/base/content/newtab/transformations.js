#ifdef 0



#endif






let gTransformation = {
  



  get _cellBorderWidths() {
    let cstyle = window.getComputedStyle(gGrid.cells[0].node, null);
    let widths = {
      left: parseInt(cstyle.getPropertyValue("border-left-width")),
      top: parseInt(cstyle.getPropertyValue("border-top-width"))
    };

    
    Object.defineProperty(this, "_cellBorderWidths",
                          {value: widths, enumerable: true});

    return widths;
  },

  




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
    if (this._isFrozen(aSite))
      return;

    let style = aSite.node.style;
    let comp = getComputedStyle(aSite.node, null);
    style.width = comp.getPropertyValue("width")
    style.height = comp.getPropertyValue("height");

    aSite.node.setAttribute("frozen", "true");
    this.setSitePosition(aSite, this.getNodePosition(aSite.node));
  },

  



  unfreezeSitePosition: function Transformation_unfreezeSitePosition(aSite) {
    if (!this._isFrozen(aSite))
      return;

    let style = aSite.node.style;
    style.left = style.top = style.width = style.height = "";
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

    
    targetPosition.left += this._cellBorderWidths.left;
    targetPosition.top += this._cellBorderWidths.top;

    
    if (currentPosition.left == targetPosition.left &&
        currentPosition.top == targetPosition.top) {
      finish();
    } else {
      this.setSitePosition(aSite, targetPosition);
      this._whenTransitionEnded(aSite.node, finish);
    }
  },

  







  rearrangeSites: function Transformation_rearrangeSites(aSites, aOptions) {
    let batch = [];
    let cells = gGrid.cells;
    let callback = aOptions && aOptions.callback;
    let unfreeze = aOptions && aOptions.unfreeze;

    aSites.forEach(function (aSite, aIndex) {
      
      if (!aSite || aSite == gDrag.draggedSite)
        return;

      let deferred = Promise.defer();
      batch.push(deferred.promise);
      let cb = function () deferred.resolve();

      if (!cells[aIndex])
        
        this.hideSite(aSite, cb);
      else if (this._getNodeOpacity(aSite.node) != 1)
        
        this.showSite(aSite, cb);
      else
        
        this._moveSite(aSite, aIndex, {unfreeze: unfreeze, callback: cb});
    }, this);

    let wait = Promise.promised(function () callback && callback());
    wait.apply(null, batch);
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
  },

  




  _isFrozen: function Transformation_isFrozen(aSite) {
    return aSite.node.hasAttribute("frozen");
  }
};
