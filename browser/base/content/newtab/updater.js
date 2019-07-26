#ifdef 0



#endif





let gUpdater = {
  



  updateGrid: function Updater_updateGrid() {
    let links = gLinks.getLinks().slice(0, gGrid.cells.length);

    
    let sites = this._findRemainingSites(links);

    
    this._removeLegacySites(sites).then(() => {
      
      
      this._freezeSitePositions(sites);

      
      
      
      this._moveSiteNodes(sites);

      
      
      this._rearrangeSites(sites).then(() => {
        
        this._fillEmptyCells(links);

        
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

  



  _rearrangeSites: function (aSites) {
    return gTransformation.rearrangeSites(aSites, {unfreeze: true});
  },

  




  _removeLegacySites: function (aSites) {
    let remainingSites = new Set(aSites);

    function promises() {
      for (let site of gGrid.sites) {
        
        if (site && !remainingSites.has(site)) {
          
          let remove = site.node.remove.bind(site.node);
          yield gTransformation.hideSite(site).then(remove);
        }
      }
    }

    return Promise.every([p for (p of promises())]);
  },

  



  _fillEmptyCells: function (aLinks) {
    let {cells, sites} = gGrid;
    let index = 0;

    
    for (let site of sites) {
      if (!site && aLinks[index]) {
        
        site = gGrid.createSite(aLinks[index], cells[index]);

        
        site.node.style.opacity = 0;

        
        
        window.getComputedStyle(site.node).opacity;
        gTransformation.showSite(site);
      }

      index++;
    }
  }
};
