























































function inSearchModule(aBaseURL)
{
  this.mObservers = [];
  this.mBaseURL = aBaseURL;
}

inSearchModule.prototype = 
{
  mTitle: null,
  mBaseURL: null,
  mImpl: null,
  mDialogElementIds: null,
  mDialogURL: null,
  mContextMenuItems: null,
  mColumns: null,
  mColDelimiter: null,
  mNameSpace: null,
  
  mRDFArray: null,
  mResult: null,
  mObservers: null,

  mStartTime: null,
  mElapsed: 0,
  
  
  
  
  get searchService() { return this.mSearchService},
  set searchService(aVal) { this.mSearchService = aVal },
  
  get title() { return this.mTitle },
  get baseURL() { return this.mBaseURL },
  get defaultIconURL() { return this.mDefaultIconURL },

  get datasource() { return this.mRDFArray.datasource },
  get resultCount() { return this.mRDFArray.length },
  get progressPercent() { return this.mImpl.progressPercent },
  get progressText() { return this.mImp.progressText },
  get isPastMilestone() { return this.mImpl.isPastMilestone },
  get elapsed() { return this.mElapsed },
  
  
  
  
  initFromElement: function(aSearchEl)
  {
    this.parseSearch(aSearchEl);
    
    if (this.mImpl.constructor)
      this.mImpl.constructor();
  },
  
  
  

  parseSearch: function(aSearchEl)
  {
    
    this.mTitle = aSearchEl.getAttribute("title");
    
    this.mDefaultIconURL = aSearchEl.getAttribute("defaultIcon");
    
    var ns = aSearchEl.getAttribute("namespace")
    this.mNameSpace = ns ? ns : kInspectorNSURI;

    this.parseDialog(aSearchEl);
    this.parseContextMenu(aSearchEl);
    this.parseColumns(aSearchEl);
    this.parseImplementation(aSearchEl);
  },  

  parseDialog: function(aSearchEl)
  {
    var els = aSearchEl.getElementsByTagNameNS(kISMLNSURI, "dialog");
    if (els.length > 0) {
      var dialogEl = els[0];
      
      
      var ids = dialogEl.getAttribute("elements");
      if (ids)
        this.mDialogElementIds = ids.split(",");

      
      this.setDialogURL(dialogEl.getAttribute("href"));
      
      this.mDialogResizable = (dialogEl.getAttribute("resizable") == "true") ? true : false;
    }
  },  
  
  parseContextMenu: function(aSearchEl)
  {
    var els = aSearchEl.getElementsByTagNameNS(kISMLNSURI, "contextmenu");
    if (els.length > 0) {
      var kids = els[0].childNodes;
      this.mContextMenu = [];
      for (var i = 0; i < kids.length; ++i) {
        this.mContextMenu[i] = kids[i].cloneNode(true);
      }      
    }
  },  

  parseColumns: function(aSearchEl)
  {
    
    var els = aSearchEl.getElementsByTagNameNS(kISMLNSURI, "columns");
    if (els.length > 0) {
      this.mColumns = [];
      var cols = els[0];
      this.mColDelimiter = cols.getAttribute("delimiter");
      
      var kids = cols.childNodes;
      var col, data;
      for (var i= 0; i < kids.length; ++i) {
        col = kids[i];
        if (col.nodeType == Node.ELEMENT_NODE) { 
          data = { 
            name: col.getAttribute("name"), 
            title: col.getAttribute("title"), 
            flex: col.getAttribute("flex"), 
            className: col.getAttribute("class"),
            copy: col.getAttribute("copy") == "true"
          };
          this.mColumns.push(data);
        }
      }
    }
  },  

  parseImplementation: function(aSearchEl)
  {
    this.mImpl = this.getDefaultImplementation();
    
    
    var els = aSearchEl.getElementsByTagNameNS(kISMLNSURI, "implementation");
    if (els.length > 0) {
      var kids = aSearchEl.getElementsByTagNameNS(kISMLNSURI, "*");
      for (var i = 0; i < kids.length; i++) {
        if (kids[i].localName == "property")
          this.parseProperty(kids[i]);
        if (kids[i].localName == "method")
          this.parseMethod(kids[i]);
      }
    }
  },
  
  parseProperty: function(aPropEl)
  {
    var name = aPropEl.getAttribute("name");
    var fn = null;
    
    
    try {
      fn = this.getCodeTagFunction(aPropEl, "getter", null);
      if (fn)
        this.mImpl.__defineGetter__(name, fn);
    } catch (ex) {
      throw "### SYNTAX ERROR IN ISML GETTER \"" + name + "\" ###\n" + ex;
    }

    
    try {
      fn = this.getCodeTagFunction(aPropEl, "setter", ["val"]);
      if (fn)
        this.mImpl.__defineSetter__(name, fn);
    } catch (ex) {
      throw "### SYNTAX ERROR IN ISML SETTER \"" + name + "\" ###\n" + ex;
    }
  },
  
  parseMethod: function(aMethodEl)
  {
    var name = aMethodEl.getAttribute("name");
    var def = aMethodEl.getAttribute("defaultCommand") == "true";
    
    
    var els = aMethodEl.getElementsByTagNameNS(kISMLNSURI, "parameter");
    var params = [];
    for (var i = 0; i < els.length; i++) {
      params[i] = els[i].getAttribute("name");
    }
    
    
    try {
      var fn = this.getCodeTagFunction(aMethodEl, "body", params);
      this.mImpl[name] = fn;
      if (def)
        this.mImpl.__DefaultCmd__ = fn;
    } catch (ex) {
      throw "### SYNTAX ERROR IN ISML METHOD \"" + name + "\" ###\n" + ex;
    }    
  },
  
  getCodeTagFunction: function(aParent, aLocalName, aParams)
  {
    var els = aParent.getElementsByTagNameNS(kISMLNSURI, aLocalName);
    if (els.length) {
      var body = els[0];
      
      var node = body.childNodes.length > 0 ? body.firstChild : body;
      return this.getJSFunction(aParams, node.nodeValue);
    }
    
    return null;
  },  
  
  getJSFunction: function(aParams, aCode)
  {
    var params = "";
    if (aParams) {
      for (var i = 0; i < aParams.length; i++) {
        params += aParams[i];
        if (i < aParams.length-1) params += ",";
      }
    }
   
    var js = "function(" + params + ") " + 
      "{" + 
        (aCode ? aCode : "") + 
      "}";
    
    var fn;
    eval("fn = " + js);
    return fn;
  },
  
  getDefaultImplementation: function()
  {
    return { module: this };
  },

  
  
 
  openDialog: function()
  {
    window.openDialog(this.mDialogURL, "inSearchModule_dialog", 
      "chrome,modal,resizable="+this.mDialogResizable, this);
  },
  
  processDialog: function(aWindow)
  {
    var map = inFormManager.readWindow(aWindow, this.mDialogElementIds);
    this.implStartSearch(map);
  },

  
  

  startSearch: function()
  {
    if (this.mDialogURL) {
      this.openDialog();
    } else
      this.implStartSearch(null);  
  },
  
  implStartSearch: function(aMap)
  {
    this.mStartTime = new Date();
    this.mElapsed = 0;
    
    this.notifySearchStart();
    this.initDataSource();
    this.prepareForResult();
    this.mImpl.searchStart(aMap);    
  },
  
  stopSearch: function()
  {
    this.searchEnd();
  },
  
  
  

  setResultProperty: function(aAttr, aValue)
  {
    this.mResult[aAttr] = aValue;
  },
  
  searchResultReady: function()
  {
    this.mRDFArray.add(this.mResult);
    this.notifySearchResult();
    this.prepareForResult();
  },
  
  searchError: function(aMsg)
  {
  },
  
  searchEnd: function()
  {
    this.mElapsed = new Date() - this.mStartTime;
    this.notifySearchEnd();
  },

  
  

  get columnCount() { return this.mColumns.length },
  
  getColumn: function(aIndex) { return this.mColumns[aIndex] },
  getColumnName: function(aIndex) { return this.mColumns[aIndex].name },
  getColumnTitle: function(aIndex) { return this.mColumns[aIndex].title },
  getColumnClassName: function(aIndex) { return this.mColumns[aIndex].className },
  getColumnFlex: function(aIndex) { return this.mColumns[aIndex].flex },
 
  
  

  initDataSource: function()
  {
    this.mRDFArray = new RDFArray(this.mNameSpace, "inspector:searchResults", "results");
    this.mRDFArray.initialize();
  },
  
  getResultPropertyAt: function(aIndex, aProp)
  {
    return this.mRDFArray.get(aIndex, aProp);
  },
  
  getItemText: function(aIndex)
  {
    var cols = this.mColumns;
    var text = [];
    for (var i = 0; i < cols.length; ++i) {
      if (cols[i].copy) {
        text.push(this.getResultPropertyAt(aIndex, cols[i].name));
      }
    }
    return text.join(this.mColDelimiter);
  },

  
  
  
  installContextMenu: function(aMenu, aInsertionPoint, aDir)
  {
    if (this.mContextMenu) {
      aMenu._searchModule = this;
      var item;
      this.mMenuItems = [];
      if (this.mContextMenu.length == 0)
        aInsertionPoint.setAttribute("hide", "true");
      for (var i = 0; i < this.mContextMenu.length; ++i) {
        item = this.mContextMenu[i];
        this.mMenuItems.push(item);
        this.installSearchReference(item);
        if (aDir == inSearchService.INSERT_BEFORE)
          aMenu.insertBefore(item, aInsertionPoint);
        else {
         
        }
      }
    }
  },
  
  uninstallContextMenu: function(aMenu, aInsertionPoint, aDir)
  {
    if (this.mContextMenu) {
      if (this.mContextMenu.length == 0)
        aInsertionPoint.removeAttribute("hide");
      
      for (var i = 0; i < this.mContextMenu.length; ++i)
        aMenu.removeChild(this.mMenuItems[i]);
    }
  },

  installSearchReference: function(aItem)
  {
    if (aItem.nodeType == Node.ELEMENT_NODE) {
      if (aItem.localName == "menuitem") {
        aItem.search = this.mImpl;
        for (var i = 0; i < aItem.childNodes.length; ++i)
          this.installSearchReference(aItem.childNodes[i]);
      }
    }
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
      o[i].onSearchStart(this);
  },
  
  notifySearchResult: function()
  {
    var o = this.mObservers;
    for (var i = 0; i < o.length; i++)
      o[i].onSearchResult(this);
  },

  notifySearchEnd: function(aResult)
  {
    var o = this.mObservers;
    for (var i = 0; i < o.length; i++)
      o[i].onSearchEnd(this, aResult);
  },

  notifySearchError: function(aMsg)
  {
    var o = this.mObservers;
    for (var i = 0; i < o.length; i++)
      o[i].onSearchError(this, aMsg);
  },

  
  

  callDefaultCommand: function()
  {
    if (this.mImpl.__DefaultCmd__)
      this.mImpl.__DefaultCmd__();
  },
  
  prepareForResult: function()
  {
    this.mResult = { _icon: this.mDefaultIconURL };
  },

  setDialogURL: function(aURL)
  {
    this.mDialogURL = aURL;
    
    
    














  }  

};




