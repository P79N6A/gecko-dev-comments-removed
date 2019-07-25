#ifdef 0



#endif





let gUpdater = {
  




  updateGrid: function Updater_updateGrid(aCallback) {
    let links = gLinks.getLinks().slice(0, gGrid.cells.length);

    
    let sites = this._findRemainingSites(links);

    let self = this;

    
    this._removeLegacySites(sites, function () {
      
      
      self._freezeSitePositions(sites);

      
      
      
      self._moveSiteNodes(sites);

      
      
      self._rearrangeSites(sites, function () {
        
        self._fillEmptyCells(links, aCallback);

        
        gAllPages.update(gPage);
      });
    });
  },

  






  _findRemainingSites: function Updater_findRemainingSites(aLinks) {
    let map = {};

    
    gGrid.sites.forEach(function (aSite) {
      if (aSite)
        map[aSite.url] = aSite;
    });

    
    return aLinks.map(function (aLink) {
      return aLink && (aLink.url in map) && map[aLink.url];
    });
  },

  



  _freezeSitePositions: function Updater_freezeSitePositions(aSites) {
    aSites.forEach(function (aSite) {
      if (aSite)
        gTransformation.freezeSitePosition(aSite);
    });
  },

  



  _moveSiteNodes: function Updater_moveSiteNodes(aSites) {
    let cells = gGrid.cells;

    
    
    
    let sites = aSites.slice(0, cells.length);

    sites.forEach(function (aSite, aIndex) {
      let cell = cells[aIndex];
      let cellSite = cell.site;

      
      if (!aSite || cellSite != aSite) {
        let cellNode = cell.node;

        
        if (cellSite)
          cellNode.removeChild(cellSite.node);

        
        if (aSite)
          cellNode.appendChild(aSite.node);
      }
    }, this);
  },

  




  _rearrangeSites: function Updater_rearrangeSites(aSites, aCallback) {
    let options = {callback: aCallback, unfreeze: true};
    gTransformation.rearrangeSites(aSites, options);
  },

  





  _removeLegacySites: function Updater_removeLegacySites(aSites, aCallback) {
    let batch = new Batch(aCallback);

    
    gGrid.sites.forEach(function (aSite) {
      
      if (!aSite || aSites.indexOf(aSite) != -1)
        return;

      batch.push();

      
      gTransformation.hideSite(aSite, function () {
        let node = aSite.node;

        
        node.parentNode.removeChild(node);
        batch.pop();
      });
    });

    batch.close();
  },

  




  _fillEmptyCells: function Updater_fillEmptyCells(aLinks, aCallback) {
    let {cells, sites} = gGrid;
    let batch = new Batch(aCallback);

    
    sites.forEach(function (aSite, aIndex) {
      if (aSite || !aLinks[aIndex])
        return;

      batch.push();

      
      let site = gGrid.createSite(aLinks[aIndex], cells[aIndex]);

      
      site.node.style.opacity = 0;

      
      setTimeout(function () {
        gTransformation.showSite(site, function () batch.pop());
      }, 0);
    });

    batch.close();
  }
};
