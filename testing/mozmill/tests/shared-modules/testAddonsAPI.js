




































var MODULE_NAME = 'AddonsAPI';

const RELATIVE_ROOT = '.';
const MODULE_REQUIRES = ['DOMUtilsAPI', 'PrefsAPI', 'TabbedBrowsingAPI',
                         'UtilsAPI'];

const TIMEOUT = 5000;
const TIMEOUT_DOWNLOAD = 15000;
const TIMEOUT_SEARCH = 30000;


const SEARCH_FILTER = [
  "local",
  "remote"
];



const AMO_PREFERENCES = [
  {name: "extensions.getAddons.browseAddons", old: "addons.mozilla.org", new: "preview.addons.mozilla.org"},
  {name: "extensions.getAddons.recommended.browseURL", old: "addons.mozilla.org", new: "preview.addons.mozilla.org"},
  {name: "extensions.getAddons.recommended.url", old: "services.addons.mozilla.org", new: "preview.addons.mozilla.org"},
  {name: "extensions.getAddons.search.browseURL", old: "addons.mozilla.org", new: "preview.addons.mozilla.org"},
  {name: "extensions.getAddons.search.url", old: "services.addons.mozilla.org", new: "preview.addons.mozilla.org"},
  {name: "extensions.getMoreThemesURL", old: "addons.mozilla.org", new: "preview.addons.mozilla.org"}
];




function addonsManager(aController) {
  this._DOMUtilsAPI = collector.getModule('DOMUtilsAPI');
  this._TabbedBrowsingAPI = collector.getModule('TabbedBrowsingAPI');
  this._UtilsAPI = collector.getModule('UtilsAPI');

  this._controller = aController;
  this._tabBrowser = new this._TabbedBrowsingAPI.tabBrowser(this._controller);
}




addonsManager.prototype = {

  
  
  

  





  get controller() {
    return this._controller;
  },

  





  get dtds() {
    var dtds = [
      "chrome://mozapps/locale/extensions/extensions.dtd",
      "chrome://browser/locale/browser.dtd"
    ];

    return dtds;
  },

  















  open : function addonsManager_open(aSpec) {
    var spec = aSpec || { };
    var type = (spec.type == undefined) ? "menu" : spec.type;
    var waitFor = (spec.waitFor == undefined) ? true : spec.waitFor;

    switch (type) {
      case "menu":
        var menuItem = new elementslib.Elem(this._controller.
                                            menus["tools-menu"].menu_openAddons);
        this._controller.click(menuItem);
        break;
      case "shortcut":
        var cmdKey = this._UtilsAPI.getEntity(this.dtds, "addons.commandkey");
        this._controller.keypress(null, cmdKey, {accelKey: true, shiftKey: true});
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown event type - " +
                        event.type);
    }

    return waitFor ? this.waitForOpened() : null;
  },

  





  get isOpen() {
    return (this.getTabs().length > 0);
  },

  









  waitForOpened : function addonsManager_waitforOpened(aSpec) {
    var spec = aSpec || { };
    var timeout = (spec.timeout == undefined) ? TIMEOUT : spec.timeout;

    
    
    
    
    
    
    
    mozmill.utils.waitForEval("subject.isOpen", timeout, 100, this);

    
    var tab = this.getTabs()[0];
    tab.controller.waitForPageLoad();

    return tab;
  },

  






  close : function addonsManager_close(aSpec) {
    this._tabBrowser.closeTab(aSpec);
  },

  







  getTabs : function addonsManager_getTabs() {
    return this._TabbedBrowsingAPI.getTabsWithURL("about:addons");
  },

  









  handleUtilsButton : function addonsManager_handleUtilsButton(aSpec) {
    var spec = aSpec || { };
    var item = spec.item;

    if (!item)
      throw new Error(arguments.callee.name + ": Menu item not specified.");

    var button = this.getElement({type: "utilsButton"});
    var menu = this.getElement({type: "utilsButton_menu"});

    try {
      this._controller.click(button);

      
      
      
      
      
      
      
      mozmill.utils.waitForEval("subject && subject.state == 'open'",
                                TIMEOUT, 100, menu.getNode());

      
      var menuItem = this.getElement({
        type: "utilsButton_menuItem",
        value: "#utils-" + item
      });

      this._controller.click(menuItem);
    } finally {
      
      this._controller.keypress(menu, "VK_ESCAPE", {});
      
      
      
      
      
      
      mozmill.utils.waitForEval("subject && subject.state == 'closed'",
                                TIMEOUT, 100, menu.getNode());
    }
  },


  
  
  

  









  isAddonCompatible : function addonsManager_isAddonCompatible(aSpec) {
    var spec = aSpec || { };
    var addon = spec.addon;

    if (!addon)
      throw new Error(arguments.callee.name + ": Add-on not specified.");

    
    return addon.getNode().getAttribute("notification") != "warning";
  },

  









  isAddonEnabled : function addonsManager_isAddonEnabled(aSpec) {
    var spec = aSpec || { };
    var addon = spec.addon;

    if (!addon)
      throw new Error(arguments.callee.name + ": Add-on not specified.");

    return addon.getNode().getAttribute("active") == "true";
  },

  









  isAddonInstalled : function addonsManager_isAddonInstalled(aSpec) {
    var spec = aSpec || { };
    var addon = spec.addon;

    if (!addon)
      throw new Error(arguments.callee.name + ": Add-on not specified.");

    
    return addon.getNode().getAttribute("remote") == "false" &&
           addon.getNode().getAttribute("status") == "installed";
  },

  






  enableAddon : function addonsManager_enableAddon(aSpec) {
    var spec = aSpec || { };
    spec.button = "enable";

    var button = this.getAddonButton(spec);
    this._controller.click(button);
  },

  






  disableAddon : function addonsManager_disableAddon(aSpec) {
    var spec = aSpec || { };
    spec.button = "disable";

    var button = this.getAddonButton(spec);
    this._controller.click(button);
  },

  










  installAddon : function addonsManager_installAddon(aSpec) {
    var spec = aSpec || { };
    var addon = spec.addon;
    var timeout = spec.timeout;
    var button = "install";
    var waitFor = (spec.waitFor == undefined) ? true : spec.waitFor;

    var button = this.getAddonButton({addon: addon, button: button});
    this._controller.click(button);

    if (waitFor)
      this.waitForDownloaded({addon: addon, timeout: timeout});
  },

  






  removeAddon : function addonsManager_removeAddon(aSpec) {
    var spec = aSpec || { };
    spec.button = "remove";

    var button = this.getAddonButton(spec);
    this._controller.click(button);
  },

  






  undo : function addonsManager_undo(aSpec) {
    var spec = aSpec || { };
    spec.link = "undo";

    var link = this.getAddonLink(spec);
    this._controller.click(link);
  },

  













  getAddons : function addonsManager_addons(aSpec) {
    var spec = aSpec || {};

    return this.getElements({
      type: "addons",
      subtype: spec.attribute,
      value: spec.value,
      parent: this.selectedView
    });
  },

  










  getAddonButton : function addonsManager_getAddonButton(aSpec) {
    var spec = aSpec || { };
    var addon = spec.addon;
    var button = spec.button;

    if (!button)
      throw new Error(arguments.callee.name + ": Button not specified.");

    return this.getAddonChildElement({addon: addon, type: button + "Button"});
  },

  












  getAddonLink : function addonsManager_getAddonLink(aSpec) {
    var spec = aSpec || { };
    var addon = spec.addon;
    var link = spec.link;

    if (!link)
      throw new Error(arguments.callee.name + ": Link not specified.");

    return this.getAddonChildElement({addon: addon, type: link + "Link"});
  },

  











  getAddonRadiogroup : function addonsManager_getAddonRadiogroup(aSpec) {
    var spec = aSpec || { };
    var addon = spec.addon;
    var radiogroup = spec.radiogroup;

    if (!radiogroup)
      throw new Error(arguments.callee.name + ": Radiogroup not specified.");

    return this.getAddonChildElement({addon: addon, type: radiogroup + "Radiogroup"});
  },

  













  getAddonChildElement : function addonsManager_getAddonChildElement(aSpec) {
    var spec = aSpec || { };
    var addon = spec.addon;
    var attribute = spec.attribute;
    var value = spec.value;
    var type = spec.type;

    if (!addon)
      throw new Error(arguments.callee.name + ": Add-on not specified.");

    
    
    if (!type) {
      type = "element";

      if (!attribute)
        throw new Error(arguments.callee.name + ": DOM attribute not specified.");
      if (!value)
        throw new Error(arguments.callee.name + ": Value not specified.");
    }

    
    if (this.selectedView.getNode().id == "detail-view") {
      return this.getElement({
        type: "detailView_" + type,
        subtype: attribute,
        value: value
      });
    } else {
      return this.getElement({
        type: "listView_" + type,
        subtype: attribute,
        value: value,
        parent: addon
      });
    }
  },

  








  waitForDownloaded : function addonsManager_waitForDownloaded(aSpec) {
    var spec = aSpec || { };
    var addon = spec.addon;
    var timeout = (spec.timeout == undefined) ? TIMEOUT_DOWNLOAD : spec.timeout;

    if (!addon)
      throw new Error(arguments.callee.name + ": Add-on not specified.");

    var self = this;
    var node = addon.getNode();
    
    
    
    
    
    
    
    mozmill.utils.waitForEval("subject.getAttribute('pending') == 'install' &&" +
                              "subject.getAttribute('status') != 'installing'",
                              timeout, 100, node);
  },


  
  
  

  





  get selectedCategory() {
    return this.getCategories({attribute: "selected", value: "true"})[0];
  },

  












  getCategories : function addonsManager_categories(aSpec) {
    var spec = aSpec || { };

    var categories = this.getElements({
      type: "categories",
      subtype: spec.attribute,
      value: spec.value
    });

    if (categories.length == 0)
      throw new Error(arguments.callee.name + ": Categories could not be found.");

    return categories;
  },

  











  getCategoryById : function addonsManager_getCategoryById(aSpec) {
    var spec = aSpec || { };
    var id = spec.id;

    if (!id)
      throw new Error(arguments.callee.name + ": Category ID not specified.");

    return this.getCategories({
      attribute: "id",
      value: "category-" + id
    })[0];
  },

  









  getCategoryId : function addonsManager_getCategoryId(aSpec) {
    var spec = aSpec || { };
    var category = spec.category;

    if (!category)
      throw new Error(arguments.callee.name + ": Category not specified.");

    return category.getNode().id;
  },

  








  setCategory : function addonsManager_setCategory(aSpec) {
    var spec = aSpec || { };
    var category = spec.category;
    var waitFor = (spec.waitFor == undefined) ? true : spec.waitFor;

    if (!category)
      throw new Error(arguments.callee.name + ": Category not specified.");

    this._controller.click(category);

    if (waitFor)
      this.waitForCategory({category: category});
  },

  










  setCategoryById : function addonsManager_setCategoryById(aSpec) {
    var spec = aSpec || { };
    var id = spec.id;
    var waitFor = (spec.waitFor == undefined) ? true : spec.waitFor;

    if (!id)
      throw new Error(arguments.callee.name + ": Category ID not specified.");

    
    var category = this.getCategoryById({id: id});
    if (category)
      this.setCategory({category: category, waitFor: waitFor});
    else
      throw new Error(arguments.callee.name + ": Category '" + id + " not found.");
  },

  








  waitForCategory : function addonsManager_waitForCategory(aSpec) {
    var spec = aSpec || { };
    var category = spec.category;
    var timeout = (spec.timeout == undefined) ? TIMEOUT : spec.timeout;

    if (!category)
      throw new Error(arguments.callee.name + ": Category not specified.");

    
    
    
    
    
    
    mozmill.utils.waitForEval("subject.self.selectedCategory.getNode() == subject.aCategory.getNode()",
                               timeout, 100, 
                               {self: this, aCategory: category});
  },

  
  
  

  


  clearSearchField : function addonsManager_clearSearchField() {
    var textbox = this.getElement({type: "search_textbox"});
    var cmdKey = this._UtilsAPI.getEntity(this.dtds, "selectAllCmd.key");

    this._controller.keypress(textbox, cmdKey, {accelKey: true});
    this._controller.keypress(textbox, 'VK_DELETE', {});
  },

  










  search : function addonsManager_search(aSpec) {
    var spec = aSpec || { };
    var value = spec.value;
    var timeout = (spec.timeout == undefined) ? TIMEOUT_SEARCH : spec.timeout;
    var waitFor = (spec.waitFor == undefined) ? true : spec.waitFor;

    if (!value)
      throw new Error(arguments.callee.name + ": Search term not specified.");

    var textbox = this.getElement({type: "search_textbox"});

    this.clearSearchField();
    this._controller.type(textbox, value);
    this._controller.keypress(textbox, "VK_RETURN", {});

    if (waitFor)
      this.waitForSearchFinished();
  },

  





  get isSearching() {
    var throbber = this.getElement({type: "search_throbber"});
    return throbber.getNode().hasAttribute("active");
  },

  





  get selectedSearchFilter() {
    var filter = this.getSearchFilter({attribute: "selected", value: "true"});

    return (filter.length > 0) ? filter[0] : undefined;
  },

  





  set selectedSearchFilter(aValue) {
    var filter = this.getSearchFilter({attribute: "value", value: aValue});

    if (SEARCH_FILTER.indexOf(aValue) == -1)
      throw new Error(arguments.callee.name + ": '" + aValue +
                      "' is not a valid search filter");

    if (filter.length > 0) {
      this._controller.click(filter[0]);
      this.waitForSearchFilter({filter: filter[0]});
    }
  },

  












  getSearchFilter : function addonsManager_getSearchFilter(aSpec) {
    var spec = aSpec || { };

    return this.getElements({
      type: "search_filterRadioButtons",
      subtype: spec.attribute,
      value: spec.value
    });
  },

  








  getSearchFilterByValue : function addonsManager_getSearchFilterByValue(aValue) {
    if (!aValue)
      throw new Error(arguments.callee.name + ": Search filter value not specified.");

    return this.getElement({
      type: "search_filterRadioGroup",
      subtype: "value",
      value: aValue
    });
  },

  









  getSearchFilterValue : function addonsManager_getSearchFilterValue(aSpec) {
    var spec = aSpec || { };
    var filter = spec.filter;

    if (!filter)
      throw new Error(arguments.callee.name + ": Search filter not specified.");

    return filter.getNode().value;
  },

  








  waitForSearchFilter : function addonsManager_waitForSearchFilter(aSpec) {
    var spec = aSpec || { };
    var filter = spec.filter;
    var timeout = (spec.timeout == undefined) ? TIMEOUT : spec.timeout;

    if (!filter)
      throw new Error(arguments.callee.name + ": Search filter not specified.");

    
    
    
    
    
    
    
    mozmill.utils.waitForEval("subject.self.selectedSearchFilter.getNode() == subject.aFilter.getNode()",
                              timeout, 100,
                              {self: this, aFilter: filter});
  },

  





  getSearchResults : function addonsManager_getSearchResults() {
    var filterValue = this.getSearchFilterValue({
      filter: this.selectedSearchFilter
    });

    switch (filterValue) {
      case "local":
        return this.getAddons({attribute: "status", value: "installed"});
      case "remote":
        return this.getAddons({attribute: "remote", value: "true"});
      default:
        throw new Error(arguments.callee.name + ": Unknown search filter '" +
                        filterValue + "' selected");
    }
  },

  






  waitForSearchFinished : function addonsManager_waitForSearchFinished(aSpec) {
    var spec = aSpec || { };
    var timeout = (spec.timeout == undefined) ? TIMEOUT_SEARCH : spec.timeout;

    
    
    
    
    
    
    
    mozmill.utils.waitForEval("subject.isSearching == false", 
                              timeout, 100, this);
  },

  
  
  

  












  getViews : function addonsManager_getViews(aSpec) {
    var spec = aSpec || { };
    var attribute = spec.attribute;
    var value = spec.value;

    return this.getElements({type: "views", subtype: attribute, value: value});
  },

  





  get isDetailViewActive() {
    return (this.selectedView.getNode().id == "detail-view");
  },

  





  get selectedView() {
    var viewDeck = this.getElement({type: "viewDeck"});
    var views = this.getViews();

    return views[viewDeck.getNode().selectedIndex];
  },


  
  
  

  















  getElement : function addonsManager_getElement(aSpec) {
    var elements = this.getElements(aSpec);

    return (elements.length > 0) ? elements[0] : undefined;
  },

  















  getElements : function addonsManager_getElements(aSpec) {
    var spec = aSpec || { };
    var type = spec.type;
    var subtype = spec.subtype;
    var value = spec.value;
    var parent = spec.parent;

    var root = parent ? parent.getNode() : this._controller.tabs.activeTab;
    var nodeCollector = new this._DOMUtilsAPI.nodeCollector(root);

    switch (type) {
      
      case "addons":
        nodeCollector.queryNodes(".addon").filterByDOMProperty(subtype, value);
        break;
      case "addonsList":
        nodeCollector.queryNodes("#addon-list");
        break;
      
      case "categoriesList":
        nodeCollector.queryNodes("#categories");
        break;
      case "categories":
        nodeCollector.queryNodes(".category").filterByDOMProperty(subtype, value);
        break;
      
      case "detailView_element":
        nodeCollector.queryNodes(value);
        break;
      case "detailView_disableButton":
        nodeCollector.queryNodes("#detail-disable");
        break;
      case "detailView_enableButton":
        nodeCollector.queryNodes("#detail-enable");
        break;
      case "detailView_installButton":
        nodeCollector.queryNodes("#detail-install");
        break;
      case "detailView_preferencesButton":
        nodeCollector.queryNodes("#detail-prefs");
        break;
      case "detailView_removeButton":
        nodeCollector.queryNodes("#detail-uninstall");
        break;
      case "detailView_findUpdatesLink":
        nodeCollector.queryNodes("#detail-findUpdates");
        break;
      
      
      
      
      case "detailView_undoLink":
        nodeCollector.queryNodes("#detail-undo");
        break;
      case "detailView_findUpdatesRadiogroup":
        nodeCollector.queryNodes("#detail-findUpdates");
        break;
      
      case "listView_element":
        nodeCollector.queryAnonymousNodes(subtype, value);
        break;
      case "listView_disableButton":
        nodeCollector.queryAnonymousNodes("anonid", "disable-btn");
        break;
      case "listView_enableButton":
        nodeCollector.queryAnonymousNodes("anonid", "enable-btn");
        break;
      case "listView_installButton":
        
        nodeCollector.queryAnonymousNodes("anonid", "install-status");
        nodeCollector.root = nodeCollector.nodes[0];
        nodeCollector.queryAnonymousNodes("anonid", "install-remote");
        break;
      case "listView_preferencesButton":
        nodeCollector.queryAnonymousNodes("anonid", "preferences-btn");
        break;
      case "listView_removeButton":
        nodeCollector.queryAnonymousNodes("anonid", "remove-btn");
        break;
      case "listView_moreLink":
        
        nodeCollector.queryAnonymousNodes("class", "details button-link");
        break;
      
      
      
      
      case "listView_undoLink":
        nodeCollector.queryAnonymousNodes("anonid", "undo");
        break;
      case "listView_cancelDownload":
        
        nodeCollector.queryAnonymousNodes("anonid", "install-status");
        nodeCollector.root = nodeCollector.nodes[0];
        nodeCollector.queryAnonymousNodes("anonid", "cancel");
        break;
      case "listView_pauseDownload":
        
        nodeCollector.queryAnonymousNodes("anonid", "install-status");
        nodeCollector.root = nodeCollector.nodes[0];
        nodeCollector.queryAnonymousNodes("anonid", "pause");
        break;
      case "listView_progressDownload":
        
        nodeCollector.queryAnonymousNodes("anonid", "install-status");
        nodeCollector.root = nodeCollector.nodes[0];
        nodeCollector.queryAnonymousNodes("anonid", "progress");
        break;
      
      
      
      case "search_filterRadioButtons":
        nodeCollector.queryNodes(".search-filter-radio").filterByDOMProperty(subtype, value);
        break;
      case "search_filterRadioGroup":
        nodeCollector.queryNodes("#search-filter-radiogroup");
        break;
      case "search_textbox":
        nodeCollector.queryNodes("#header-search");
        break;
      case "search_throbber":
        nodeCollector.queryNodes("#header-searching");
        break;
      
      case "utilsButton":
        nodeCollector.queryNodes("#header-utils-btn");
        break;
      case "utilsButton_menu":
        nodeCollector.queryNodes("#utils-menu");
        break;
      case "utilsButton_menuItem":
        nodeCollector.queryNodes(value);
        break;
      
      case "viewDeck":
        nodeCollector.queryNodes("#view-port");
        break;
      case "views":
        nodeCollector.queryNodes(".view-pane").filterByDOMProperty(subtype, value);
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return nodeCollector.elements;
  }
};




function useAmoPreviewUrls() {
  var prefSrv = collector.getModule('PrefsAPI').preferences;

  for each (var preference in AMO_PREFERENCES) {
    var pref = prefSrv.getPref(preference.name, "");
    prefSrv.setPref(preference.name,
                    pref.replace(preference.old, preference.new));
  }
}




function resetAmoPreviewUrls() {
  var prefSrv = collector.getModule('PrefsAPI').preferences;

  for each (var preference in AMO_PREFERENCES) {
    prefSrv.clearUserPref(preference.name);
  }
}
