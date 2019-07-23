
















































var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;
var TYPE_MAYBE_FEED = "application/vnd.mozilla.maybe.feed";
const kXULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";




const PREF_DISABLED_PLUGIN_TYPES = "plugin.disable_full_page_plugin_for_types";


const PREF_SHOW_PLUGINS_IN_LIST = "browser.download.show_plugins_in_list";
const PREF_HIDE_PLUGINS_WITHOUT_EXTENSIONS =
  "browser.download.hide_plugins_without_extensions";
























const PREF_FEED_SELECTED_APP    = "browser.feeds.handlers.application";
const PREF_FEED_SELECTED_WEB    = "browser.feeds.handlers.webservice";
const PREF_FEED_SELECTED_ACTION = "browser.feeds.handler";
const PREF_FEED_SELECTED_READER = "browser.feeds.handler.default";





const kActionUsePlugin = 5;

const ICON_URL_APP      = "chrome://browser/skin/preferences/application.png";



const APP_ICON_ATTR_NAME = "appHandlerIcon";




function getDisplayNameForFile(aFile) {



  if (aFile instanceof Ci.nsILocalFileWin) {
    try {
      return aFile.getVersionInfoField("FileDescription"); 
    }
    catch(ex) {
      
    }
  }




  if (aFile instanceof Ci.nsILocalFileMac) {
    try {
      return aFile.bundleDisplayName;
    }
    catch(ex) {
      
    }
  }




  return Cc["@mozilla.org/network/io-service;1"].
         getService(Ci.nsIIOService).
         newFileURI(aFile).
         QueryInterface(Ci.nsIURL).
         fileName;
}

function getLocalHandlerApp(aFile) {
  var localHandlerApp = Cc["@mozilla.org/uriloader/local-handler-app;1"].
                        createInstance(Ci.nsILocalHandlerApp);
  localHandlerApp.name = getDisplayNameForFile(aFile);
  localHandlerApp.executable = aFile;

  return localHandlerApp;
}








function ArrayEnumerator(aItems) {
  this._index = 0;
  this._contents = aItems;
}

ArrayEnumerator.prototype = {
  _index: 0,

  hasMoreElements: function() {
    return this._index < this._contents.length;
  },

  getNext: function() {
    return this._contents[this._index++];
  }
};




















function HandlerInfoWrapper(aType, aHandlerInfo) {
  this._type = aType;
  this.wrappedHandlerInfo = aHandlerInfo;
}

HandlerInfoWrapper.prototype = {
  
  
  
  wrappedHandlerInfo: null,


  
  

  _handlerSvc: Cc["@mozilla.org/uriloader/handler-service;1"].
               getService(Ci.nsIHandlerService),

  
  
  _prefSvc: Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch).
            QueryInterface(Ci.nsIPrefBranch2),

  _categoryMgr: Cc["@mozilla.org/categorymanager;1"].
                getService(Ci.nsICategoryManager),

  element: function(aID) {
    return document.getElementById(aID);
  },


  
  

  
  _type: null,
  get type() {
    return this._type;
  },

  get description() {
    if (this.wrappedHandlerInfo.description)
      return this.wrappedHandlerInfo.description;

    if (this.primaryExtension) {
      var extension = this.primaryExtension.toUpperCase();
      return this.element("bundlePreferences").getFormattedString("fileEnding",
                                                                  [extension]);
    }

    return this.type;
  },

  get preferredApplicationHandler() {
    return this.wrappedHandlerInfo.preferredApplicationHandler;
  },

  set preferredApplicationHandler(aNewValue) {
    this.wrappedHandlerInfo.preferredApplicationHandler = aNewValue;

    
    if (aNewValue)
      this.addPossibleApplicationHandler(aNewValue)
  },

  get possibleApplicationHandlers() {
    return this.wrappedHandlerInfo.possibleApplicationHandlers;
  },

  addPossibleApplicationHandler: function(aNewHandler) {
    var possibleApps = this.possibleApplicationHandlers.enumerate();
    while (possibleApps.hasMoreElements()) {
      if (possibleApps.getNext().equals(aNewHandler))
        return;
    }
    this.possibleApplicationHandlers.appendElement(aNewHandler, false);
  },

  get hasDefaultHandler() {
    return this.wrappedHandlerInfo.hasDefaultHandler;
  },

  get defaultDescription() {
    return this.wrappedHandlerInfo.defaultDescription;
  },

  
  get preferredAction() {
    
    if (this.plugin && !this.isDisabledPluginType)
      return kActionUsePlugin;

    
    
    
    
    
    
    if (this.wrappedHandlerInfo.preferredAction == Ci.nsIHandlerInfo.useHelperApp &&
        !gApplicationsPane.isValidHandlerApp(this.preferredApplicationHandler)) {
      if (this.wrappedHandlerInfo.hasDefaultHandler)
        return Ci.nsIHandlerInfo.useSystemDefault;
      else
        return Ci.nsIHandlerInfo.saveToDisk;
    }

    return this.wrappedHandlerInfo.preferredAction;
  },

  set preferredAction(aNewValue) {
    
    
    
    

    if (aNewValue != kActionUsePlugin)
      this.wrappedHandlerInfo.preferredAction = aNewValue;
  },

  get alwaysAskBeforeHandling() {
    
    
    
    
    
    if (this.plugin && this.handledOnlyByPlugin)
      return false;

    
    
    
    
    
    
    if (!(this.wrappedHandlerInfo instanceof Ci.nsIMIMEInfo) &&
        this.preferredAction == Ci.nsIHandlerInfo.saveToDisk)
      return true;

    return this.wrappedHandlerInfo.alwaysAskBeforeHandling;
  },

  set alwaysAskBeforeHandling(aNewValue) {
    this.wrappedHandlerInfo.alwaysAskBeforeHandling = aNewValue;
  },


  
  

  
  
  
  
  
  get primaryExtension() {
    try {
      if (this.wrappedHandlerInfo instanceof Ci.nsIMIMEInfo &&
          this.wrappedHandlerInfo.primaryExtension)
        return this.wrappedHandlerInfo.primaryExtension
    } catch(ex) {}

    return null;
  },


  
  

  
  
  
  
  
  plugin: null,

  
  
  
  
  
  
  
  
  
  
  
  
  handledOnlyByPlugin: undefined,

  get isDisabledPluginType() {
    return this._getDisabledPluginTypes().indexOf(this.type) != -1;
  },

  _getDisabledPluginTypes: function() {
    var types = "";

    if (this._prefSvc.prefHasUserValue(PREF_DISABLED_PLUGIN_TYPES))
      types = this._prefSvc.getCharPref(PREF_DISABLED_PLUGIN_TYPES);

    
    
    if (types != "")
      return types.split(",");

    return [];
  },

  disablePluginType: function() {
    var disabledPluginTypes = this._getDisabledPluginTypes();

    if (disabledPluginTypes.indexOf(this.type) == -1)
      disabledPluginTypes.push(this.type);

    this._prefSvc.setCharPref(PREF_DISABLED_PLUGIN_TYPES,
                              disabledPluginTypes.join(","));

    
    this._categoryMgr.deleteCategoryEntry("Gecko-Content-Viewers",
                                          this.type,
                                          false);
  },

  enablePluginType: function() {
    var disabledPluginTypes = this._getDisabledPluginTypes();

    var type = this.type;
    disabledPluginTypes = disabledPluginTypes.filter(function(v) v != type);

    this._prefSvc.setCharPref(PREF_DISABLED_PLUGIN_TYPES,
                              disabledPluginTypes.join(","));

    
    this._categoryMgr.
      addCategoryEntry("Gecko-Content-Viewers",
                       this.type,
                       "@mozilla.org/content/plugin/document-loader-factory;1",
                       false,
                       true);
  },


  
  

  store: function() {
    this._handlerSvc.store(this.wrappedHandlerInfo);
  },


  
  

  get smallIcon() {
    return this._getIcon(16);
  },

  get largeIcon() {
    return this._getIcon(32);
  },

  _getIcon: function(aSize) {
    if (this.primaryExtension)
      return "moz-icon://goat." + this.primaryExtension + "?size=" + aSize;

    if (this.wrappedHandlerInfo instanceof Ci.nsIMIMEInfo)
      return "moz-icon://goat?size=" + aSize + "&contentType=" + this.type;

    
    
    return null;
  }

};


















var feedHandlerInfo = {

  __proto__: new HandlerInfoWrapper(TYPE_MAYBE_FEED, null),


  
  

  _converterSvc:
    Cc["@mozilla.org/embeddor.implemented/web-content-handler-registrar;1"].
    getService(Ci.nsIWebContentConverterService),

  _shellSvc:
#ifdef HAVE_SHELL_SERVICE
    Cc["@mozilla.org/browser/shell-service;1"].
    getService(Ci.nsIShellService),
#else
    null,
#endif


  
  

  get description() {
    return this.element("bundlePreferences").getString("webFeed");
  },

  get preferredApplicationHandler() {
    switch (this.element(PREF_FEED_SELECTED_READER).value) {
      case "client":
        var file = this.element(PREF_FEED_SELECTED_APP).value;
        if (file)
          return getLocalHandlerApp(file);

        return null;

      case "web":
        var uri = this.element(PREF_FEED_SELECTED_WEB).value;
        if (!uri)
          return null;
        return this._converterSvc.getWebContentHandlerByURI(TYPE_MAYBE_FEED,
                                                            uri);

      case "bookmarks":
      default:
        
        
        
        return null;
    }
  },

  set preferredApplicationHandler(aNewValue) {
    if (aNewValue instanceof Ci.nsILocalHandlerApp) {
      this.element(PREF_FEED_SELECTED_APP).value = aNewValue.executable;
      this.element(PREF_FEED_SELECTED_READER).value = "client";
    }
    else if (aNewValue instanceof Ci.nsIWebContentHandlerInfo) {
      this.element(PREF_FEED_SELECTED_WEB).value = aNewValue.uri;
      this.element(PREF_FEED_SELECTED_READER).value = "web";
      
      
      
      
      
      
      this._converterSvc.setAutoHandler(this.type, aNewValue);
    }
  },

  _possibleApplicationHandlers: null,

  get possibleApplicationHandlers() {
    if (this._possibleApplicationHandlers)
      return this._possibleApplicationHandlers;

    
    
    this._possibleApplicationHandlers = {
      _inner: [],

      QueryInterface: function(aIID) {
        if (aIID.equals(Ci.nsIMutableArray) ||
            aIID.equals(Ci.nsIArray) ||
            aIID.equals(Ci.nsISupports))
          return this;

        throw Cr.NS_ERROR_NO_INTERFACE;
      },

      enumerate: function() {
        return new ArrayEnumerator(this._inner);
      },

      appendElement: function(aHandlerApp, aWeak) {
        this._inner.push(aHandlerApp);
      }
    };

    
    
    
    
    
    
    var preferredAppFile = this.element(PREF_FEED_SELECTED_APP).value;
    if (preferredAppFile) {
      let preferredApp = getLocalHandlerApp(preferredAppFile);
      let defaultApp = this._defaultApplicationHandler;
      if (!defaultApp || !defaultApp.equals(preferredApp))
        this._possibleApplicationHandlers.appendElement(preferredApp, false);
    }

    
    var webHandlers = this._converterSvc.getContentHandlers(this.type, {});
    for each (let webHandler in webHandlers)
      this._possibleApplicationHandlers.appendElement(webHandler, false);

    return this._possibleApplicationHandlers;
  },

  __defaultApplicationHandler: undefined,
  get _defaultApplicationHandler() {
    if (typeof this.__defaultApplicationHandler != "undefined")
      return this.__defaultApplicationHandler;

    var defaultFeedReader = null;
#ifdef HAVE_SHELL_SERVICE
    try {
      defaultFeedReader = this._shellSvc.defaultFeedReader;
    }
    catch(ex) {
      
    }
#endif

    if (defaultFeedReader) {
      let handlerApp = Cc["@mozilla.org/uriloader/local-handler-app;1"].
                       createInstance(Ci.nsIHandlerApp);
      handlerApp.name = getDisplayNameForFile(defaultFeedReader);
      handlerApp.QueryInterface(Ci.nsILocalHandlerApp);
      handlerApp.executable = defaultFeedReader;

      this.__defaultApplicationHandler = handlerApp;
    }
    else {
      this.__defaultApplicationHandler = null;
    }

    return this.__defaultApplicationHandler;
  },

  get hasDefaultHandler() {
#ifdef HAVE_SHELL_SERVICE
    try {
      if (this._shellSvc.defaultFeedReader)
        return true;
    }
    catch(ex) {
      
    }
#endif

    return false;
  },

  get defaultDescription() {
    if (this.hasDefaultHandler)
      return this._defaultApplicationHandler.name;

    
    return "";
  },

  
  get preferredAction() {
    switch (this.element(PREF_FEED_SELECTED_ACTION).value) {

      case "bookmarks":
        return Ci.nsIHandlerInfo.handleInternally;

      case "reader": {
        let preferredApp = this.preferredApplicationHandler;
        let defaultApp = this._defaultApplicationHandler;

        
        
        if (gApplicationsPane.isValidHandlerApp(preferredApp)) {
          if (defaultApp && defaultApp.equals(preferredApp))
            return Ci.nsIHandlerInfo.useSystemDefault;

          return Ci.nsIHandlerInfo.useHelperApp;
        }

        
        
        
        
        return Ci.nsIHandlerInfo.handleInternally;
      }

      
      
      
      case "ask":
      default:
        return Ci.nsIHandlerInfo.handleInternally;
    }
  },

  set preferredAction(aNewValue) {
    switch (aNewValue) {

      case Ci.nsIHandlerInfo.handleInternally:
        this.element(PREF_FEED_SELECTED_READER).value = "bookmarks";
        break;

      case Ci.nsIHandlerInfo.useHelperApp:
        this.element(PREF_FEED_SELECTED_ACTION).value = "reader";
        
        
        break;

      case Ci.nsIHandlerInfo.useSystemDefault:
        this.element(PREF_FEED_SELECTED_ACTION).value = "reader";
        this.preferredApplicationHandler = this._defaultApplicationHandler;
        break;
    }
  },

  get alwaysAskBeforeHandling() {
    return this.element(PREF_FEED_SELECTED_ACTION).value == "ask";
  },

  set alwaysAskBeforeHandling(aNewValue) {
    if (aNewValue == true)
      this.element(PREF_FEED_SELECTED_ACTION).value = "ask";
    else
      this.element(PREF_FEED_SELECTED_ACTION).value = "reader";
  },

  
  
  
  
  
  
  _storingAction: false,


  
  

  get primaryExtension() {
    return "xml";
  },


  
  

  
  
  
  
  
  store: function() {},


  
  

  get smallIcon() {
    return "chrome://browser/skin/feeds/feedIcon16.png";
  },

  get largeIcon() {
    return "chrome://browser/skin/feeds/feedIcon.png";
  }

};





var gApplicationsPane = {
  
  
  _handledTypes: {},
  
  
  
  
  
  
  
  
  _visibleTypes: [],

  
  
  
  
  _visibleTypeDescriptionCount: {},


  
  

  
  _brandShortName : null,
  _prefsBundle    : null,
  _list           : null,
  _filter         : null,

  
  
  _prefSvc      : Cc["@mozilla.org/preferences-service;1"].
                  getService(Ci.nsIPrefBranch).
                  QueryInterface(Ci.nsIPrefBranch2),

  _mimeSvc      : Cc["@mozilla.org/mime;1"].
                  getService(Ci.nsIMIMEService),

  _helperAppSvc : Cc["@mozilla.org/uriloader/external-helper-app-service;1"].
                  getService(Ci.nsIExternalHelperAppService),

  _handlerSvc   : Cc["@mozilla.org/uriloader/handler-service;1"].
                  getService(Ci.nsIHandlerService),

  _ioSvc        : Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService),


  
  

  init: function() {
    
    this._brandShortName =
      document.getElementById("bundleBrand").getString("brandShortName");
    this._prefsBundle = document.getElementById("bundlePreferences");
    this._list = document.getElementById("handlersView");
    this._filter = document.getElementById("filter");

    
    
    this._prefSvc.addObserver(PREF_SHOW_PLUGINS_IN_LIST, this, false);
    this._prefSvc.addObserver(PREF_HIDE_PLUGINS_WITHOUT_EXTENSIONS, this, false);
    this._prefSvc.addObserver(PREF_FEED_SELECTED_APP, this, false);
    this._prefSvc.addObserver(PREF_FEED_SELECTED_WEB, this, false);
    this._prefSvc.addObserver(PREF_FEED_SELECTED_ACTION, this, false);
    this._prefSvc.addObserver(PREF_FEED_SELECTED_READER, this, false);

    
    window.addEventListener("unload", this, false);

    
    
    
    if (document.getElementById("actionColumn").hasAttribute("sortDirection")) {
      this._sortColumn = document.getElementById("actionColumn");
      
      
      
      
      document.getElementById("typeColumn").removeAttribute("sortDirection");
    }
    else 
      this._sortColumn = document.getElementById("typeColumn");

    
    
    
    
    
    
    
    var _delayedPaneLoad = function(self) {
      self._loadData();
      self._rebuildVisibleTypes();
      self._sortVisibleTypes();
      self._rebuildView();
    }
    setTimeout(_delayedPaneLoad, 0, this);
  },

  destroy: function() {
    window.removeEventListener("unload", this, false);
    this._prefSvc.removeObserver(PREF_SHOW_PLUGINS_IN_LIST, this);
    this._prefSvc.removeObserver(PREF_HIDE_PLUGINS_WITHOUT_EXTENSIONS, this);
    this._prefSvc.removeObserver(PREF_FEED_SELECTED_APP, this);
    this._prefSvc.removeObserver(PREF_FEED_SELECTED_WEB, this);
    this._prefSvc.removeObserver(PREF_FEED_SELECTED_ACTION, this);
    this._prefSvc.removeObserver(PREF_FEED_SELECTED_READER, this);
  },


  
  

  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIObserver) ||
        aIID.equals(Ci.nsIDOMEventListener ||
        aIID.equals(Ci.nsISupports)))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },


  
  

  observe: function (aSubject, aTopic, aData) {
    
    
    if (aTopic == "nsPref:changed" && !this._storingAction) {
      
      
      if (aData == PREF_SHOW_PLUGINS_IN_LIST ||
          aData == PREF_HIDE_PLUGINS_WITHOUT_EXTENSIONS) {
        this._rebuildVisibleTypes();
        this._sortVisibleTypes();
      }

      
      
      this._rebuildView();
    }
  },


  
  

  handleEvent: function(aEvent) {
    if (aEvent.type == "unload") {
      this.destroy();
    }
  },


  
  

  _loadData: function() {
    this._loadFeedHandler();
    this._loadPluginHandlers();
    this._loadApplicationHandlers();
  },

  _loadFeedHandler: function() {
    this._handledTypes[TYPE_MAYBE_FEED] = feedHandlerInfo;
    feedHandlerInfo.handledOnlyByPlugin = false;
  },

  



















  _loadPluginHandlers: function() {
    for (let i = 0; i < navigator.plugins.length; ++i) {
      let plugin = navigator.plugins[i];
      for (let j = 0; j < plugin.length; ++j) {
        let type = plugin[j].type;

        let handlerInfoWrapper;
        if (type in this._handledTypes)
          handlerInfoWrapper = this._handledTypes[type];
        else {
          let wrappedHandlerInfo =
            this._mimeSvc.getFromTypeAndExtension(type, null);
          handlerInfoWrapper = new HandlerInfoWrapper(type, wrappedHandlerInfo);
          handlerInfoWrapper.handledOnlyByPlugin = true;
          this._handledTypes[type] = handlerInfoWrapper;
        }

        handlerInfoWrapper.plugin = plugin;
      }
    }
  },

  


  _loadApplicationHandlers: function() {
    var wrappedHandlerInfos = this._handlerSvc.enumerate();
    while (wrappedHandlerInfos.hasMoreElements()) {
      let wrappedHandlerInfo =
        wrappedHandlerInfos.getNext().QueryInterface(Ci.nsIHandlerInfo);
      let type = wrappedHandlerInfo.type;

      let handlerInfoWrapper;
      if (type in this._handledTypes)
        handlerInfoWrapper = this._handledTypes[type];
      else {
        handlerInfoWrapper = new HandlerInfoWrapper(type, wrappedHandlerInfo);
        this._handledTypes[type] = handlerInfoWrapper;
      }

      handlerInfoWrapper.handledOnlyByPlugin = false;
    }
  },


  
  

  _rebuildVisibleTypes: function() {
    
    this._visibleTypes = [];
    this._visibleTypeDescriptionCount = {};

    
    var showPlugins = this._prefSvc.getBoolPref(PREF_SHOW_PLUGINS_IN_LIST);
    var hidePluginsWithoutExtensions =
      this._prefSvc.getBoolPref(PREF_HIDE_PLUGINS_WITHOUT_EXTENSIONS);

    for (let type in this._handledTypes) {
      let handlerInfo = this._handledTypes[type];

      
      
      
      
      
      
      
      if (hidePluginsWithoutExtensions && handlerInfo.handledOnlyByPlugin &&
          handlerInfo.wrappedHandlerInfo instanceof Ci.nsIMIMEInfo &&
          !handlerInfo.primaryExtension)
        continue;

      
      if (handlerInfo.handledOnlyByPlugin && !showPlugins)
        continue;

      
      this._visibleTypes.push(handlerInfo);

      if (handlerInfo.description in this._visibleTypeDescriptionCount)
        this._visibleTypeDescriptionCount[handlerInfo.description]++;
      else
        this._visibleTypeDescriptionCount[handlerInfo.description] = 1;
    }
  },

  _rebuildView: function() {
    
    while (this._list.childNodes.length > 1)
      this._list.removeChild(this._list.lastChild);

    var visibleTypes = this._visibleTypes;

    
    if (this._filter.value)
      visibleTypes = visibleTypes.filter(this._matchesFilter, this);

    for each (let visibleType in visibleTypes) {
      let item = document.createElement("richlistitem");
      item.setAttribute("type", visibleType.type);
      item.setAttribute("typeDescription", this._describeType(visibleType));
      if (visibleType.smallIcon)
        item.setAttribute("typeIcon", visibleType.smallIcon);
      item.setAttribute("actionDescription",
                        this._describePreferredAction(visibleType));

      if (!this._setIconClassForPreferredAction(visibleType, item)) {
        item.setAttribute("actionIcon",
                          this._getIconURLForPreferredAction(visibleType));
      }

      this._list.appendChild(item);
    }

    this._selectLastSelectedType();
  },

  _matchesFilter: function(aType) {
    var filterValue = this._filter.value.toLowerCase();
    return this._describeType(aType).toLowerCase().indexOf(filterValue) != -1 ||
           this._describePreferredAction(aType).toLowerCase().indexOf(filterValue) != -1;
  },

  









  _describeType: function(aHandlerInfo) {
    if (this._visibleTypeDescriptionCount[aHandlerInfo.description] > 1)
      return this._prefsBundle.getFormattedString("typeDescriptionWithType",
                                                  [aHandlerInfo.description,
                                                   aHandlerInfo.type]);

    return aHandlerInfo.description;
  },

  











  _describePreferredAction: function(aHandlerInfo) {
    
    
    
    if (aHandlerInfo.alwaysAskBeforeHandling) {
      if (aHandlerInfo.type == TYPE_MAYBE_FEED)
        return this._prefsBundle.getFormattedString("previewInApp",
                                                    [this._brandShortName]);
      else
        return this._prefsBundle.getString("alwaysAsk");
    }

    switch (aHandlerInfo.preferredAction) {
      case Ci.nsIHandlerInfo.saveToDisk:
        return this._prefsBundle.getString("saveFile");

      case Ci.nsIHandlerInfo.useHelperApp:
        var preferredApp = aHandlerInfo.preferredApplicationHandler;
        if (preferredApp instanceof Ci.nsILocalHandlerApp)
          return getDisplayNameForFile(preferredApp.executable);
        else
          return preferredApp.name;

      case Ci.nsIHandlerInfo.handleInternally:
        
        if (aHandlerInfo.type == TYPE_MAYBE_FEED)
          return this._prefsBundle.getFormattedString("liveBookmarksInApp",
                                                      [this._brandShortName]);

        
        
        
        if (this.isValidHandlerApp(aHandlerInfo.preferredApplicationHandler))
          return aHandlerInfo.preferredApplicationHandler.name;

        return aHandlerInfo.defaultDescription;

        
        
        
        

      case Ci.nsIHandlerInfo.useSystemDefault:
        return aHandlerInfo.defaultDescription;

      case kActionUsePlugin:
        return this._prefsBundle.getFormattedString("pluginName",
                                                    [aHandlerInfo.plugin.name,
                                                     this._brandShortName]);
    }
  },

  _selectLastSelectedType: function() {
    
    
    
    
    if (this._list.disabled)
      return;

    var lastSelectedType = this._list.getAttribute("lastSelectedType");
    if (!lastSelectedType)
      return;

    var item = this._list.getElementsByAttribute("type", lastSelectedType)[0];
    if (!item)
      return;

    this._list.selectedItem = item;
  },

  






  isValidHandlerApp: function(aHandlerApp) {
    if (!aHandlerApp)
      return false;

    if (aHandlerApp instanceof Ci.nsILocalHandlerApp)
      return this._isValidHandlerExecutable(aHandlerApp.executable);

    if (aHandlerApp instanceof Ci.nsIWebHandlerApp)
      return aHandlerApp.uriTemplate;

    if (aHandlerApp instanceof Ci.nsIWebContentHandlerInfo)
      return aHandlerApp.uri;

    return false;
  },

  _isValidHandlerExecutable: function(aExecutable) {
    return aExecutable &&
           aExecutable.exists() &&
           aExecutable.isExecutable() &&



#ifdef XP_WIN
#expand    aExecutable.leafName != "__MOZ_APP_NAME__.exe";
#else
#ifdef XP_MACOSX
#expand    aExecutable.leafName != "__MOZ_APP_DISPLAYNAME__.app";
#else
#expand    aExecutable.leafName != "__MOZ_APP_NAME__-bin";
#endif
#endif
  },

  



  rebuildActionsMenu: function() {
    var typeItem = this._list.selectedItem;
    var handlerInfo = this._handledTypes[typeItem.type];
    var menu =
      document.getAnonymousElementByAttribute(typeItem, "class", "actionsMenu");
    var menuPopup = menu.menupopup;

    
    while (menuPopup.hasChildNodes())
      menuPopup.removeChild(menuPopup.lastChild);

    {
      var askMenuItem = document.createElementNS(kXULNS, "menuitem");
      askMenuItem.setAttribute("alwaysAsk", "true");
      let label;
      if (handlerInfo.type == TYPE_MAYBE_FEED)
        label = this._prefsBundle.getFormattedString("previewInApp",
                                                     [this._brandShortName]);
      else
        label = this._prefsBundle.getString("alwaysAsk");
      askMenuItem.setAttribute("label", label);
      askMenuItem.setAttribute("tooltiptext", label);
      askMenuItem.setAttribute(APP_ICON_ATTR_NAME, "ask");
      menuPopup.appendChild(askMenuItem);
    }

    
    if (handlerInfo.type == TYPE_MAYBE_FEED) {
      var internalMenuItem = document.createElementNS(kXULNS, "menuitem");
      internalMenuItem.setAttribute("action", Ci.nsIHandlerInfo.handleInternally);
      let label = this._prefsBundle.getFormattedString("liveBookmarksInApp",
                                                       [this._brandShortName]);
      internalMenuItem.setAttribute("label", label);
      internalMenuItem.setAttribute("tooltiptext", label);
      internalMenuItem.setAttribute(APP_ICON_ATTR_NAME, "feed");
      menuPopup.appendChild(internalMenuItem);

      
      
      let menuItem = document.createElementNS(kXULNS, "menuseparator");
      menuPopup.appendChild(menuItem);
    }

    
    if (handlerInfo.hasDefaultHandler) {
      var defaultMenuItem = document.createElementNS(kXULNS, "menuitem");
      defaultMenuItem.setAttribute("action", Ci.nsIHandlerInfo.useSystemDefault);
      defaultMenuItem.setAttribute("label", handlerInfo.defaultDescription);
      defaultMenuItem.setAttribute("tooltiptext", handlerInfo.defaultDescription);
      defaultMenuItem.setAttribute("image", this._getIconURLForSystemDefault(handlerInfo));

      menuPopup.appendChild(defaultMenuItem);
    }

    
    let preferredApp = handlerInfo.preferredApplicationHandler;
    let possibleApps = handlerInfo.possibleApplicationHandlers.enumerate();
    var possibleAppMenuItems = [];
    while (possibleApps.hasMoreElements()) {
      let possibleApp = possibleApps.getNext();
      if (!this.isValidHandlerApp(possibleApp))
        continue;

      let menuItem = document.createElementNS(kXULNS, "menuitem");
      menuItem.setAttribute("action", Ci.nsIHandlerInfo.useHelperApp);
      let label;
      if (possibleApp instanceof Ci.nsILocalHandlerApp)
        label = getDisplayNameForFile(possibleApp.executable);
      else
        label = possibleApp.name;
      menuItem.setAttribute("label", label);
      menuItem.setAttribute("tooltiptext", label);
      menuItem.setAttribute("image", this._getIconURLForHandlerApp(possibleApp));

      
      
      menuItem.handlerApp = possibleApp;

      menuPopup.appendChild(menuItem);
      possibleAppMenuItems.push(menuItem);
    }

    
    if (handlerInfo.plugin) {
      var pluginMenuItem = document.createElementNS(kXULNS, "menuitem");
      pluginMenuItem.setAttribute("action", kActionUsePlugin);
      let label = this._prefsBundle.getFormattedString("pluginName",
                                                       [handlerInfo.plugin.name,
                                                        this._brandShortName]);
      pluginMenuItem.setAttribute("label", label);
      pluginMenuItem.setAttribute("tooltiptext", label);
      pluginMenuItem.setAttribute(APP_ICON_ATTR_NAME, "plugin");
      menuPopup.appendChild(pluginMenuItem);
    }

    
#ifdef XP_WIN
    
    
    var executableType = Cc["@mozilla.org/mime;1"].getService(Ci.nsIMIMEService)
                                                  .getTypeFromExtension("exe");
    if (handlerInfo.type != executableType)
#endif
    {
      let menuItem = document.createElementNS(kXULNS, "menuitem");
      menuItem.setAttribute("oncommand", "gApplicationsPane.chooseApp(event)");
      let label = this._prefsBundle.getString("chooseApp");
      menuItem.setAttribute("label", label);
      menuItem.setAttribute("tooltiptext", label);
      menuPopup.appendChild(menuItem);
    }

    
    
    
    
    if ((handlerInfo.wrappedHandlerInfo instanceof Ci.nsIMIMEInfo) &&
        handlerInfo.type != TYPE_MAYBE_FEED) {
      var saveMenuItem = document.createElementNS(kXULNS, "menuitem");
      saveMenuItem.setAttribute("action", Ci.nsIHandlerInfo.saveToDisk);
      let label = this._prefsBundle.getString("saveFile");
      saveMenuItem.setAttribute("label", label);
      saveMenuItem.setAttribute("tooltiptext", label);
      saveMenuItem.setAttribute(APP_ICON_ATTR_NAME, "save");
      menuPopup.appendChild(saveMenuItem);
    }

    
    
    
    
    if (handlerInfo.alwaysAskBeforeHandling)
      menu.selectedItem = askMenuItem;
    else switch (handlerInfo.preferredAction) {
      case Ci.nsIHandlerInfo.handleInternally:
        menu.selectedItem = internalMenuItem;
        break;
      case Ci.nsIHandlerInfo.useSystemDefault:
        menu.selectedItem = defaultMenuItem;
        break;
      case Ci.nsIHandlerInfo.useHelperApp:
        if (preferredApp)
          menu.selectedItem = 
            possibleAppMenuItems.filter(function(v) v.handlerApp.equals(preferredApp))[0];
        break;
      case kActionUsePlugin:
        menu.selectedItem = pluginMenuItem;
        break;
      case Ci.nsIHandlerInfo.saveToDisk:
        menu.selectedItem = saveMenuItem;
        break;
    }
  },


  
  

  _sortColumn: null,

  


  sort: function (event) {
    var column = event.target;

    
    
    if (this._sortColumn && this._sortColumn != column)
      this._sortColumn.removeAttribute("sortDirection");

    this._sortColumn = column;

    
    if (column.getAttribute("sortDirection") == "ascending")
      column.setAttribute("sortDirection", "descending");
    else
      column.setAttribute("sortDirection", "ascending");

    this._sortVisibleTypes();
    this._rebuildView();
  },

  


  _sortVisibleTypes: function() {
    if (!this._sortColumn)
      return;

    var t = this;

    function sortByType(a, b) {
      return t._describeType(a).toLowerCase().
             localeCompare(t._describeType(b).toLowerCase());
    }

    function sortByAction(a, b) {
      return t._describePreferredAction(a).toLowerCase().
             localeCompare(t._describePreferredAction(b).toLowerCase());
    }

    switch (this._sortColumn.getAttribute("value")) {
      case "type":
        this._visibleTypes.sort(sortByType);
        break;
      case "action":
        this._visibleTypes.sort(sortByAction);
        break;
    }

    if (this._sortColumn.getAttribute("sortDirection") == "descending")
      this._visibleTypes.reverse();
  },

  


  filter: function() {
    if (this._filter.value == "") {
      this.clearFilter();
      return;
    }

    this._rebuildView();

    document.getElementById("clearFilter").disabled = false;
  },

  _filterTimeout: null,

  onFilterInput: function() {
    if (this._filterTimeout)
      clearTimeout(this._filterTimeout);
   
    this._filterTimeout = setTimeout("gApplicationsPane.filter()", 500);
  },

  onFilterKeyPress: function(aEvent) {
    if (aEvent.keyCode == KeyEvent.DOM_VK_ESCAPE)
      this.clearFilter();
  },
  
  clearFilter: function() {
    this._filter.value = "";
    this._rebuildView();

    this._filter.focus();
    document.getElementById("clearFilter").disabled = true;
  },

  focusFilterBox: function() {
    this._filter.focus();
    this._filter.select();
  },


  
  

  onSelectAction: function(aActionItem) {
    this._storingAction = true;

    try {
      this._storeAction(aActionItem);
    }
    finally {
      this._storingAction = false;
    }
  },

  _storeAction: function(aActionItem) {
    var typeItem = this._list.selectedItem;
    var handlerInfo = this._handledTypes[typeItem.type];

    if (aActionItem.hasAttribute("alwaysAsk")) {
      handlerInfo.alwaysAskBeforeHandling = true;
    }
    else if (aActionItem.hasAttribute("action")) {
      let action = parseInt(aActionItem.getAttribute("action"));

      
      if (action == kActionUsePlugin)
        handlerInfo.enablePluginType();
      else if (handlerInfo.plugin && !handlerInfo.isDisabledPluginType)
        handlerInfo.disablePluginType();

      
      
      
      
      
      
      if (action == Ci.nsIHandlerInfo.useHelperApp)
        handlerInfo.preferredApplicationHandler = aActionItem.handlerApp;

      
      handlerInfo.alwaysAskBeforeHandling = false;

      
      handlerInfo.preferredAction = action;
    }

    handlerInfo.store();

    
    
    handlerInfo.handledOnlyByPlugin = false;

    
    typeItem.setAttribute("actionDescription",
                          this._describePreferredAction(handlerInfo));
    if (!this._setIconClassForPreferredAction(handlerInfo, typeItem)) {
      typeItem.setAttribute("actionIcon",
                            this._getIconURLForPreferredAction(handlerInfo));
    }
  },

  chooseApp: function(aEvent) {
    
    
    aEvent.stopPropagation();

    var handlerApp;

#ifdef XP_WIN
    var params = {};
    var handlerInfo = this._handledTypes[this._list.selectedItem.type];

    if (handlerInfo.type == TYPE_MAYBE_FEED) {
      
      params.mimeInfo = this._mimeSvc.getFromTypeAndExtension(handlerInfo.type, 
                                                 handlerInfo.primaryExtension);
    } else {
      params.mimeInfo = handlerInfo.wrappedHandlerInfo;
    }

    params.title         = this._prefsBundle.getString("fpTitleChooseApp");
    params.description   = handlerInfo.description;
    params.filename      = null;
    params.handlerApp    = null;

    window.openDialog("chrome://global/content/appPicker.xul", null,
                      "chrome,modal,centerscreen,titlebar,dialog=yes",
                      params);

    if (params.handlerApp && 
        params.handlerApp.executable && 
        params.handlerApp.executable.isFile()) {
      handlerApp = params.handlerApp;

      
      handlerInfo.addPossibleApplicationHandler(handlerApp);
    }
#else
    var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    var winTitle = this._prefsBundle.getString("fpTitleChooseApp");
    fp.init(window, winTitle, Ci.nsIFilePicker.modeOpen);
    fp.appendFilters(Ci.nsIFilePicker.filterApps);

    
    
    if (fp.show() == Ci.nsIFilePicker.returnOK && fp.file &&
        this._isValidHandlerExecutable(fp.file)) {
      handlerApp = Cc["@mozilla.org/uriloader/local-handler-app;1"].
                   createInstance(Ci.nsILocalHandlerApp);
      handlerApp.name = getDisplayNameForFile(fp.file);
      handlerApp.executable = fp.file;

      
      let handlerInfo = this._handledTypes[this._list.selectedItem.type];
      handlerInfo.addPossibleApplicationHandler(handlerApp);
    }
#endif

    
    
    
    this.rebuildActionsMenu();

    
    if (handlerApp) {
      let typeItem = this._list.selectedItem;
      let actionsMenu =
        document.getAnonymousElementByAttribute(typeItem, "class", "actionsMenu");
      let menuItems = actionsMenu.menupopup.childNodes;
      for (let i = 0; i < menuItems.length; i++) {
        let menuItem = menuItems[i];
        if (menuItem.handlerApp && menuItem.handlerApp.equals(handlerApp)) {
          actionsMenu.selectedIndex = i;
          this.onSelectAction(menuItem);
          break;
        }
      }
    }
  },

  
  
  onSelectionChanged: function() {
    if (this._list.selectedItem)
      this._list.setAttribute("lastSelectedType",
                              this._list.selectedItem.getAttribute("type"));
  },

  _setIconClassForPreferredAction: function(aHandlerInfo, aElement) {
    
    
    
    aElement.removeAttribute("actionIcon");

    if (aHandlerInfo.alwaysAskBeforeHandling) {
      aElement.setAttribute(APP_ICON_ATTR_NAME, "ask");
      return true;
    }

    switch (aHandlerInfo.preferredAction) {
      case Ci.nsIHandlerInfo.saveToDisk:
        aElement.setAttribute(APP_ICON_ATTR_NAME, "save");
        return true;

      case Ci.nsIHandlerInfo.handleInternally:
        if (aHandlerInfo.type == TYPE_MAYBE_FEED) {
          aElement.setAttribute(APP_ICON_ATTR_NAME, "feed");
          return true;
        }
        break;

      case kActionUsePlugin:
        aElement.setAttribute(APP_ICON_ATTR_NAME, "plugin");
        return true;
    }
    aElement.removeAttribute(APP_ICON_ATTR_NAME);
    return false;
  },

  _getIconURLForPreferredAction: function(aHandlerInfo) {
    switch (aHandlerInfo.preferredAction) {
      case Ci.nsIHandlerInfo.useSystemDefault:
        return this._getIconURLForSystemDefault(aHandlerInfo);

      case Ci.nsIHandlerInfo.useHelperApp:
        let (preferredApp = aHandlerInfo.preferredApplicationHandler) {
          if (this.isValidHandlerApp(preferredApp))
            return this._getIconURLForHandlerApp(preferredApp);
        }
        break;

      
      
      default:
        return ICON_URL_APP;
    }
  },

  _getIconURLForHandlerApp: function(aHandlerApp) {
    if (aHandlerApp instanceof Ci.nsILocalHandlerApp)
      return this._getIconURLForFile(aHandlerApp.executable);

    if (aHandlerApp instanceof Ci.nsIWebHandlerApp)
      return this._getIconURLForWebApp(aHandlerApp.uriTemplate);

    if (aHandlerApp instanceof Ci.nsIWebContentHandlerInfo)
      return this._getIconURLForWebApp(aHandlerApp.uri)

    
    return "";
  },

  _getIconURLForFile: function(aFile) {
    var fph = this._ioSvc.getProtocolHandler("file").
              QueryInterface(Ci.nsIFileProtocolHandler);
    var urlSpec = fph.getURLSpecFromFile(aFile);

    return "moz-icon://" + urlSpec + "?size=16";
  },

  _getIconURLForWebApp: function(aWebAppURITemplate) {
    var uri = this._ioSvc.newURI(aWebAppURITemplate, null, null);

    
    
    
    
    
    

    if (/^https?/.test(uri.scheme))
      return uri.prePath + "/favicon.ico";

    return "";
  },

  _getIconURLForSystemDefault: function(aHandlerInfo) {
    
    
    
    if ("wrappedHandlerInfo" in aHandlerInfo) {
      let wrappedHandlerInfo = aHandlerInfo.wrappedHandlerInfo;

      if (wrappedHandlerInfo instanceof Ci.nsIMIMEInfo &&
          wrappedHandlerInfo instanceof Ci.nsIPropertyBag) {
        try {
          let url = wrappedHandlerInfo.getProperty("defaultApplicationIconURL");
          if (url)
            return url + "?size=16";
        }
        catch(ex) {}
      }
    }

    
    
    
    return ICON_URL_APP;
  }

};
