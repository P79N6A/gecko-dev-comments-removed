









































var MODULE_NAME = 'AddonsAPI';

const RELATIVE_ROOT = '.';
const MODULE_REQUIRES = ['PrefsAPI', 'UtilsAPI'];

const gTimeout = 5000;


const AM_TOPBAR       = '/id("extensionsManager")/id("topBar")';
const AM_DECK         = '/id("extensionsManager")/id("addonsMsg")';
const AM_SELECTOR     = AM_TOPBAR + '/{"flex":"1"}/{"class":"viewGroupWrapper"}/id("viewGroup")';
const AM_NOTIFICATION = AM_DECK + '/{"type":"warning"}';
const AM_SEARCHFIELD  = AM_DECK + '/id("extensionsBox")/id("searchPanel")/id("searchfield")';
const AM_SEARCHBUTTON = AM_SEARCHFIELD + '/anon({"class":"textbox-input-box"})/anon({"anonid":"search-icons"})';
const AM_SEARCHINPUT  = AM_SEARCHFIELD + '/anon({"class":"textbox-input-box"})/anon({"anonid":"input"})';
const AM_LISTBOX      = AM_DECK + '/id("extensionsBox")/[1]/id("extensionsView")';
const AM_LISTBOX_BTN  = '/anon({"flex":"1"})/{"class":"addonTextBox"}/{"flex":"1"}';



const AMO_PREFERENCES = [
  {name: "extensions.getAddons.browseAddons", old: "addons.mozilla.org", new: "preview.addons.mozilla.org"},
  {name: "extensions.getAddons.recommended.browseURL", old: "addons.mozilla.org", new: "preview.addons.mozilla.org"},
  {name: "extensions.getAddons.recommended.url", old: "services.addons.mozilla.org", new: "preview.addons.mozilla.org"},
  {name: "extensions.getAddons.search.browseURL", old: "addons.mozilla.org", new: "preview.addons.mozilla.org"},
  {name: "extensions.getAddons.search.url", old: "services.addons.mozilla.org", new: "preview.addons.mozilla.org"},
  {name: "extensions.getMoreThemesURL", old: "addons.mozilla.org", new: "preview.addons.mozilla.org"}
];




function addonsManager()
{
  this._controller = null;
  this._utilsAPI = collector.getModule('UtilsAPI');
}




addonsManager.prototype = {
  





  get controller() {
    return this._controller;
  },

  





  get paneId() {
    var selected = this.getElement({type: "selector_button", subtype: "selected", value: "true"});

    return /\w+/.exec(selected.getNode().id);
  },

  





  set paneId(id) {
    if (this.paneId == id)
      return;

    var button = this.getElement({type: "selector_button", subtype: "id", value: id + "-view"});
    this._controller.waitThenClick(button, gTimeout);

    
    this._controller.waitForEval("subject.window.paneId == subject.expectedPaneId", gTimeout, 100,
                                 {window: this, expectedPaneId: id});
  },

  


  clearSearchField : function addonsManager_clearSearchField() {
    this.paneId = 'search';

    var searchInput = this.getElement({type: "search_fieldInput"});
    var cmdKey = UtilsAPI.getEntity(this.getDtds(), "selectAllCmd.key");
    this._controller.keypress(searchInput, cmdKey, {accelKey: true});
    this._controller.keypress(searchInput, 'VK_DELETE', {});
  },

  





  close : function addonsManager_close(force) {
    var windowCount = mozmill.utils.getWindows().length;

    if (this._controller) {
      
      if (force) {
        this._controller.window.close();
      } else {
        var cmdKey = UtilsAPI.getEntity(this.getDtds(), "closeCmd.key");
        this._controller.keypress(null, cmdKey, {accelKey: true});
      }

      this._controller.waitForEval("subject.getWindows().length == " + (windowCount - 1),
                                   gTimeout, 100, mozmill.utils);
      this._controller = null;
    }
  },

  





  getDtds : function downloadManager_getDtds() {
    var dtds = ["chrome://mozapps/locale/extensions/extensions.dtd",
                "chrome://browser/locale/browser.dtd"];
    return dtds;
  },

  










  getElement : function addonsManager_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      case "button_continue":
        elem = new elementslib.ID(this._controller.window.document, "continueDialogButton");
        break;
      case "button_enableAll":
        elem = new elementslib.ID(this._controller.window.document, "enableAllButton");
        break;
      case "button_findUpdates":
        elem = new elementslib.ID(this._controller.window.document, "checkUpdatesAllButton");
        break;
      case "button_hideInformation":
        elem = new elementslib.ID(this._controller.window.document, "hideUpdateInfoButton");
        break;
      case "button_installFile":
        elem = new elementslib.ID(this._controller.window.document, "installFileButton");
        break;
      case "button_installUpdates":
        elem = new elementslib.ID(this._controller.window.document, "installUpdatesAllButton");
        break;
      case "button_showInformation":
        elem = new elementslib.ID(this._controller.window.document, "showUpdateInfoButton");
        break;
      case "button_skip":
        elem = new elementslib.ID(this._controller.window.document, "skipDialogButton");
        break;
      case "link_browseAddons":
        elem = new elementslib.ID(this._controller.window.document, "browseAddons");
        break;
      case "link_getMore":
        elem = new elementslib.ID(this._controller.window.document, "getMore");
        break;
      case "listbox":
        elem = new elementslib.Lookup(this._controller.window.document, AM_LISTBOX);
        break;
      case "listbox_button":
        
        if (this.paneId == "search") {
          elem = new elementslib.Lookup(this._controller.window.document, spec.value.expression +
                                        AM_LISTBOX_BTN + '/{"flex":"1"}/anon({"anonid":"selectedButtons"})' +
                                        '/{"command":"cmd_' + spec.subtype + '"}');
        } else {
          elem = new elementslib.Lookup(this._controller.window.document, spec.value.expression +
                                        AM_LISTBOX_BTN + '/{"command":"cmd_' + spec.subtype + '"}');
        }
        break;
      case "listbox_element":
        elem = new elementslib.Lookup(this._controller.window.document, AM_LISTBOX +
                                      '/{"' + spec.subtype + '":"' + spec.value + '"}');
        break;
      case "notificationBar":
        elem = new elementslib.Lookup(this._controller.window.document, AM_NOTIFICATION);
        break;
      case "notificationBar_buttonRestart":
        elem = new elementslib.Lookup(this._controller.window.document, AM_DECK +
                                      '/{"type":"warning"}');
        break;
      case "search_field":
        elem = new elementslib.Lookup(this._controller.window.document, AM_SEARCHFIELD);
        break;
      case "search_fieldButton":
        elem = new elementslib.Lookup(this._controller.window.document, AM_SEARCHBUTTON);
        break;
      case "search_fieldInput":
        elem = new elementslib.Lookup(this._controller.window.document, AM_SEARCHINPUT);
        break;
      case "search_status":
        elem = new elementslib.ID(this._controller.window.document,
                                  'urn:mozilla:addons:search:status:' + spec.subtype);
        break;
      case "search_statusLabel":
        elem = new elementslib.Elem(spec.value.getNode().boxObject.firstChild);
        break;
      case "selector":
        elem = new elementslib.Lookup(this._controller.window.document, AM_SELECTOR);
        break;
      case "selector_button":
        elem = new elementslib.Lookup(this._controller.window.document, AM_SELECTOR +
                                      '/{"' + spec.subtype + '":"' + spec.value + '"}');
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
  },

  









  getListboxItem : function addonsManager_getListItem(name, value) {
    return this.getElement({type: "listbox_element", subtype: name, value: value});
  },

  





  getPane : function addonsManager_getPane(id) {
    return this.getElement({type: "selector_button", subtype: "id", value: id + "-view"});
  },

  









  getPluginState : function addonsManager_getPluginState(node, value) {
    
    this.paneId = "plugins";
    
    var plugin = this.getListboxItem(node, value);
    var status = plugin.getNode().getAttribute('isDisabled') == 'false';

    return status;
  },

  





  open : function addonsManager_open(controller) {
    controller.click(new elementslib.Elem(controller.menus["tools-menu"].menu_openAddons));
    this.waitForOpened(controller);
  },

  





  search : function addonsManager_search(searchTerm) {
    
    this.paneId = "search";

    var searchField = this.getElement({type: "search_field"});

    this.clearSearchField();
    this._controller.waitForElement(searchField, gTimeout);
    this._controller.type(searchField, searchTerm);
    this._controller.keypress(searchField, "VK_RETURN", {});
  },

  









  setPluginState : function addonsManager_setPluginState(node, value, enable) {
    
    if (this.getPluginState(node, value) == enable)
      return;

    
    var plugin = this.getListboxItem(node, value);
    this._controller.click(plugin);

    
    var subtype = enable ? "enable" : "disable";
    var button = this.getElement({type: "listbox_button", subtype: subtype, value: plugin});

    this._controller.waitThenClick(button, gTimeout);
    this._controller.waitForEval("subject.plugin.getPluginState(subject.node, subject.value) == subject.state", gTimeout, 100,
                                 {plugin: this, node: node, value: value, state: enable});
  },

  





  waitForOpened : function addonsManager_waitforOpened(controller) {
    this._controller = this._utilsAPI.handleWindow("type", "Extension:Manager",
                                                   null, true);
  }
};




function useAmoPreviewUrls() {
  var prefSrv = collector.getModule('PrefsAPI').preferences;

  for each (preference in AMO_PREFERENCES) {
    var pref = prefSrv.getPref(preference.name, "");
    prefSrv.setPref(preference.name, pref.replace(preference.old, preference.new));
  }
}




function resetAmoPreviewUrls() {
  var prefSrv = collector.getModule('PrefsAPI').preferences;

  for each (preference in AMO_PREFERENCES) {
    prefSrv.clearUserPref(preference.name);
  }
}
