























































var kSearchURLPrefix = "chrome://inspector/content/search/";




function inSearchService() 
{
  this.mInstances = {};
  this.mObservers = [];
  
  
  
  
  var browser = document.getElementById("inSearchServiceLoader");
  browser.addEventListener("pageshow", inSearchService_LoadListener, true);
  this.mWebNav = browser.webNavigation;
}


inSearchService.INSERT_BEFORE = 1;
inSearchService.INSERT_AFTER = 2;

inSearchService.prototype = 
{
  mInstances: null,
  mObservers: null,
  mCurrentModule: null,
  
  mTree: null,
  mContextMenu: null,
  mCMInsertPt: null,
  mCMInsert: inSearchService.INSERT_BEFORE,
  
  
  
  
  get currentModule() { return this.mCurrentModule },

  get resultsTree() { return this.mTree },
  set resultsTree(aTree) {
    
    if (this.mTree) throw "inSearchService.tree should only be set once"
    
    this.mTree = aTree;
    aTree._searchService = this;

    this.mTreeBuilder = new inSearchTreeBuilder(aTree, kInspectorNSURI, "results");
    this.mTreeBuilder.isIconic = true;

    
    
    var parent = aTree.parentNode;
    parent._tempTreeYuckyHack = aTree;
    parent.addEventListener("click", inSearchService_TreeClickListener, false);
  },

  get contextMenu() { return this.mContextMenu },
  set contextMenu(aVal) 
  { 
    this.mContextMenu = aVal;
    aVal._searchService = this;
  },
  
  get contextMenuInsertPt() { return this.mCMInsertPt },
  set contextMenuInsertPt(aVal) { this.mCMInsertPt = aVal },
  
  get contextMenuInsert() { return this.mCMInsert },
  set contextMenuInsert(aVal) { this.mCMInsert = aVal },
  
  
  

  startModule: function(aURL)
  {
    var instance = this.mInstances[aURL];
    if (instance)
      this.doStartModule(instance);
    else
      this.loadModule(aURL);
  },
  
  doStartModule: function(aModule)
  {
    aModule.startSearch();
  },
 
  startSearch: function(aModule)
  { 
    this.mCurrentModule = aModule;
    
    
    this.installContextMenu();
    
    
    this.mTreeBuilder.module = aModule;
  },    

  clearSearch: function()
  {
    var mod = this.mCurrentModule;
    if (mod) {
      
      this.mTreeBuilder.module = null;
      this.mTreeBuilder.buildContent();
      
      
      this.uninstallContextMenu();
    } 

    this.mCurrentModule = null;
  },

  
  

  loadModule: function(aURL)
  {
    this.mWebNav.loadURI(aURL, nsIWebNavigation.LOAD_FLAGS_NONE, null, null, null);
    this.mLoadingURL = aURL;
    




 
  },
  
  searchFileLoaded: function()
  {
    var mod = this.createModule(this.mWebNav.document);
    mod.addSearchObserver(this);
    this.mInstances[this.mLoadingURL] = mod;
    this.doStartModule(mod);
  },
  
  createModule: function(aDocument)
  {
    var mod = new inSearchModule(aDocument.location);
    mod.searchService = this;
    mod.initFromElement(aDocument.documentElement);
    
    return mod;    
  },
  
  
  

  onSearchStart: function(aModule) 
  {
    this.startSearch(aModule);
    this.notifySearchStart();
  },

  onSearchResult: function(aModule)
  {
    this.notifySearchResult();
  },

  onSearchEnd: function(aModule, aResult)
  {
    this.notifySearchEnd(aResult);
  },

  onSearchError: function(aModule, aMessage)
  {
    this.notifySearchError(aMessage);
    this.clearSearch();
  },

  
  

  get selectedItemCount()
  {
    return this.mTree ? this.mTree.selectedItems.length : null;
  },
  
  getSelectedIndex: function(aIdx)
  {
    if (this.mTree) {
      var items = this.mTree.selectedItems;
      return this.mTree.getIndexOfItem(items[aIdx]);
    }
    return null;
  },
  
  onTreeDblClick: function()
  {
    this.mCurrentModule.callDefaultCommand();
  },

  
  

  installContextMenu: function()
  {
    var mod = this.mCurrentModule;
    if (mod) {
      var menu = this.mContextMenu;
      menu.addEventListener("popupshowing", inSearchService_onCreatePopup, true);
      mod.installContextMenu(menu, this.mCMInsertPt, this.mCMInsert);
    }
  },
  
  uninstallContextMenu: function()
  {
    var mod = this.mCurrentModule;
    if (mod) {
      
      var menu = this.mContextMenu;
      menu.removeEventListener("popupshowing", inSearchService_onCreatePopup, true);
      mod.uninstallContextMenu(menu, this.mCMInsertPt, this.mCMInsert);
    }
  },

  onCreatePopup: function(aMenu)
  {
    
  },
  
  
  
  
  
  
  
  addSearchObserver: function(aObserver)
  {
    this.mObservers.push(aObserver);
  },
  
  removeSearchObserver: function(aObserver)
  {
    var o;
    var obs = this.mObservers;
    for (var i = 0; i < obs.length; i++) {
      o = obs[i];
      if (o == aObserver) {
        obs.splice(i, 1);
        return;
      }
    }
  },

  notifySearchStart: function()
  {
    var o = this.mObservers;
    for (var i = 0; i < o.length; i++)
      o[i].onSearchStart(this.mCurrentModule);
  },
  
  notifySearchResult: function()
  {
    var o = this.mObservers;
    for (var i = 0; i < o.length; i++)
      o[i].onSearchResult(this.mCurrentModule);
  },

  notifySearchEnd: function(aResult)
  {
    var o = this.mObservers;
    for (var i = 0; i < o.length; i++)
      o[i].onSearchEnd(this.mCurrentModule, aResult);
  },

  notifySearchError: function(aMsg)
  {
    var o = this.mObservers;
    for (var i = 0; i < o.length; i++)
      o[i].onSearchError(this.mCurrentModule, aMsg);
  }
  
};




function inSearchService_LoadListener(aEvent)
{
  inspector.searchRegistry.searchFileLoaded();
}

function inSearchService_TreeClickListener(aEvent)
{
  if (aEvent.detail == 2) {
    var tree = this._tempTreeYuckyHack;
    tree._searchService.onTreeDblClick();
  }
}

function inSearchService_onCreatePopup(aEvent)
{
  
  
  




}





























































