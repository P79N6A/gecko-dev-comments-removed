










































const MODULE_NAME = 'TabbedBrowsingAPI';


const RELATIVE_ROOT = '.';
const MODULE_REQUIRES = ['PrefsAPI', 'UtilsAPI'];

const TIMEOUT = 5000;

const PREF_TABS_ANIMATE = "browser.tabs.animate";

const TABS_VIEW = '/id("main-window")/id("tab-view-deck")/{"flex":"1"}';
const TABS_BROWSER = TABS_VIEW + '/id("browser")/id("appcontent")/id("content")';
const TABS_TOOLBAR = TABS_VIEW + '/id("navigator-toolbox")/id("TabsToolbar")';
const TABS_TABS = TABS_TOOLBAR + '/id("tabbrowser-tabs")';
const TABS_ARROW_SCROLLBOX = TABS_TABS + '/anon({"anonid":"arrowscrollbox"})';
const TABS_STRIP = TABS_ARROW_SCROLLBOX + '/anon({"anonid":"scrollbox"})/anon({"flex":"1"})';







function closeAllTabs(controller)
{
  var browser = new tabBrowser(controller);
  browser.closeAllTabs();
}







function tabBrowser(controller)
{
  this._controller = controller;
  this._tabs = this.getElement({type: "tabs"});

  this._UtilsAPI = collector.getModule('UtilsAPI');
  this._PrefsAPI = collector.getModule('PrefsAPI');
}




tabBrowser.prototype = {
  





  get controller() {
    return this._controller;
  },

  





  get length() {
    return this._tabs.getNode().itemCount;
  },

  





  get selectedIndex() {
    return this._tabs.getNode().selectedIndex;
  },

  





  set selectedIndex(index) {
    this._controller.click(this.getTab(index), 2, 2);
  },

  


  closeAllTabs : function tabBrowser_closeAllTabs()
  {
    while (this._controller.tabs.length > 1) {
      this.closeTab({type: "menu"});
    }

    this._controller.open("about:blank");
    this._controller.waitForPageLoad();
  },

  







  closeTab : function tabBrowser_closeTab(event) {
    
    this._PrefsAPI.preferences.setPref(PREF_TABS_ANIMATE, false);

    
    var self = { closed: false };
    function checkTabClosed() { self.closed = true; }
    this._controller.window.addEventListener("TabClose", checkTabClosed, false);

    switch (event.type) {
      case "closeButton":
        var button = this.getElement({type: "tabs_tabCloseButton",
                                     subtype: "tab", value: this.getTab()});
        controller.click(button);
        break;
      case "menu":
        var menuitem = new elementslib.Elem(this._controller.menus['file-menu'].menu_close);
        this._controller.click(menuitem);
        break;
      case "middleClick":
        var tab = this.getTab(event.index);
        this._controller.middleClick(tab);
        break;
      case "shortcut":
        var cmdKey = this._UtilsAPI.getEntity(this.getDtds(), "closeCmd.key");
        this._controller.keypress(null, cmdKey, {accelKey: true});
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown event - " + event.type);
    }

    try {
      this._controller.waitForEval("subject.tab.closed == true", TIMEOUT, 100,
                                   {tab: self});
    } finally {
      this._controller.window.removeEventListener("TabClose", checkTabClosed, false);
      this._PrefsAPI.preferences.clearUserPref(PREF_TABS_ANIMATE);
    }
  },

  





  getDtds : function tabBrowser_getDtds() {
    var dtds = ["chrome://browser/locale/browser.dtd",
                "chrome://browser/locale/tabbrowser.dtd",
                "chrome://global/locale/global.dtd"];
    return dtds;
  },

  










  getElement : function tabBrowser_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      



      case "tabs":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      TABS_TABS);
        break;
      case "tabs_allTabsButton":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      TABS_TOOLBAR + '/id("alltabs-button")');
        break;
      case "tabs_allTabsPopup":
        elem = new elementslib.Lookup(this._controller.window.document, TABS_TOOLBAR +
                                      '/id("alltabs-button")/id("alltabs-popup")');
        break;
      case "tabs_newTabButton":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      TABS_ARROW_SCROLLBOX + '/anon({"class":"tabs-newtab-button"})');
        break;
      case "tabs_scrollButton":
        elem = new elementslib.Lookup(controller.window.document,
                                      TABS_ARROW_SCROLLBOX +
                                      '/anon({"anonid":"scrollbutton-' + spec.subtype + '"})');
        break;
      case "tabs_strip":
        elem = new elementslib.Lookup(this._controller.window.document, TABS_STRIP);
        break;
      case "tabs_tab":
        switch (spec.subtype) {
          case "index":
            elem = new elementslib.Elem(this._tabs.getNode().getItemAtIndex(spec.value));
            break;
        }
        break;
      case "tabs_tabCloseButton":
        elem = new elementslib.Elem(spec.value.getNode().boxObject.lastChild);
        break;
      case "tabs_tabFavicon":
        elem = new elementslib.Elem(spec.value.getNode().boxObject.firstChild);
        break;
      case "tabs_tabPanel":
        var panelId = spec.value.getNode().getAttribute("linkedpanel");
        elem = new elementslib.Lookup(this._controller.window.document, TABS_BROWSER +
                                      '/anon({"anonid":"tabbox"})/anon({"anonid":"panelcontainer"})' +
                                      '/{"id":"' + panelId + '"}');
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
  },

  







  getTab : function tabBrowser_getTab(index) {
    if (index === undefined)
      index = this.selectedIndex;

    return this.getElement({type: "tabs_tab", subtype: "index", value: index});
  },

  









  getTabPanelElement : function tabBrowser_getTabPanelElement(tabIndex, elemString)
  {
    var index = tabIndex ? tabIndex : this.selectedIndex;
    var elemStr = elemString ? elemString : "";

    
    var panel = this.getElement({type: "tabs_tabPanel", subtype: "tab", value: this.getTab(index)});
    var elem = new elementslib.Lookup(controller.window.document, panel.expression + elemStr);

    return elem;
  },

  






  openInNewTab : function tabBrowser_openInNewTab(event) {
    
    this._PrefsAPI.preferences.setPref(PREF_TABS_ANIMATE, false);

    
    var self = { opened: false };
    function checkTabOpened() { self.opened = true; }
    this._controller.window.addEventListener("TabOpen", checkTabOpened, false);

    switch (event.type) {
      case "contextMenu":
        var contextMenuItem = new elementslib.ID(this._controller.window.document,
                                                 "context-openlinkintab");
        this._controller.rightClick(event.target);
        this._controller.click(contextMenuItem);
        this._UtilsAPI.closeContentAreaContextMenu(this._controller);
        break;
      case "middleClick":
        this._controller.middleClick(event.target);
        break;
    }

    try {
      this._controller.waitForEval("subject.tab.opened == true", TIMEOUT, 100,
                                   {tab: self});
    } finally {
      this._controller.window.removeEventListener("TabOpen", checkTabOpened, false);
      this._PrefsAPI.preferences.clearUserPref(PREF_TABS_ANIMATE);
    }
  },

  






  openTab : function tabBrowser_openTab(event) {
    
    this._PrefsAPI.preferences.setPref(PREF_TABS_ANIMATE, false);

    
    var self = { opened: false };
    function checkTabOpened() { self.opened = true; }
    this._controller.window.addEventListener("TabOpen", checkTabOpened, false);

    switch (event.type) {
      case "menu":
        var menuitem = new elementslib.Elem(this._controller.menus['file-menu'].menu_newNavigatorTab);
        this._controller.click(menuitem);
        break;
      case "shortcut":
        var cmdKey = this._UtilsAPI.getEntity(this.getDtds(), "tabCmd.commandkey");
        this._controller.keypress(null, cmdKey, {accelKey: true});
        break;
      case "newTabButton":
        var newTabButton = this.getElement({type: "tabs_newTabButton"});
        this._controller.click(newTabButton);
        break;
      case "tabStrip":
        var tabStrip = this.getElement({type: "tabs_strip"});
        
        
        if (this._UtilsAPI.getEntity(this.getDtds(), "locale.dir") == "rtl") {
          
          this._controller.click(tabStrip, 100, 3);
          
          this._controller.doubleClick(tabStrip, 100, 3);
        } else {
          
          this._controller.click(tabStrip, tabStrip.getNode().clientWidth - 100, 3);
          
          this._controller.doubleClick(tabStrip, tabStrip.getNode().clientWidth - 100, 3);
        }
        break;
    }

    try {
      this._controller.waitForEval("subject.tab.opened == true", TIMEOUT, 100,
                                   {tab: self});
    } finally {
      this._controller.window.removeEventListener("TabOpen", checkTabOpened, false);
      this._PrefsAPI.preferences.clearUserPref(PREF_TABS_ANIMATE);
    }
  }
}
