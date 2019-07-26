



'use strict';




let TopSites = {
  prepareCache: function(aForce){
    
    

    
    if (this._promisedCache && !aForce) {
      return this._promisedCache;
    }
    let deferred = Promise.defer();
    this._promisedCache = deferred.promise;

    NewTabUtils.links.populateCache(function () {
      deferred.resolve();
      this._promisedCache = null;
      this._sites = null;  
      this._sitesDirty.clear();
    }.bind(this), true);
    return this._promisedCache;
  },

  _sites: null,
  _sitesDirty: new Set(),
  getSites: function() {
    if (this._sites) {
      return this._sites;
    }

    let links = NewTabUtils.links.getLinks();
    let sites = links.map(function(aLink){
      let site = new Site(aLink);
      return site;
    });

    
    this._sites = sites;
    this._sitesDirty.clear();
    return this._sites;
  },

  



  dirty: function() {
    
    for (let i=0; i<arguments.length; i++) {
      this._sitesDirty.add(arguments[i]);
    }
    return this._sitesDirty;
  },

  


  update: function() {
    NewTabUtils.allPages.update();
    
    this._sitesDirty.clear();
  },

  




  pinSites: function(aSites, aSlotIndices) {
    if (aSites.length !== aSlotIndices.length)
        throw new Error("TopSites.pinSites: Mismatched sites/indices arguments");

    for (let i=0; i<aSites.length && i<aSlotIndices.length; i++){
      let site = aSites[i],
          idx = aSlotIndices[i];
      if (!(site && site.url)) {
        throw Cr.NS_ERROR_INVALID_ARG
      }
      
      NewTabUtils.pinnedLinks.pin(site, idx);
      this.dirty(site);
    }
    this.update();
  },

  



  unpinSites: function(aSites) {
    for (let site of aSites) {
      if (!(site && site.url)) {
        throw Cr.NS_ERROR_INVALID_ARG
      }
      
      NewTabUtils.pinnedLinks.unpin(site);
      this.dirty(site);
    }
    this.update();
  },

  



  hideSites: function(aSites) {
    for (let site of aSites) {
      if (!(site && site.url)) {
        throw Cr.NS_ERROR_INVALID_ARG
      }

      site._restorePinIndex = NewTabUtils.pinnedLinks._indexOfLink(site);
      
      NewTabUtils.blockedLinks.block(site);
    }
    
    this._sites = null;
    this._sitesDirty.clear();
    this.update();
  },

  



  restoreSites: function(aSites) {
    for (let site of aSites) {
      if (!(site && site.url)) {
        throw Cr.NS_ERROR_INVALID_ARG
      }
      NewTabUtils.blockedLinks.unblock(site);
      let pinIndex = site._restorePinIndex;

      if (!isNaN(pinIndex) && pinIndex > -1) {
        NewTabUtils.pinnedLinks.pin(site, pinIndex);
      }
    }
    
    this._sites = null;
    this._sitesDirty.clear();
    this.update();
  },

  _linkFromNode: function _linkFromNode(aNode) {
    return {
      url: aNode.getAttribute("value"),
      title: aNode.getAttribute("label")
    };
  }
};

