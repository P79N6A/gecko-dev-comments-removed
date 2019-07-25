












































var MODULE_NAME = 'PrefsAPI';

const RELATIVE_ROOT = '.'
const MODULE_REQUIRES = ['ModalDialogAPI'];

const gTimeout = 5000;


const PREF_DIALOG_BUTTONS  = '/{"type":"prefwindow"}/anon({"anonid":"dlg-buttons"})';
const PREF_DIALOG_DECK     = '/{"type":"prefwindow"}/anon({"class":"paneDeckContainer"})/anon({"anonid":"paneDeck"})';
const PREF_DIALOG_SELECTOR = '/{"type":"prefwindow"}/anon({"orient":"vertical"})/anon({"anonid":"selector"})';







function preferencesDialog(controller)
{
  this._controller = controller;
}




preferencesDialog.prototype = {
  





  get controller() {
    return this._controller;
  },

  





  get selectedPane() {
    return this.getElement({type: "deck_pane"});
  },

  


  get paneId() {
    
    var selector = this.getElement({type: "selector"});

    this._controller.waitForEval("subject.selector.getAttribute('pane') == subject.dlg.selectedPane.getNode().id", gTimeout, 100,
                                 {selector: selector.getNode().selectedItem, dlg: this});

    return this.selectedPane.getNode().id;
  },

  




  set paneId(id) {
    var button = this.getElement({type: "selector_button", value: id});
    this._controller.waitThenClick(button, gTimeout);

    
    var selector = this.getElement({type: "selector"});
    this._controller.waitForEval("subject.selector.getAttribute('pane') == subject.newPane", gTimeout, 100,
                                 {selector: selector.getNode().selectedItem, newPane: id});
    return this.paneId;
  },

  








  close : function preferencesDialog_close(saveChanges) {
    saveChanges = (saveChanges == undefined) ? false : saveChanges;

    if (mozmill.isWindows) {
      var button = this.getElement({type: "button", subtype: (saveChanges ? "accept" : "cancel")});
      this._controller.click(button);
    } else {
      this._controller.keypress(null, 'VK_ESCAPE', {});
    }
  },

  










  getElement : function aboutSessionRestore_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      case "button":
        elem = new elementslib.Lookup(this._controller.window.document, PREF_DIALOG_BUTTONS +
                                      '/{"dlgtype":"' + spec.subtype + '"}');
        break;
      case "deck":
        elem = new elementslib.Lookup(this._controller.window.document, PREF_DIALOG_DECK);
        break;
      case "deck_pane":
        var deck = this.getElement({type: "deck"}).getNode();

        
        var panel = deck.boxObject.firstChild;
        for (var ii = 0; ii < deck.selectedIndex; ii++)
          panel = panel.nextSibling;

        elem = new elementslib.Elem(panel);
        break;
      case "selector":
        elem = new elementslib.Lookup(this._controller.window.document, PREF_DIALOG_SELECTOR);
        break;
      case "selector_button":
        elem = new elementslib.Lookup(this._controller.window.document, PREF_DIALOG_SELECTOR +
                                      '/{"pane":"' + spec.value + '"}');
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
  }
};




var preferences = {
  _branch : Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch),

  





  get branch() {
    return this._branch;
  },

  







  clearUserPref : function preferences_clearUserPref(prefName) {
    try {
      this._branch.clearUserPref(prefName);
      return true;
    } catch (e) {
      return false;
    }
  },

  









  getPref : function preferences_getPref(prefName, defaultValue) {
    try {
      switch (typeof defaultValue) {
        case ('boolean'):
          return this._branch.getBoolPref(prefName);
        case ('string'):
          return this._branch.getCharPref(prefName);
        case ('number'):
          return this._branch.getIntPref(prefName);
        default:
          return undefined;
      }
    } catch(e) {
      return defaultValue;
    }
  },

  










  setPref : function preferences_setPref(name, value) {
    try {
      switch (typeof value) {
        case ('boolean'):
          this._branch.setBoolPref(name, value);
          break;
        case ('string'):
          this._branch.setCharPref(name, value);
          break;
        case ('number'):
          this._branch.setIntPref(name, value);
          break;
        default:
          return false;
      }
    } catch(e) {
      return false;
    }

    return true;
  }
};









function openPreferencesDialog(callback, launcher)
{
  var prefCtrl = null;

  if(!callback)
    throw new Error("No callback given for Preferences Dialog");

  if (mozmill.isWindows) {
    
    var md = collector.getModule('ModalDialogAPI');
    var prefModal = new md.modalDialog(callback);
    prefModal.start();
  }

  
  if (launcher) {
    launcher();

    
    mozmill.controller.sleep(500);
    var win = Cc["@mozilla.org/appshell/window-mediator;1"]
                 .getService(Ci.nsIWindowMediator).getMostRecentWindow(null);
    prefCtrl = new mozmill.controller.MozMillController(win);
  } else {
    prefCtrl = new mozmill.getPreferencesController();
  }

  
  if (!mozmill.isWindows) {
    prefCtrl.sleep(500);
    callback(prefCtrl);
  }

  
  mozmill.controller.sleep(500);
}
