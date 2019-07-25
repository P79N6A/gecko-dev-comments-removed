










































var MODULE_NAME = 'ToolbarAPI';

const gTimeout = 5000;

const autocompletePopup = '/id("main-window")/id("mainPopupSet")/id("PopupAutoCompleteRichResult")';
const urlbarContainer = '/id("main-window")/id("navigator-toolbox")/id("nav-bar")/id("urlbar-container")';







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
        description = result.getNode().boxObject.lastChild.childNodes[1].childNodes[0];
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

  










  getElement : function autoCompleteResults_getElement(spec) {
    var elem = null;

    switch (spec.type) {
      



      case "popup":
        elem = new elementslib.Lookup(this._controller.window.document, autocompletePopup);
        break;
      case "results":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      autocompletePopup + '/anon({"anonid":"richlistbox"})');
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
                                 gTimeout, 100, this.urlbar.getNode());
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
        this._controller.keypress(null, "l", {accelKey: true});
        break;
      default:
        throw new Error(arguments.callee.name + ": Unkown event type - " + event.type);
    }

    
    this._controller.waitForEval("subject.getAttribute('focused') == 'true'",
                                 gTimeout, 100, this.urlbar.getNode());
  },

  










  getElement : function locationBar_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      



      case "favicon":
        elem = new elementslib.ID(this._controller.window.document, "page-proxy-favicon");
        break;
      case "feedButton":
        elem = new elementslib.ID(this._controller.window.document, "feed-button");
        break;
      case "goButton":
        elem = new elementslib.ID(this._controller.window.document, "go-button");
        break;
      case "historyDropMarker":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      urlbarContainer + '/id("urlbar")/anon({"anonid":"historydropmarker"})');
        break;
      case "identityBox":
        elem = new elementslib.ID(this._controller.window.document, "identity-box");
        break;
      case "starButton":
        elem = new elementslib.ID(this._controller.window.document, "star-button");
        break;
      case "urlbar":
        elem = new elementslib.ID(this._controller.window.document, "urlbar");
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
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
                                 gTimeout, 100, this.autoCompleteResults);
  },

  





  type : function locationBar_type(text) {
    this._controller.type(this.urlbar, text);
    this.contains(text);
  }
}
