








































const MODULE_NAME = 'SearchAPI';


const RELATIVE_ROOT = '.';
const MODULE_REQUIRES = ['ModalDialogAPI', 'UtilsAPI'];

const TIMEOUT = 5000;


const MANAGER_BUTTONS   = '/id("engineManager")/anon({"anonid":"buttons"})';


const NAV_BAR             = '/id("main-window")/id("tab-view-deck")/{"flex":"1"}' +
                            '/id("navigator-toolbox")/id("nav-bar")';
const SEARCH_BAR          = NAV_BAR + '/id("search-container")/id("searchbar")';
const SEARCH_TEXTBOX      = SEARCH_BAR      + '/anon({"anonid":"searchbar-textbox"})';
const SEARCH_DROPDOWN     = SEARCH_TEXTBOX  + '/[0]/anon({"anonid":"searchbar-engine-button"})';
const SEARCH_POPUP        = SEARCH_DROPDOWN + '/anon({"anonid":"searchbar-popup"})';
const SEARCH_INPUT        = SEARCH_TEXTBOX  + '/anon({"class":"autocomplete-textbox-container"})' +
                                              '/anon({"anonid":"textbox-input-box"})' +
                                              '/anon({"anonid":"input"})';
const SEARCH_CONTEXT      = SEARCH_TEXTBOX  + '/anon({"anonid":"textbox-input-box"})' +
                                              '/anon({"anonid":"input-box-contextmenu"})';
const SEARCH_GO_BUTTON    = SEARCH_TEXTBOX  + '/anon({"class":"search-go-container"})' +
                                              '/anon({"class":"search-go-button"})';
const SEARCH_AUTOCOMPLETE =  '/id("main-window")/id("mainPopupSet")/id("PopupAutoComplete")';







function engineManager(controller)
{
  this._controller = controller;

  this._ModalDialogAPI = collector.getModule('ModalDialogAPI');
  this._WidgetsAPI = collector.getModule('WidgetsAPI');
}




engineManager.prototype = {
  





  get controller()
  {
    return this._controller;
  },

  





  get engines() {
    var engines = [ ];
    var tree = this.getElement({type: "engine_list"}).getNode();

    for (var ii = 0; ii < tree.view.rowCount; ii ++) {
      engines.push({name: tree.view.getCellText(ii, tree.columns.getColumnAt(0)),
                    keyword: tree.view.getCellText(ii, tree.columns.getColumnAt(1))});
    }

    return engines;
  },

  





  get selectedEngine() {
    var treeNode = this.getElement({type: "engine_list"}).getNode();

    if(this.selectedIndex != -1) {
      return treeNode.view.getCellText(this.selectedIndex,
                                       treeNode.columns.getColumnAt(0));
    } else {
      return null;
    }
  },

  





  set selectedEngine(name) {
    var treeNode = this.getElement({type: "engine_list"}).getNode();

    for (var ii = 0; ii < treeNode.view.rowCount; ii ++) {
      if (name == treeNode.view.getCellText(ii, treeNode.columns.getColumnAt(0))) {
        this.selectedIndex = ii;
        break;
      }
    }
  },

  





  get selectedIndex() {
    var tree = this.getElement({type: "engine_list"});
    var treeNode = tree.getNode();

    return treeNode.view.selection.currentIndex;
  },

  





  set selectedIndex(index) {
    var tree = this.getElement({type: "engine_list"});
    var treeNode = tree.getNode();

    if (index < treeNode.view.rowCount) {
      this._WidgetsAPI.clickTreeCell(this._controller, tree, index, 0, {});
    }

    this._controller.waitForEval("subject.manager.selectedIndex == subject.newIndex", TIMEOUT, 100,
                                 {manager: this, newIndex: index});
  },

  


  get suggestionsEnabled() {
    var checkbox = this.getElement({type: "suggest"});

    return checkbox.getNode().checked;
  },

  


  set suggestionsEnabled(state) {
    var checkbox = this.getElement({type: "suggest"});
    this._controller.check(checkbox, state);
  },

  







  close : function preferencesDialog_close(saveChanges) {
    saveChanges = (saveChanges == undefined) ? false : saveChanges;

    var button = this.getElement({type: "button", subtype: (saveChanges ? "accept" : "cancel")});
    this._controller.click(button);
  },

  







  editKeyword : function engineManager_editKeyword(name, handler)
  {
    if (!handler)
      throw new Error(arguments.callee.name + ": No callback handler specified.");

    
    this.selectedEngine = name;

    
    md = new this._ModalDialogAPI.modalDialog(handler);
    md.start(200);

    var button = this.getElement({type: "engine_button", subtype: "edit"});
    this._controller.click(button);

    
    
    this._controller.sleep(400);
  },

  





  getDtds : function engineManager_getDtds() {
    var dtds = ["chrome://browser/locale/engineManager.dtd"];
    return dtds;
  },

  










  getElement : function engineManager_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      



      case "more_engines":
        elem = new elementslib.ID(this._controller.window.document, "addEngines");
        break;
      case "button":
        elem = new elementslib.Lookup(this._controller.window.document, MANAGER_BUTTONS +
                                      '/{"dlgtype":"' + spec.subtype + '"}');
        break;
      case "engine_button":
        switch(spec.subtype) {
          case "down":
            elem = new elementslib.ID(this._controller.window.document, "dn");
            break;
          case "edit":
            elem = new elementslib.ID(this._controller.window.document, "edit");
            break;
          case "remove":
            elem = new elementslib.ID(this._controller.window.document, "remove");
            break;
          case "up":
            elem = new elementslib.ID(this._controller.window.document, "up");
            break;
        }
        break;
      case "engine_list":
        elem = new elementslib.ID(this._controller.window.document, "engineList");
        break;
      case "suggest":
        elem = new elementslib.ID(this._controller.window.document, "enableSuggest");
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
  },

  


  getMoreSearchEngines : function engineManager_getMoreSearchEngines() {
    var link = this.getElement({type: "more_engines"});
    this._controller.click(link);
  },

  





  moveDownEngine : function engineManager_moveDownEngine(name) {
    this.selectedEngine = name;
    var index = this.selectedIndex;

    var button = this.getElement({type: "engine_button", subtype: "down"});
    this._controller.click(button);

    this._controller.waitForEval("subject.manager.selectedIndex == subject.oldIndex + 1", TIMEOUT, 100,
                                 {manager: this, oldIndex: index});
  },

  





  moveUpEngine : function engineManager_moveUpEngine(name) {
    this.selectedEngine = name;
    var index = this.selectedIndex;

    var button = this.getElement({type: "engine_button", subtype: "up"});
    this._controller.click(button);

    this._controller.waitForEval("subject.manager.selectedIndex == subject.oldIndex - 1", TIMEOUT, 100,
                                 {manager: this, oldIndex: index});
  },

  





  removeEngine : function engineManager_removeEngine(name) {
    this.selectedEngine = name;

    var button = this.getElement({type: "engine_button", subtype: "remove"});
    this._controller.click(button);

    this._controller.waitForEval("subject.manager.selectedEngine != subject.removedEngine", TIMEOUT, 100,
                                 {manager: this, removedEngine: name});
  },

  


  restoreDefaults : function engineManager_restoreDefaults() {
    var button = this.getElement({type: "button", subtype: "extra2"});
    this._controller.click(button);
  }
};







function searchBar(controller)
{
  this._controller = controller;
  this._bss = Cc["@mozilla.org/browser/search-service;1"]
                 .getService(Ci.nsIBrowserSearchService);

  this._ModalDialogAPI = collector.getModule('ModalDialogAPI');
  this._utilsAPI = collector.getModule('UtilsAPI');
}




searchBar.prototype = {
  





  get controller()
  {
    return this._controller;
  },

  


  get engines()
  {
    var engines = [ ];
    var popup = this.getElement({type: "searchBar_dropDownPopup"});

    for (var ii = 0; ii < popup.getNode().childNodes.length; ii++) {
      var entry = popup.getNode().childNodes[ii];
      if (entry.className.indexOf("searchbar-engine") != -1) {
        engines.push({name: entry.id,
                      selected: entry.selected,
                      tooltipText: entry.getAttribute('tooltiptext')
                    });
      }
    }

    return engines;
  },

  


  get enginesDropDownOpen()
  {
    var popup = this.getElement({type: "searchBar_dropDownPopup"});
    return popup.getNode().state != "closed";
  },

  


  set enginesDropDownOpen(newState)
  {
    if (this.enginesDropDownOpen != newState) {
      var button = this.getElement({type: "searchBar_dropDown"});
      this._controller.click(button);

      this._controller.waitForEval("subject.searchBar.enginesDropDownOpen == subject.newState", TIMEOUT, 100,
                                   {searchBar: this, newState: newState });
      this._controller.sleep(0);
    }
  },

  


  get installableEngines()
  {
    var engines = [ ];
    var popup = this.getElement({type: "searchBar_dropDownPopup"});

    for (var ii = 0; ii < popup.getNode().childNodes.length; ii++) {
      var entry = popup.getNode().childNodes[ii];
      if (entry.className.indexOf("addengine-item") != -1) {
        engines.push({name: entry.getAttribute('title'),
                      selected: entry.selected,
                      tooltipText: entry.getAttribute('tooltiptext')
                    });
      }
    }

    return engines;
  },

  





  get selectedEngine()
  {
    
    var state = this.enginesDropDownOpen;
    this.enginesDropDownOpen = true;

    var engine = this.getElement({type: "engine", subtype: "selected", value: "true"});
    this._controller.waitForElement(engine, TIMEOUT);

    this.enginesDropDownOpen = state;

    return engine.getNode().id;
  },

  





  set selectedEngine(name) {
    
    this.enginesDropDownOpen = true;

    var engine = this.getElement({type: "engine", subtype: "id", value: name});
    this._controller.waitThenClick(engine, TIMEOUT);

    
    this._controller.waitForEval("subject.searchBar.enginesDropDownOpen == false", TIMEOUT, 100,
                                 {searchBar: this});

    this._controller.waitForEval("subject.searchBar.selectedEngine == subject.newEngine", TIMEOUT, 100,
                                 {searchBar: this, newEngine: name});
  },

  


  get visibleEngines()
  {
    return this._bss.getVisibleEngines({});
  },

  





  checkSearchResultPage : function searchBar_checkSearchResultPage(searchTerm) {
    
    var targetUrl = this._bss.currentEngine.getSubmission(searchTerm, null).uri;
    var currentUrl = this._controller.tabs.activeTabWindow.document.location.href;

    
    var domainName = targetUrl.host.replace(/.+\.(\w+)\.\w+$/gi, "$1");
    var index = currentUrl.indexOf(domainName);

    this._controller.assertJS("subject.URLContainsDomain == true",
                              {URLContainsDomain: currentUrl.indexOf(domainName) != -1});

    
    this._controller.assertJS("subject.URLContainsText == true",
                              {URLContainsText: currentUrl.toLowerCase().indexOf(searchTerm.toLowerCase()) != -1});
  },

  


  clear : function searchBar_clear()
  {
    var activeElement = this._controller.window.document.activeElement;

    var searchInput = this.getElement({type: "searchBar_input"});
    var cmdKey = this._utilsAPI.getEntity(this.getDtds(), "selectAllCmd.key");
    this._controller.keypress(searchInput, cmdKey, {accelKey: true});
    this._controller.keypress(searchInput, 'VK_DELETE', {});

    if (activeElement)
      activeElement.focus();
  },

  





  focus : function searchBar_focus(event)
  {
    var input = this.getElement({type: "searchBar_input"});

    switch (event.type) {
      case "click":
        this._controller.click(input);
        break;
      case "shortcut":
        if (mozmill.isLinux) {
          var cmdKey = this._utilsAPI.getEntity(this.getDtds(), "searchFocusUnix.commandkey");
        } else {
          var cmdKey = this._utilsAPI.getEntity(this.getDtds(), "searchFocus.commandkey");
        }
        this._controller.keypress(null, cmdKey, {accelKey: true});
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + event.type);
    }

    
    var activeElement = this._controller.window.document.activeElement;
    this._controller.assertJS("subject.isFocused == true",
                              {isFocused: input.getNode() == activeElement});
  },

  





  getDtds : function searchBar_getDtds() {
    var dtds = ["chrome://browser/locale/browser.dtd"];
    return dtds;
  },

  










  getElement : function searchBar_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      



      case "engine":
        
        
        var popup = this.getElement({type: "searchBar_dropDownPopup"}).getNode();
        for (var ii = 0; ii < popup.childNodes.length; ii++) {
          var entry = popup.childNodes[ii];
          if (entry.getAttribute(spec.subtype) == spec.value) {
            elem = new elementslib.Elem(entry);
            break;
          }
        }
        
        
        break;
      case "engine_manager":
        
        
        var popup = this.getElement({type: "searchBar_dropDownPopup"}).getNode();
        for (var ii = popup.childNodes.length - 1; ii >= 0; ii--) {
          var entry = popup.childNodes[ii];
          if (entry.className == "open-engine-manager") {
            elem = new elementslib.Elem(entry);
            break;
          }
        }
        
        
        break;
      case "searchBar":
        elem = new elementslib.Lookup(this._controller.window.document, SEARCH_BAR);
        break;
      case "searchBar_autoCompletePopup":
        elem = new elementslib.Lookup(this._controller.window.document, SEARCH_AUTOCOMPLETE);
        break;
      case "searchBar_contextMenu":
        elem = new elementslib.Lookup(this._controller.window.document, SEARCH_CONTEXT);
        break;
      case "searchBar_dropDown":
        elem = new elementslib.Lookup(this._controller.window.document, SEARCH_DROPDOWN);
        break;
      case "searchBar_dropDownPopup":
        elem = new elementslib.Lookup(this._controller.window.document, SEARCH_POPUP);
        break;
      case "searchBar_goButton":
        elem = new elementslib.Lookup(this._controller.window.document, SEARCH_GO_BUTTON);
        break;
      case "searchBar_input":
        elem = new elementslib.Lookup(this._controller.window.document, SEARCH_INPUT);
        break;
      case "searchBar_suggestions":
        elem = new elementslib.Lookup(this._controller.window.document, SEARCH_AUTOCOMPLETE +
                                      '/anon({"anonid":"tree"})');
         break;
      case "searchBar_textBox":
        elem = new elementslib.Lookup(this._controller.window.document, SEARCH_TEXTBOX);
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
  },

  


  getSuggestions : function(searchTerm) {
    var suggestions = [ ];
    var popup = this.getElement({type: "searchBar_autoCompletePopup"});
    var treeElem = this.getElement({type: "searchBar_suggestions"});

    
    this.type(searchTerm);

    this._controller.waitForEval("subject.popup.state == 'open'", TIMEOUT, 100,
                                 {popup: popup.getNode()});
    this._controller.waitForElement(treeElem, TIMEOUT);

    
    var tree = treeElem.getNode();
    this._controller.waitForEval("subject.tree.view != null", TIMEOUT, 100,
                                 {tree: tree});
    for (var i = 0; i < tree.view.rowCount; i ++) {
      suggestions.push(tree.view.getCellText(i, tree.columns.getColumnAt(0)));
    }

    
    this._controller.keypress(popup, "VK_ESCAPE", {});
    this._controller.waitForEval("subject.popup.state == 'closed'", TIMEOUT, 100,
                                 {popup: popup.getNode()});

    return suggestions;
  },

  





  isEngineInstalled : function searchBar_isEngineInstalled(name)
  {
    var engine = this._bss.getEngineByName(name);
    return (engine != null);
  },

  





  openEngineManager : function searchBar_openEngineManager(handler)
  {
    if (!handler)
      throw new Error(arguments.callee.name + ": No callback handler specified.");

    this.enginesDropDownOpen = true;
    var engineManager = this.getElement({type: "engine_manager"});

    
    md = new this._ModalDialogAPI.modalDialog(handler);
    md.start();

    
    this._controller.sleep(0);
    this._controller.click(engineManager);

    
    this._controller.waitForEval("subject.search.enginesDropDownOpen == false", TIMEOUT, 100,
                                 {search: this});

    
    
    this._controller.sleep(200);
  },

  





  removeEngine : function searchBar_removeEngine(name)
  {
    if (this.isEngineInstalled(name)) {
      var engine = this._bss.getEngineByName(name);
      this._bss.removeEngine(engine);
    }
  },

  


  restoreDefaultEngines : function searchBar_restoreDefaults()
  {
    
    this.openEngineManager(function(controller) {
      var manager = new engineManager(controller);

      
      manager.moveDownEngine(manager.engines[0].name);
      manager.restoreDefaults();
      manager.close(true);
    });

    
    this._bss.restoreDefaultEngines();
    this._bss.currentEngine = this._bss.defaultEngine;

    
    this.clear();
  },

  






  search : function searchBar_search(data)
  {
    var searchBar = this.getElement({type: "searchBar"});
    this.type(data.text);

    switch (data.action) {
      case "returnKey":
        this._controller.keypress(searchBar, 'VK_RETURN', {});
        break;
      case "goButton":
      default:
        this._controller.click(this.getElement({type: "searchBar_goButton"}));
        break;
    }

    this._controller.waitForPageLoad();
    this.checkSearchResultPage(data.text);
  },

  





  type : function searchBar_type(searchTerm) {
    var searchBar = this.getElement({type: "searchBar"});
    this._controller.type(searchBar, searchTerm);
  }
};
