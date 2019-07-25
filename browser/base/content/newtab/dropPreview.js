#ifdef 0



#endif






let gDropPreview = {
  





  rearrange: function DropPreview_rearrange(aCell) {
    let sites = gGrid.sites;

    
    this._insertDraggedSite(sites, aCell);

    
    
    this._repositionPinnedSites(sites, aCell);

    return sites;
  },

  




  _insertDraggedSite: function DropPreview_insertDraggedSite(aSites, aCell) {
    let dropIndex = aCell.index;
    let draggedSite = gDrag.draggedSite;

    
    if (draggedSite) {
      let dragCell = draggedSite.cell;
      let dragIndex = dragCell.index;

      
      if (dragIndex != dropIndex) {
        aSites.splice(dragIndex, 1);
        aSites.splice(dropIndex, 0, draggedSite);
      }
    
    } else {
      aSites.splice(dropIndex, 0, null);
    }
  },

  





  _repositionPinnedSites:
    function DropPreview_repositionPinnedSites(aSites, aCell) {

    
    let pinnedSites = this._filterPinnedSites(aSites, aCell);

    
    pinnedSites.forEach(function (aSite) {
      aSites[aSites.indexOf(aSite)] = aSites[aSite.cell.index];
      aSites[aSite.cell.index] = aSite;
    }, this);

    
    
    if (this._hasOverflowedPinnedSite(aSites, aCell))
      this._repositionOverflowedPinnedSite(aSites, aCell);
  },

  






  _filterPinnedSites: function DropPreview_filterPinnedSites(aSites, aCell) {
    let draggedSite = gDrag.draggedSite;

    
    
    let range = this._getPinnedRange(aCell);

    return aSites.filter(function (aSite, aIndex) {
      
      if (!aSite || aSite == draggedSite || !aSite.isPinned())
        return false;

      let index = aSite.cell.index;

      
      return (index > range.end || index < range.start);
    });
  },

  




  _getPinnedRange: function DropPreview_getPinnedRange(aCell) {
    let dropIndex = aCell.index;
    let range = {start: dropIndex, end: dropIndex};

    
    if (aCell.containsPinnedSite()) {
      let links = gPinnedLinks.links;

      
      while (range.start && links[range.start - 1])
        range.start--;

      let maxEnd = links.length - 1;

      
      while (range.end < maxEnd && links[range.end + 1])
        range.end++;
    }

    return range;
  },

  






  _hasOverflowedPinnedSite:
    function DropPreview_hasOverflowedPinnedSite(aSites, aCell) {

    
    
    if (!aCell.containsPinnedSite())
      return false;

    let cells = gGrid.cells;

    
    if (aSites.length <= cells.length)
      return false;

    let overflowedSite = aSites[cells.length];

    
    return (overflowedSite && overflowedSite.isPinned());
  },

  






  _repositionOverflowedPinnedSite:
    function DropPreview_repositionOverflowedPinnedSite(aSites, aCell) {

    
    let index = this._indexOfLowerPrioritySite(aSites, aCell);

    if (index > -1) {
      let cells = gGrid.cells;
      let dropIndex = aCell.index;

      
      
      for (let i = index + 1, lastPosition = index; i < aSites.length; i++) {
        if (i != dropIndex) {
          aSites[lastPosition] = aSites[i];
          lastPosition = i;
        }
      }

      
      aSites.splice(cells.length, 1);
    }
  },

  






  _indexOfLowerPrioritySite:
    function DropPreview_indexOfLowerPrioritySite(aSites, aCell) {

    let cells = gGrid.cells;
    let dropIndex = aCell.index;

    
    
    
    for (let i = cells.length - 1; i >= 0; i--) {
      
      if (i == dropIndex)
        continue;

      let site = aSites[i];

      
      if (!site || !site.isPinned())
        return i;
    }

    return -1;
  }
};
