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

  



  showSite: function (aSite) {
    let node = aSite.node;
    return this._setNodeOpacity(node, 1).then(() => {
      
      node.style.opacity = "";
    });
  },

  



  hideSite: function (aSite) {
    return this._setNodeOpacity(aSite.node, 0);
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

  






  slideSiteTo: function (aSite, aTarget, aOptions) {
    let currentPosition = this.getNodePosition(aSite.node);
    let targetPosition = this.getNodePosition(aTarget.node)
    let promise;

    
    targetPosition.left += this._cellBorderWidths.left;
    targetPosition.top += this._cellBorderWidths.top;

    
    if (currentPosition.left == targetPosition.left &&
        currentPosition.top == targetPosition.top) {
      promise = Promise.resolve();
    } else {
      this.setSitePosition(aSite, targetPosition);
      promise = this._whenTransitionEnded(aSite.node, ["left", "top"]);
    }

    if (aOptions && aOptions.unfreeze) {
      promise = promise.then(() => this.unfreezeSitePosition(aSite));
    }

    return promise;
  },

  






  rearrangeSites: function (aSites, aOptions) {
    let self = this;
    let cells = gGrid.cells;
    let unfreeze = aOptions && aOptions.unfreeze;

    function promises() {
      let index = 0;

      for (let site of aSites) {
        if (site && site !== gDrag.draggedSite) {
          if (!cells[index]) {
            
            yield self.hideSite(site);
          } else if (self._getNodeOpacity(site.node) != 1) {
            
            yield self.showSite(site);
          } else {
            
            yield self._moveSite(site, index, {unfreeze: unfreeze});
          }
        }
        index++;
      }
    }

    return Promise.every([p for (p of promises())]);
  },

  




  _whenTransitionEnded: function (aNode, aProperties) {
    let deferred = Promise.defer();
    let props = new Set(aProperties);
    aNode.addEventListener("transitionend", function onEnd(e) {
      if (props.has(e.propertyName)) {
        aNode.removeEventListener("transitionend", onEnd);
        deferred.resolve();
      }
    });
    return deferred.promise;
  },

  




  _getNodeOpacity: function Transformation_getNodeOpacity(aNode) {
    let cstyle = window.getComputedStyle(aNode, null);
    return cstyle.getPropertyValue("opacity");
  },

  




  _setNodeOpacity: function (aNode, aOpacity) {
    if (this._getNodeOpacity(aNode) == aOpacity) {
      return Promise.resolve();
    }

    aNode.style.opacity = aOpacity;
    return this._whenTransitionEnded(aNode, ["opacity"]);
  },

  





  _moveSite: function (aSite, aIndex, aOptions) {
    this.freezeSitePosition(aSite);
    return this.slideSiteTo(aSite, gGrid.cells[aIndex], aOptions);
  },

  




  _isFrozen: function Transformation_isFrozen(aSite) {
    return aSite.node.hasAttribute("frozen");
  }
};
