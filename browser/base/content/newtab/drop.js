#ifdef 0



#endif



const DELAY_REARRANGE_MS = 100;




let gDrop = {
  


  _lastDropTarget: null,

  



  enter: function Drop_enter(aCell) {
    this._delayedRearrange(aCell);
  },

  




  exit: function Drop_exit(aCell, aEvent) {
    if (aEvent.dataTransfer && !aEvent.dataTransfer.mozUserCancelled) {
      this._delayedRearrange();
    } else {
      
      this._cancelDelayedArrange();
      this._rearrange();
    }
  },

  





  drop: function Drop_drop(aCell, aEvent, aCallback) {
    
    
    if (aCell.containsPinnedSite())
      this._repinSitesAfterDrop(aCell);

    
    this._pinDraggedSite(aCell, aEvent);

    this._cancelDelayedArrange();

    
    gUpdater.updateGrid(aCallback);
  },

  



  _repinSitesAfterDrop: function Drop_repinSitesAfterDrop(aCell) {
    let sites = gDropPreview.rearrange(aCell);

    
    let pinnedSites = sites.filter(function (aSite) {
      return aSite && aSite.isPinned();
    });

    
    pinnedSites.forEach(function (aSite) aSite.pin(sites.indexOf(aSite)), this);
  },

  




  _pinDraggedSite: function Drop_pinDraggedSite(aCell, aEvent) {
    let index = aCell.index;
    let draggedSite = gDrag.draggedSite;

    if (draggedSite) {
      
      if (aCell != draggedSite.cell)
        draggedSite.pin(index);
    } else {
      
      let dt = aEvent.dataTransfer;
      let [url, title] = dt.getData("text/x-moz-url").split(/[\r\n]+/);
      gPinnedLinks.pin({url: url, title: title}, index);
    }
  },

  



  _delayedRearrange: function Drop_delayedRearrange(aCell) {
    
    if (this._lastDropTarget == aCell)
      return;

    let self = this;

    function callback() {
      self._rearrangeTimeout = null;
      self._rearrange(aCell);
    }

    this._cancelDelayedArrange();
    this._rearrangeTimeout = setTimeout(callback, DELAY_REARRANGE_MS);

    
    this._lastDropTarget = aCell;
  },

  


  _cancelDelayedArrange: function Drop_cancelDelayedArrange() {
    if (this._rearrangeTimeout) {
      clearTimeout(this._rearrangeTimeout);
      this._rearrangeTimeout = null;
    }
  },

  



  _rearrange: function Drop_rearrange(aCell) {
    let sites = gGrid.sites;

    
    if (aCell)
      sites = gDropPreview.rearrange(aCell);

    gTransformation.rearrangeSites(sites, {unfreeze: !aCell});
  }
};
