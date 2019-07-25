










































var MODULE_NAME = 'SessionStoreAPI';


var RELATIVE_ROOT = '.';
var MODULE_REQUIRES = ['WidgetsAPI'];


var sessionStoreService = Cc["@mozilla.org/browser/sessionstore;1"]
                             .getService(Ci.nsISessionStore);

const gTimeout = 5000;







function aboutSessionRestore(controller)
{
  this._controller = controller;

  this._WidgetsAPI = collector.getModule('WidgetsAPI');
}




aboutSessionRestore.prototype = {
  





  get controller() {
    return this._controller;
  },

  





  get tabList() {
    return this.getElement({type: "tabList"});
  },

  










  getElement : function aboutSessionRestore_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      case "button_newSession":
        elem = new elementslib.ID(this._controller.tabs.activeTab, "errorCancel");
        break;
      case "button_restoreSession":
        elem = new elementslib.ID(this._controller.tabs.activeTab, "errorTryAgain");
        break;
      case "error_longDesc":
        elem = new elementslib.ID(this._controller.tabs.activeTab, "errorLongDesc");
        break;
      case "error_pageContainer":
        elem = new elementslib.ID(this._controller.tabs.activeTab, "errorPageContainer");
        break;
      case "error_shortDesc":
        elem = new elementslib.ID(this._controller.tabs.activeTab, "errorShortDescText");
        break;
      case "error_title":
        elem = new elementslib.ID(this._controller.tabs.activeTab, "errorTitleText");
        break;
      case "tabList":
        elem = new elementslib.ID(this._controller.window.document, "tabList");
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
  },

  








  getRestoreState : function aboutSessionRestore_getRestoreState(element) {
    var tree = this.tabList.getNode();

    return tree.view.getCellValue(element.listIndex, tree.columns.getColumnAt(0));
  },

  







  getTabs : function aboutSessionRestore_getTabs(window) {
    var tabs = [ ];
    var tree = this.tabList.getNode();

    
    var ii = window.listIndex + 1;
    while (ii < tree.view.rowCount && !tree.view.isContainer(ii)) {
      tabs.push({
                 index: tabs.length,
                 listIndex : ii,
                 restore: tree.view.getCellValue(ii, tree.columns.getColumnAt(0)),
                 title: tree.view.getCellText(ii, tree.columns.getColumnAt(2))
                });
      ii++;
    }

    return tabs;
  },

  





  getWindows : function aboutSessionRestore_getWindows() {
    var windows = [ ];
    var tree = this.tabList.getNode();

    for (var ii = 0; ii < tree.view.rowCount; ii++) {
      if (tree.view.isContainer(ii)) {
        windows.push({
                      index: windows.length,
                      listIndex : ii,
                      open: tree.view.isContainerOpen(ii),
                      restore: tree.view.getCellValue(ii, tree.columns.getColumnAt(0)),
                      title: tree.view.getCellText(ii, tree.columns.getColumnAt(2))
                     });
      }
    }

    return windows;
  },

  





  toggleRestoreState : function aboutSessionRestore_toggleRestoreState(element) {
    var state = this.getRestoreState(element);

    this._WidgetsAPI.clickTreeCell(this._controller, this.tabList, element.listIndex, 0, {});
    this._controller.sleep(0);

    this._controller.assertJS("subject.newState != subject.oldState",
                              {newState : this.getRestoreState(element), oldState : state});
  }
}









function undoClosedTab(controller, event)
{
  var count = sessionStoreService.getClosedTabCount(controller.window);

  switch (event.type) {
    case "menu":
      throw new Error("Menu gets build dynamically and cannot be accessed.");
      break;
    case "shortcut":
      controller.keypress(null, "t", {accelKey: true, shiftKey: true});
      break;
  }

  if (count > 0)
    controller.assertJS("subject.newTabCount < subject.oldTabCount",
                        {
                         newTabCount : sessionStoreService.getClosedTabCount(controller.window),
                         oldTabCount : count
                        });
}









function undoClosedWindow(controller, event)
{
  var count = sessionStoreService.getClosedWindowCount(controller.window);

  switch (event.type) {
    case "menu":
      throw new Error("Menu gets build dynamically and cannot be accessed.");
      break;
    case "shortcut":
      controller.keypress(null, "n", {accelKey: true, shiftKey: true});
      break;
  }

  if (count > 0)
    controller.assertJS("subject.newWindowCount < subject.oldWindowCount",
                        {
                         newWindowCount : sessionStoreService.getClosedWindowCount(controller.window),
                         oldWindowCount : count
                        });
}
