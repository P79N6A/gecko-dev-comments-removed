










































const MODULE_NAME = 'ToolbarAPI';

const RELATIVE_ROOT = '.';
const MODULE_REQUIRES = ['UtilsAPI'];

const TIMEOUT = 5000;

const AUTOCOMPLETE_POPUP = '/id("main-window")/id("mainPopupSet")/id("PopupAutoCompleteRichResult")';
const NOTIFICATION_POPUP = '/id("main-window")/id("mainPopupSet")/id("notification-popup")';
const URLBAR_CONTAINER = '/id("main-window")/id("tab-view-deck")/{"flex":"1"}' +
                         '/id("navigator-toolbox")/id("nav-bar")/id("urlbar-container")';
const URLBAR_INPUTBOX = URLBAR_CONTAINER + '/id("urlbar")/anon({"anonid":"stack"})' + 
                                           '/anon({"anonid":"textbox-container"})' + 
                                           '/anon({"anonid":"textbox-input-box"})';
const CONTEXT_MENU = URLBAR_INPUTBOX + '/anon({"anonid":"input-box-contextmenu"})';







function autoCompleteResults(controller)
{
  this._controller = controller;
  this.popup = this.getElement({type: "popup"});
  this._results = this.getElement({type: "results"});
}




autoCompleteResults.prototype = {
  





  get allResults() {
    var results = [];
    for (ii = 0; ii < this.length; ii++) {
      results.push(this.getResult(ii));
    }
    return results;
  },

  





  get controller() {
    return this._controller;
  },

  





  get isOpened() {
    return (this.popup.getNode().state == 'open');
  },

  





  get length() {
    return this._results.getNode().itemCount;
  },

  





  get selectedIndex() {
    return this._results.getNode().selectedIndex;
  },

  





  get visibleResults() {
    var results = [];
    for (ii = 0; ii < this.length; ii++) {
      var result = this.getResult(ii);
      if (!result.getNode().hasAttribute("collapsed"))
        results.push(result);
    }
    return results;
  },

  










  getUnderlinedText : function autoCompleteResults_getUnderlinedText(result, type) {
    this._controller.assertJS("subject.resultNode != null",
                              {resultNode: result.getNode()});

    
    var description = null;
    switch (type) {
      case "title":
        description = result.getNode().boxObject.firstChild.childNodes[1].childNodes[0];
        break;
      case "url":
        description = result.getNode().boxObject.lastChild.childNodes[2].childNodes[0];
        break;
      default:
        throw new Error(arguments.callee.name + ": Type unknown - " + type);
    }

    let values = [ ];
    for each (node in description.childNodes) {
      if (node.nodeName == 'span') {
        
        values.push(node.innerHTML);
      }
    }

    return values;
  },

  





  getDtds : function autoCompleteResults_getDtds() {
    return null;
  },

  










  getElement : function autoCompleteResults_getElement(spec) {
    var elem = null;

    switch (spec.type) {
      



      case "popup":
        elem = new elementslib.Lookup(this._controller.window.document, AUTOCOMPLETE_POPUP);
        break;
      case "results":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      AUTOCOMPLETE_POPUP + '/anon({"anonid":"richlistbox"})');
        break;
      case "result":
        elem = new elementslib.Elem(this._results.getNode().getItemAtIndex(spec.value));
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
  },

  







  getResult : function autoCompleteResults_getResult(index) {
    return this.getElement({type: "result", value: index});
  }
}







function locationBar(controller)
{
  this._controller = controller;
  this._autoCompleteResults = new autoCompleteResults(controller);
  this._utilsApi = collector.getModule('UtilsAPI');
}




locationBar.prototype = {
  





  get autoCompleteResults() {
    return this._autoCompleteResults;
  },

  





  get controller() {
    return this._controller;
  },

  





  get urlbar() {
    return this.getElement({type: "urlbar"});
  },

  





  get value() {
    return this.urlbar.value;
  },

  


  clear : function locationBar_clear() {
    this.focus({type: "shortcut"});
    this._controller.keypress(this.urlbar, "VK_DELETE", {});
    this._controller.waitForEval("subject.value == ''",
                                 TIMEOUT, 100, this.urlbar.getNode());
  },

  


  closeContextMenu : function locationBar_closeContextMenu() {
    var menu = this.getElement({type: "contextMenu"});
    this._controller.keypress(menu, "VK_ESCAPE", {});
  },

  





  contains : function locationBar_contains(text) {
    return this.urlbar.getNode().value.indexOf(text) != -1;
  },

  





  focus : function locationBar_focus(event) {
    switch (event.type) {
      case "click":
        this._controller.click(this.urlbar);
        break;
      case "shortcut":
        var cmdKey = this._utilsApi.getEntity(this.getDtds(), "openCmd.commandkey");
        this._controller.keypress(null, cmdKey, {accelKey: true});
        break;
      default:
        throw new Error(arguments.callee.name + ": Unkown event type - " + event.type);
    }

    
    this._controller.waitForEval("subject.getAttribute('focused') == 'true'",
                                 TIMEOUT, 100, this.urlbar.getNode());
  },

  





  getDtds : function locationBar_getDtds() {
    var dtds = ["chrome://branding/locale/brand.dtd",
                "chrome://browser/locale/browser.dtd"];
    return dtds;
  },

  










  getElement : function locationBar_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      



      case "contextMenu":
        elem = new elementslib.Lookup(this._controller.window.document, CONTEXT_MENU);
        break;
      case "contextMenu_entry":
        elem = new elementslib.Lookup(this._controller.window.document, CONTEXT_MENU +
                                      '/{"cmd":"cmd_' + spec.subtype + '"}');
        break;
      case "favicon":
        elem = new elementslib.ID(this._controller.window.document, "page-proxy-favicon");
        break;
      case "feedButton":
        elem = new elementslib.ID(this._controller.window.document, "feed-button");
        break;
      case "goButton":
        elem = new elementslib.ID(this._controller.window.document, "urlbar-go-button");
        break;
      case "historyDropMarker":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      URLBAR_CONTAINER + '/id("urlbar")/anon({"anonid":"historydropmarker"})');
        break;
      case "identityBox":
        elem = new elementslib.ID(this._controller.window.document, "identity-box");
        break;
      case "notification_element":
        elem = new elementslib.Lookup(this._controller.window.document, NOTIFICATION_POPUP +
                                      spec.subtype);
        break;
      case "starButton":
        elem = new elementslib.ID(this._controller.window.document, "star-button");
        break;
      case "urlbar":
        elem = new elementslib.ID(this._controller.window.document, "urlbar");
        break;
      case "urlbar_input":
        elem = new elementslib.Lookup(this._controller.window.document, URLBAR_INPUTBOX +
                                      '/anon({"anonid":"input"})');
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
  },

  











  getNotificationElement : function locationBar_getNotificationElement(aType, aLookupString)
  {
    var lookup = '/id("' + aType + '")';
    lookup = aLookupString ? lookup + aLookupString : lookup;

    
    return this.getElement({type: "notification_element", subtype: lookup});
  },

  





  loadURL : function locationBar_loadURL(url) {
    this.focus({type: "shortcut"});
    this.type(url);
    this._controller.keypress(this.urlbar, "VK_RETURN", {});
  },

  


  toggleAutocompletePopup : function locationBar_toggleAutocompletePopup() {
    var dropdown = this.getElement({type: "historyDropMarker"});
    var stateOpen = this.autoCompleteResults.isOpened;

    this._controller.click(dropdown);
    this._controller.waitForEval("subject.isOpened == " + stateOpen,
                                 TIMEOUT, 100, this.autoCompleteResults);
  },

  





  type : function locationBar_type(text) {
    this._controller.type(this.urlbar, text);
    this.contains(text);
  }
}
