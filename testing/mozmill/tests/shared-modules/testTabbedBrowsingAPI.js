










































var MODULE_NAME = 'TabbedBrowsingAPI';

const gTimeout = 5000;

const tabsBrowser = '/id("main-window")/id("browser")/id("appcontent")/id("content")';
const tabsStrip = tabsBrowser + '/anon({"anonid":"tabbox"})/anon({"anonid":"strip"})';
const tabsContainer = tabsStrip + '/anon({"anonid":"tabcontainer"})/anon({"class":"tabs-stack"})/{"class":"tabs-container"}';
const tabsArrowScrollbox = tabsContainer + '/anon({"anonid":"arrowscrollbox"})';








function closeAllTabs(controller)
{
  var browser = new tabBrowser(controller);
  browser.closeAllTabs();
}







function tabBrowser(controller)
{
  this._controller = controller;
  this._tabs = this.getElement({type: "tabs"});
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
    while (this._controller.tabs.length > 1)
      this.closeTab({type: "menu"});

    this._controller.open("about:blank");
    this._controller.waitForPageLoad();
  },

  







  closeTab : function tabBrowser_closeTab(event) {
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
        this._controller.keypress(null, "w", {accelKey: true});
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown event - " + event.type);
    }
  },

  










  getElement : function tabBrowser_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      



      case "tabs":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      tabsStrip + '/anon({"anonid":"tabcontainer"})');
        break;
      case "tabs_allTabsButton":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      tabsContainer + '/{"pack":"end"}/anon({"anonid":"alltabs-button"})');
        break;
      case "tabs_allTabsPopup":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      tabsContainer + '/{"pack":"end"}/anon({"anonid":"alltabs-button"})' +
                                      '/anon({"anonid":"alltabs-popup"})');
        break;
      case "tabs_animateBox":
        elem = new elementslib.Lookup(this._controller.window.document, tabsContainer +
                                      '/{"pack":"end"}/anon({"anonid":"alltabs-box-animate"})');
        break;
      case "tabs_container":
        elem = new elementslib.Lookup(this._controller.window.document, tabsContainer);
        break;
      case "tabs_newTabButton":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      tabsArrowScrollbox + '/anon({"class":"tabs-newtab-button"})');
        break;
      case "tabs_scrollButton":
        elem = new elementslib.Lookup(controller.window.document,
                                      tabsArrowScrollbox +
                                      '/anon({"anonid":"scrollbutton-' + spec.subtype + '"})');
        break;
      case "tabs_strip":
        elem = new elementslib.Lookup(this._controller.window.document, tabsStrip);
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
        elem = new elementslib.Lookup(this._controller.window.document, tabsBrowser +
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

  






  openTab : function tabBrowser_openTab(event) {
    switch (event.type) {
      case "menu":
        var menuitem = new elementslib.Elem(this._controller.menus['file-menu'].menu_newNavigatorTab);
        this._controller.click(menuitem);
        break;
      case "shortcut":
        this._controller.keypress(null, "t", {accelKey: true});
        break;
      case "newTabButton":
        var newTabButton = this.getElement({type: "tabs_newTabButton"});
        this._controller.click(newTabButton);
        break;
      case "tabStrip":
        var tabStrip = this.getElement({type: "tabs_strip"});

        
        this._controller.click(tabStrip, tabStrip.getNode().clientWidth - 100, 3);

        
        this._controller.doubleClick(tabStrip, tabStrip.getNode().clientWidth - 100, 3);
        break;
    }
  }
}
