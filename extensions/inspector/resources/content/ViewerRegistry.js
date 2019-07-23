


















































const kViewerURLPrefix = "chrome://inspector/content/viewers/";
const kViewerRegURL  = "chrome://inspector/content/res/viewer-registry.rdf";




function ViewerRegistry() 
{
  this.mViewerHash = {};
}

ViewerRegistry.prototype = 
{
  
  

  
  
  
  

  mDS: null,
  mObserver: null,
  mViewerDS: null,
  mViewerHash: null,
  mFilters: null,
  
  get url() { return this.mURL; },

  

  load: function(aURL, aObserver)
  {
    this.mURL = aURL;
    this.mObserver = aObserver;
    RDFU.loadDataSource(aURL, new ViewerRegistryLoadObserver(this));
  },

  onError: function(aStatus, aErrorMsg)
  {
    this.mObserver.onViewerRegistryLoadError(aStatus, aErrorMsg);
  },

  onLoad: function(aDS)
  {
    this.mDS = aDS;
    this.prepareRegistry();
    this.mObserver.onViewerRegistryLoad();
  },

  prepareRegistry: function prepareRegistry()
  {
    this.mViewerDS = RDFArray.fromContainer(this.mDS, "inspector:viewers", kInspectorNSURI);

    
    var js, fn;
    this.mFilters = [];
    for (var i = 0; i < this.mViewerDS.length; ++i) {
      js = this.getEntryProperty(i, "filter");
      try {
        fn = new Function("object", "linkedViewer", js);
      } catch (ex) {
        fn = new Function("return false");
        debug("### ERROR - Syntax error in filter for viewer \"" + this.getEntryProperty(i, "description") + "\"\n");
      }
      this.mFilters.push(fn);
    }
  },

  
  
  
  
  
  
  getEntryURL: function(aIndex)
  {
    var uid = this.getEntryProperty(aIndex, "uid");
    return kViewerURLPrefix + uid + "/" + uid + ".xul";
  },

  

  









  findViewersForObject: function findViewersForObject(aObject, aPanelId,
                                                      aLinkedViewer)
  {
    
    var len = this.mViewerDS.length;
    var entry;
    var urls = [];
    for (var i = 0; i < len; ++i) {
      if (this.getEntryProperty(i, "panels").indexOf(aPanelId) == -1) {
        continue;
      }
      if (this.objectMatchesEntry(aObject, aLinkedViewer, i)) {
        if (this.getEntryProperty(i, "important")) {
          urls.unshift(i);
        } else {
          urls.push(i);
        }
      }
    }

    return urls;
  },

  








  objectMatchesEntry: function objectMatchesEntry(aObject, aLinkedViewer,
                                                  aIndex)
  {
    return this.mFilters[aIndex](aObject, aLinkedViewer);
  },

  
  
  
  
  
  
  cacheViewer: function(aViewer, aIndex)
  {
    var uid = this.getEntryProperty(aIndex, "uid");
    this.mViewerHash[uid] = { viewer: aViewer, entry: aIndex };
  },

  uncacheViewer: function(aViewer)
  {
    delete this.mViewerHash[aViewer.uid];
  },
  
  
  getViewerByUID: function(aUID)
  {
    return this.mViewerHash[aUID].viewer;
  },

  
  getEntryForViewer: function(aViewer)
  {
    return this.mViewerHash[aViewer.uid].entry;
  },

  
  getEntryByUID: function(aUID)
  {
    return this.mViewerHash[aUID].aIndex;
  },

  getEntryProperty: function(aIndex, aProp)
  {
    return this.mViewerDS.get(aIndex, aProp);
  },

  getEntryCount: function()
  {
    return this.mViewerDS.length;
  },

  
  

  addNewEntry: function(aUID, aDescription, aFilter)
  {
  },

  removeEntry: function(aIndex)
  {
  },

  saveRegistry: function()
  {
  }

};




function ViewerRegistryLoadObserver(aTarget) 
{
  this.mTarget = aTarget;
}

ViewerRegistryLoadObserver.prototype = {
  mTarget: null,

  onError: function(aStatus, aErrorMsg) 
  {
    this.mTarget.onError(aStatus, aErrorMsg);
  },

  onDataSourceReady: function(aDS) 
  {
    this.mTarget.onLoad(aDS);
  }
};
