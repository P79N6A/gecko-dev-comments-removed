
















































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

    
    if (aNewValue) {
      var found = false;
      var possibleApps = this.possibleApplicationHandlers.
                         QueryInterface(Ci.nsIArray).enumerate();
      while (possibleApps.hasMoreElements() && !found)
        found = possibleApps.getNext().equals(aNewValue);
      if (!found)
        this.possibleApplicationHandlers.appendElement(aNewValue, false);
    }
  },

  get possibleApplicationHandlers() {
    return this.wrappedHandlerInfo.possibleApplicationHandlers;
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

    
    
    
    if (!this.wrappedHandlerInfo.preferredAction) {
      if (gApplicationsPane.isValidHandlerApp(this.preferredApplicationHandler))
        return Ci.nsIHandlerInfo.useHelperApp;
      else
        return Ci.nsIHandlerInfo.useSystemDefault;
    }

    
    
    if (this.wrappedHandlerInfo.preferredAction == Ci.nsIHandlerInfo.useHelperApp &&
        !gApplicationsPane.isValidHandlerApp(this.preferredApplicationHandler))
      return Ci.nsIHandlerInfo.useSystemDefault;

    return this.wrappedHandlerInfo.preferredAction;
  },

  set preferredAction(aNewValue) {
    
    
    
    

    if (aNewValue != kActionUsePlugin)
      this.wrappedHandlerInfo.preferredAction = aNewValue;
  },

  get alwaysAskBeforeHandling() {
    
    
    
    
    
    if (this.plugin && this.handledOnlyByPlugin)
      return false;

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

  _shellSvc: Cc["@mozilla.org/browser/shell-service;1"].
             getService(Ci.nsIShellService),


  
  

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

  get possibleApplicationHandlers() {
    var handlerApps = Cc["@mozilla.org/array;1"].
                      createInstance(Ci.nsIMutableArray);

    
    
    
    
    var preferredAppFile = this.element(PREF_FEED_SELECTED_APP).value;
    if (preferredAppFile && preferredAppFile.exists()) {
      let preferredApp = getLocalHandlerApp(preferredAppFile);
      let defaultApp = this._defaultApplicationHandler;
      if (!defaultApp || !defaultApp.equals(preferredApp))
        handlerApps.appendElement(preferredApp, false);
    }

    
    var webHandlers = this._converterSvc.getContentHandlers(this.type, {});
    for each (let webHandler in webHandlers)
      handlerApps.appendElement(webHandler, false);

    return handlerApps;
  },

  __defaultApplicationHandler: undefined,
  get _defaultApplicationHandler() {
    if (typeof this.__defaultApplicationHandler != "undefined")
      return this.__defaultApplicationHandler;

    var defaultFeedReader;
    try {
      defaultFeedReader = this._shellSvc.defaultFeedReader;
    }
    catch(ex) {
      
    }

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
    try {
      if (this._shellSvc.defaultFeedReader)
        return true;
    }
    catch(ex) {
      
    }

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

      case "reader":
        let preferredApp = this.preferredApplicationHandler;
        let defaultApp = this._defaultApplicationHandler;

        
        
        if (gApplicationsPane.isValidHandlerApp(preferredApp)) {
          if (defaultApp && defaultApp.equals(preferredApp))
            return Ci.nsIHandlerInfo.useSystemDefault;

          return Ci.nsIHandlerInfo.useHelperApp;
        }

        
        
        
        
        return Ci.nsIHandlerInfo.handleInternally;

      
      
      
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


  
  

  get primaryExtension() {
    return "xml";
  },


  
  

  handledOnlyByPlugin: false,


  
  

  
  
  
  
  
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


  
  

  
  _brandShortName : null,
  _prefsBundle    : null,
  _list           : null,
  _filter         : null,

  
  
  _prefSvc      : Cc["@mozilla.org/preferences-service;1"].
                  getService(Ci.nsIPrefBranch).
                  QueryInterface(Ci.nsIPrefBranch2),

  _mimeSvc      : Cc["@mozilla.org/uriloader/external-helper-app-service;1"].
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

    
    
    
    if (document.getElementById("typeColumn").hasAttribute("sortDirection"))
      this._sortColumn = document.getElementById("typeColumn");
    else if (document.getElementById("actionColumn").hasAttribute("sortDirection"))
      this._sortColumn = document.getElementById("actionColumn");

    
    
    
    
    
    
    
    var _delayedPaneLoad = function(self) {
      self._loadData();
      self.rebuildView();
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
    
    
    if (aTopic == "nsPref:changed")
      this.rebuildView();
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
  },

  



















  _loadPluginHandlers: function() {
    for (let i = 0; i < navigator.plugins.length; ++i) {
      let plugin = navigator.plugins[i];
      for (let j = 0; j < plugin.length; ++j) {
        let type = plugin[j].type;
        let handlerInfoWrapper;

        if (typeof this._handledTypes[type] == "undefined") {
          let wrappedHandlerInfo =
            this._mimeSvc.getFromTypeAndExtension(type, null);
          handlerInfoWrapper = new HandlerInfoWrapper(type, wrappedHandlerInfo);
          this._handledTypes[type] = handlerInfoWrapper;
        }
        else
          handlerInfoWrapper = this._handledTypes[type];

        handlerInfoWrapper.plugin = plugin;
        handlerInfoWrapper.handledOnlyByPlugin = true;
      }
    }
  },

  


  _loadApplicationHandlers: function() {
    var wrappedHandlerInfos = this._handlerSvc.enumerate();
    while (wrappedHandlerInfos.hasMoreElements()) {
      let wrappedHandlerInfo = wrappedHandlerInfos.getNext().
                               QueryInterface(Ci.nsIHandlerInfo);
      let type = wrappedHandlerInfo.type;
      let handlerInfoWrapper;

      if (typeof this._handledTypes[type] == "undefined") {
        handlerInfoWrapper = new HandlerInfoWrapper(type, wrappedHandlerInfo);
        this._handledTypes[type] = handlerInfoWrapper;
      }
      else
        handlerInfoWrapper = this._handledTypes[type];

      handlerInfoWrapper.handledOnlyByPlugin = false;
    }
  },


  
  

  rebuildView: function() {
    
    while (this._list.childNodes.length > 1)
      this._list.removeChild(this._list.lastChild);

    var visibleTypes = this._getVisibleTypes();

    if (this._sortColumn)
      this._sortTypes(visibleTypes);

    for each (let visibleType in visibleTypes) {
      let item = document.createElement("richlistitem");
      item.setAttribute("type", visibleType.type);
      item.setAttribute("typeDescription", visibleType.description);
      item.setAttribute("typeIcon", visibleType.smallIcon);
      item.setAttribute("actionDescription",
                        this._describePreferredAction(visibleType));
      item.setAttribute("actionIcon",
                        this._getIconURLForPreferredAction(visibleType));
      this._list.appendChild(item);
    }

    this._selectLastSelectedType();
  },

  _getVisibleTypes: function() {
    var visibleTypes = [];

    var showPlugins = this._prefSvc.getBoolPref(PREF_SHOW_PLUGINS_IN_LIST);
    var hideTypesWithoutExtensions =
      this._prefSvc.getBoolPref(PREF_HIDE_PLUGINS_WITHOUT_EXTENSIONS);

    for (let type in this._handledTypes) {
      let handlerInfo = this._handledTypes[type];

      
      
      
      
      
      
      if (hideTypesWithoutExtensions &&
          handlerInfo.wrappedHandlerInfo instanceof Ci.nsIMIMEInfo &&
          !handlerInfo.primaryExtension)
        continue;

      
      if (handlerInfo.handledOnlyByPlugin && !showPlugins)
        continue;

      
      
      
      if (handlerInfo.handledOnlyByPlugin && handlerInfo.isDisabledPluginType)
        continue;

      
      
      
      
      if (handlerInfo.alwaysAskBeforeHandling &&
          handlerInfo.type != TYPE_MAYBE_FEED)
        continue;

      
      if (this._filter.value && !this._matchesFilter(handlerInfo))
        continue;

      
      visibleTypes.push(handlerInfo);
    }

    return visibleTypes;
  },

  _matchesFilter: function(aType) {
    var filterValue = this._filter.value.toLowerCase();
    return aType.description.toLowerCase().indexOf(filterValue) != -1 ||
           this._describePreferredAction(aType).toLowerCase().indexOf(filterValue) != -1;
  },

  










  _describePreferredAction: function(aHandlerInfo) {
    
    
    
    
    if (aHandlerInfo.alwaysAskBeforeHandling)
      return this._prefsBundle.getFormattedString("previewInApp",
                                                  [this._brandShortName]);

    switch (aHandlerInfo.preferredAction) {
      case Ci.nsIHandlerInfo.saveToDisk:
        return this._prefsBundle.getString("saveFile");

      case Ci.nsIHandlerInfo.useHelperApp:
        return getDisplayNameForFile(aHandlerInfo.preferredApplicationHandler.executable);

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
      return aHandlerApp.executable &&
             aHandlerApp.executable.exists() &&
             aHandlerApp.executable.isExecutable();

    if (aHandlerApp instanceof Ci.nsIWebHandlerApp)
      return aHandlerApp.uriTemplate;

    if (aHandlerApp instanceof Ci.nsIWebContentHandlerInfo)
      return aHandlerApp.uri;

    return false;
  },

  



  rebuildActionsMenu: function() {
    var typeItem = this._list.selectedItem;
    var handlerInfo = this._handledTypes[typeItem.type];
    var menu =
      document.getAnonymousElementByAttribute(typeItem, "class", "actionsMenu");
    var menuPopup = menu.firstChild;

    
    while (menuPopup.hasChildNodes())
      menuPopup.removeChild(menuPopup.lastChild);

    
    if (handlerInfo.type == TYPE_MAYBE_FEED) {
      let menuItem = document.createElementNS(kXULNS, "menuitem");
      menuItem.setAttribute("alwaysAsk", "true");
      let label = this._prefsBundle.getFormattedString("previewInApp",
                                                       [this._brandShortName]);
      menuItem.setAttribute("label", label);
      menuPopup.appendChild(menuItem);
      if (handlerInfo.alwaysAskBeforeHandling)
        menu.selectedItem = menuItem;

      menuItem = document.createElementNS(kXULNS, "menuitem");
      menuItem.setAttribute("action", Ci.nsIHandlerInfo.handleInternally);
      label = this._prefsBundle.getFormattedString("liveBookmarksInApp",
                                                   [this._brandShortName]);
      menuItem.setAttribute("label", label);
      menuItem.setAttribute("image", "chrome://browser/skin/page-livemarks.png");
      menuPopup.appendChild(menuItem);
      if (handlerInfo.preferredAction == Ci.nsIHandlerInfo.handleInternally)
        menu.selectedItem = menuItem;

      
      
      menuItem = document.createElementNS(kXULNS, "menuseparator");
      menuPopup.appendChild(menuItem);
    }

    
    if (handlerInfo.hasDefaultHandler) {
      let menuItem = document.createElementNS(kXULNS, "menuitem");
      menuItem.setAttribute("action", Ci.nsIHandlerInfo.useSystemDefault);
      menuItem.setAttribute("label", handlerInfo.defaultDescription);

      if (handlerInfo.wrappedHandlerInfo) {
        let iconURL =
          this._getIconURLForSystemDefault(handlerInfo.wrappedHandlerInfo);
        menuItem.setAttribute("image", iconURL);
      }

      menuPopup.appendChild(menuItem);
      if (handlerInfo.preferredAction == Ci.nsIHandlerInfo.useSystemDefault)
        menu.selectedItem = menuItem;
    }

    
    let preferredApp = handlerInfo.preferredApplicationHandler;
    let possibleApps = handlerInfo.possibleApplicationHandlers.
                       QueryInterface(Ci.nsIArray).enumerate();
    while (possibleApps.hasMoreElements()) {
      let possibleApp = possibleApps.getNext();
      if (!this.isValidHandlerApp(possibleApp))
        continue;

      let menuItem = document.createElementNS(kXULNS, "menuitem");
      menuItem.setAttribute("action", Ci.nsIHandlerInfo.useHelperApp);
      if (possibleApp instanceof Ci.nsILocalHandlerApp)
        menuItem.setAttribute("label", getDisplayNameForFile(possibleApp.executable));
      else
        menuItem.setAttribute("label", possibleApp.name);
      menuItem.setAttribute("image", this._getIconURLForHandlerApp(possibleApp));

      
      
      menuItem.handlerApp = possibleApp;

      menuPopup.appendChild(menuItem);

      
      
      if (handlerInfo.preferredAction == Ci.nsIHandlerInfo.useHelperApp &&
          preferredApp.equals(possibleApp))
        menu.selectedItem = menuItem;
    }

    
    if (handlerInfo.plugin) {
      let menuItem = document.createElementNS(kXULNS, "menuitem");
      menuItem.setAttribute("action", kActionUsePlugin);
      let label = this._prefsBundle.getFormattedString("pluginName",
                                                       [handlerInfo.plugin.name,
                                                        this._brandShortName]);
      menuItem.setAttribute("label", label);
      menuPopup.appendChild(menuItem);
      if (handlerInfo.preferredAction == kActionUsePlugin)
        menu.selectedItem = menuItem;
    }

    
    {
      let menuItem = document.createElementNS(kXULNS, "menuitem");
      menuItem.setAttribute("oncommand", "gApplicationsPane.chooseApp(event)");
      menuItem.setAttribute("label", this._prefsBundle.getString("chooseApp"));
      menuPopup.appendChild(menuItem);
    }

    
    
    
    
    
    
    
    if ((handlerInfo.wrappedHandlerInfo instanceof Ci.nsIMIMEInfo) &&
        handlerInfo.type != TYPE_MAYBE_FEED &&
        !handlerInfo.handledOnlyByPlugin) {
      let menuItem = document.createElementNS(kXULNS, "menuitem");
      menuItem.setAttribute("action", Ci.nsIHandlerInfo.saveToDisk);
      menuItem.setAttribute("label", this._prefsBundle.getString("saveFile"));
      menuPopup.appendChild(menuItem);
      if (handlerInfo.preferredAction == Ci.nsIHandlerInfo.saveToDisk)
        menu.selectedItem = menuItem;
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

    this.rebuildView();
  },

  




  _sortTypes: function(aTypes) {
    if (!this._sortColumn)
      return;

    function sortByType(a, b) {
      return a.description.toLowerCase().localeCompare(b.description.toLowerCase());
    }

    var t = this;
    function sortByAction(a, b) {
      return t._describePreferredAction(a).toLowerCase().
             localeCompare(t._describePreferredAction(b).toLowerCase());
    }

    switch (this._sortColumn.getAttribute("value")) {
      case "type":
        aTypes.sort(sortByType);
        break;
      case "action":
        aTypes.sort(sortByAction);
        break;
    }

    if (this._sortColumn.getAttribute("sortDirection") == "descending")
      aTypes.reverse();
  },

  


  filter: function() {
    if (this._filter.value == "") {
      this.clearFilter();
      return;
    }

    this.rebuildView();

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
    this.rebuildView();

    this._filter.focus();
    document.getElementById("clearFilter").disabled = true;
  },

  focusFilterBox: function() {
    this._filter.focus();
    this._filter.select();
  },


  
  

  onSelectAction: function(event) {
    var actionItem = event.originalTarget;
    var typeItem = this._list.selectedItem;
    var handlerInfo = this._handledTypes[typeItem.type];

    if (actionItem.hasAttribute("alwaysAsk")) {
      handlerInfo.alwaysAskBeforeHandling = true;
    }
    else if (actionItem.hasAttribute("action")) {
      let action = parseInt(actionItem.getAttribute("action"));

      
      if (action == kActionUsePlugin)
        handlerInfo.enablePluginType();
      else if (handlerInfo.plugin && !handlerInfo.isDisabledPluginType)
        handlerInfo.disablePluginType();

      
      
      
      
      
      
      if (action == Ci.nsIHandlerInfo.useHelperApp)
        handlerInfo.preferredApplicationHandler = actionItem.handlerApp;

      
      handlerInfo.alwaysAskBeforeHandling = false;

      
      handlerInfo.preferredAction = action;
    }

    handlerInfo.store();

    
    
    typeItem.setAttribute("actionDescription",
                          this._describePreferredAction(handlerInfo));
  },

  chooseApp: function(aEvent) {
    
    
    aEvent.stopPropagation();

    var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    var winTitle = this._prefsBundle.getString("fpTitleChooseApp");
    fp.init(window, winTitle, Ci.nsIFilePicker.modeOpen);
    fp.appendFilters(Ci.nsIFilePicker.filterApps);

    if (fp.show() == Ci.nsIFilePicker.returnOK && fp.file) {
      
      
      
#ifdef XP_WIN
#expand      if (fp.file.leafName == "__MOZ_APP_NAME__.exe")
#else
#ifdef XP_MACOSX
#expand      if (fp.file.leafName == "__MOZ_APP_DISPLAYNAME__.app")
#else
#expand      if (fp.file.leafName == "__MOZ_APP_NAME__-bin")
#endif
#endif
        { this.rebuildActionsMenu(); return; }

      let handlerApp = Cc["@mozilla.org/uriloader/local-handler-app;1"].
                       createInstance(Ci.nsIHandlerApp);
      handlerApp.name = getDisplayNameForFile(fp.file);
      handlerApp.QueryInterface(Ci.nsILocalHandlerApp);
      handlerApp.executable = fp.file;

      var handlerInfo = this._handledTypes[this._list.selectedItem.type];

      handlerInfo.preferredApplicationHandler = handlerApp;
      handlerInfo.preferredAction = Ci.nsIHandlerInfo.useHelperApp;

      handlerInfo.store();
    }

    
    
    
    this.rebuildActionsMenu();
  },

  
  
  onSelectionChanged: function() {
    if (this._list.selectedItem)
      this._list.setAttribute("lastSelectedType",
                              this._list.selectedItem.getAttribute("type"));
  },

  _getIconURLForPreferredAction: function(aHandlerInfo) {
    var preferredApp = aHandlerInfo.preferredApplicationHandler;

    if (aHandlerInfo.preferredAction == Ci.nsIHandlerInfo.useHelperApp &&
        this.isValidHandlerApp(preferredApp))
      return this._getIconURLForHandlerApp(preferredApp);

    if (aHandlerInfo.preferredAction == Ci.nsIHandlerInfo.useSystemDefault &&
        aHandlerInfo.wrappedHandlerInfo)
      return this._getIconURLForSystemDefault(aHandlerInfo.wrappedHandlerInfo);

    
    return "";
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
    
    
    
    if (aHandlerInfo instanceof Ci.nsIMIMEInfo &&
        aHandlerInfo instanceof Ci.nsIPropertyBag) {
      try {
        let url = aHandlerInfo.getProperty("defaultApplicationIconURL");
        if (url)
          return url + "?size=16";
      }
      catch(ex) {}
    }

    
    
    return "";
  }

};
