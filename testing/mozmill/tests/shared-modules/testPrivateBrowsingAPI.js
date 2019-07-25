










































const MODULE_NAME = 'PrivateBrowsingAPI';

const RELATIVE_ROOT = '.';
const MODULE_REQUIRES = ['ModalDialogAPI', 'PrefsAPI', 'UtilsAPI'];


const PB_NO_PROMPT_PREF = 'browser.privatebrowsing.dont_prompt_on_enter';

const gTimeout = 5000;








function privateBrowsing(controller)
{
  this._prefs = collector.getModule('PrefsAPI').preferences;
  this._utilsApi = collector.getModule('UtilsAPI');
  this._controller = controller;

  



  this._pbMenuItem = new elementslib.Elem(this._controller.menus['tools-menu'].privateBrowsingItem);
  this._pbTransitionItem = new elementslib.ID(this._controller.window.document, "Tools:PrivateBrowsing");

  this.__defineGetter__('_pbs', function() {
    delete this._pbs;
    return this._pbs = Cc["@mozilla.org/privatebrowsing;1"].
                       getService(Ci.nsIPrivateBrowsingService);
  });
}




privateBrowsing.prototype = {
  



  _handler: null,

  





  get enabled() {
    return this._pbs.privateBrowsingEnabled;
  },

  





  set enabled(value) {
    this._pbs.privateBrowsingEnabled = value;
  },

  





  set handler(callback) {
    this._handler = callback;
  },

  





  get showPrompt() {
    return !this._prefs.getPref(PB_NO_PROMPT_PREF, true);
  },

  





  set showPrompt(value){
    this._prefs.setPref(PB_NO_PROMPT_PREF, !value);
  },

  





  getDtds : function downloadManager_getDtds() {
    var dtds = ["chrome://branding/locale/brand.dtd",
                "chrome://browser/locale/browser.dtd",
                "chrome://browser/locale/aboutPrivateBrowsing.dtd"];
    return dtds;
  },

  


  reset : function privateBrowsing_reset() {
    try {
      this.stop(true);
    } catch (ex) {
      
      this.enabled = false;
    }

    this.showPrompt = true;
  },

  





  start: function privateBrowsing_start(useShortcut)
  {
    if (this.enabled)
      return;

    if (this.showPrompt) {
      
      if (!this._handler)
        throw new Error("Private Browsing mode not enabled due to missing callback handler");

      var md = collector.getModule('ModalDialogAPI');
      dialog = new md.modalDialog(this._handler);
      dialog.start();
    }

    if (useShortcut) {
      var cmdKey = this._utilsApi.getEntity(this.getDtds(), "privateBrowsingCmd.commandkey");
      this._controller.keypress(null, cmdKey, {accelKey: true, shiftKey: true});
    } else {
      this._controller.click(this._pbMenuItem);
    }

    this.waitForTransistionComplete(true);
  },

  





  stop: function privateBrowsing_stop(useShortcut)
  {
    if (!this.enabled)
      return;

    if (useShortcut) {
      var privateBrowsingCmdKey = this._utilsApi.getEntity(this.getDtds(), "privateBrowsingCmd.commandkey");
      this._controller.keypress(null, privateBrowsingCmdKey, {accelKey: true, shiftKey: true});
    } else {
      this._controller.click(this._pbMenuItem);
    }

    this.waitForTransistionComplete(false);
  },

  





  waitForTransistionComplete : function privateBrowsing_waitForTransitionComplete(state) {
    
    this._controller.waitForEval("subject.hasAttribute('disabled') == false", gTimeout, 100,
                                 this._pbTransitionItem.getNode());
    this._controller.waitForEval("subject.privateBrowsing.enabled == subject.state", gTimeout, 100,
                                 {privateBrowsing: this, state: state});
  }
}
