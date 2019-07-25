








































const MODULE_NAME = 'SoftwareUpdateAPI';

const RELATIVE_ROOT = '.';
const MODULE_REQUIRES = ['PrefsAPI', 'UtilsAPI'];

const gTimeout                = 5000;
const gTimeoutUpdateCheck     = 10000;
const gTimeoutUpdateDownload  = 360000;


const WIZARD = '/id("updates")';
const WIZARD_BUTTONS = WIZARD + '/anon({"anonid":"Buttons"})';
const WIZARD_DECK = WIZARD  + '/anon({"anonid":"Deck"})';

const WIZARD_PAGES = {
  dummy: 'dummy',
  checking: 'checking',
  pluginUpdatesFound: 'pluginupdatesfound',
  noUpdatesFound: 'noupdatesfound',
  manualUpdate: 'manualUpdate',
  incompatibleCheck: 'incompatibleCheck',
  updatesFoundBasic: 'updatesfoundbasic',
  updatesFoundBillboard: 'updatesfoundbillboard',
  license: 'license',
  incompatibleList: 'incompatibleList',
  downloading: 'downloading',
  errors: 'errors',
  errorPatching: 'errorpatching',
  finished: 'finished',
  finishedBackground: 'finishedBackground',
  installed: 'installed'
}


if (mozmill.isMac) {
  var WIZARD_BUTTONS_BOX = WIZARD_BUTTONS +
                             '/anon({"flex":"1"})/{"class":"wizard-buttons-btm"}/';
  var WIZARD_BUTTON = {
          back: '{"dlgtype":"back"}',
          next: '{"dlgtype":"next"}',
          cancel: '{"dlgtype":"cancel"}',
          finish: '{"dlgtype":"finish"}',
          extra1: '{"dlgtype":"extra1"}',
          extra2: '{"dlgtype":"extra2"}'
        }
} else {
  var WIZARD_BUTTONS_BOX = WIZARD_BUTTONS +
                       '/anon({"flex":"1"})/{"class":"wizard-buttons-box-2"}/';
  var WIZARD_BUTTON = {
    back: '{"dlgtype":"back"}',
    next: 'anon({"anonid":"WizardButtonDeck"})/[1]/{"dlgtype":"next"}',
    cancel: '{"dlgtype":"cancel"}',
    finish: 'anon({"anonid":"WizardButtonDeck"})/[0]/{"dlgtype":"finish"}',
    extra1: '{"dlgtype":"extra1"}',
    extra2: '{"dlgtype":"extra2"}'
  }
}




function softwareUpdate()
{
  this._controller = null;
  this._wizard = null;

  this._PrefsAPI = collector.getModule('PrefsAPI');
  this._UtilsAPI = collector.getModule('UtilsAPI');

  this._aus = Cc["@mozilla.org/updates/update-service;1"].
              getService(Ci.nsIApplicationUpdateService);
  this._ums = Cc["@mozilla.org/updates/update-manager;1"].
              getService(Ci.nsIUpdateManager);
  this._vc = Cc["@mozilla.org/xpcom/version-comparator;1"].
             getService(Ci.nsIVersionComparator);
}




softwareUpdate.prototype = {
  





  get activeUpdate() {
    return this._ums.activeUpdate;
  },

  





  get allowed() {
    return this._aus.canCheckForUpdates && this._aus.canApplyUpdates;
  },

  





  get controller() {
    return this._controller;
  },

  


  get currentPage() {
    return this._wizard.getNode().getAttribute('currentpageid');
  },

  


  get isCompleteUpdate() {
    
    
    if (!this.activeUpdate)
      throw new Error(arguments.callee.name + ": isCompleteUpdate called " +
                      "when activeUpdate is null!");

    var patchCount = this.activeUpdate.patchCount;
    
    controller.assertJS("subject.patchCount < 3",
                        {patchCount: patchCount < 3});
    
    controller.assertJS("subject.patchCount > 0",
                        {patchCount: patchCount > 0});

    
    



       
       




    return (this.activeUpdate.selectedPatch.type  == "complete");
  },

  




  get updateType() {
    return this.activeUpdate.type;
  },

  


  get updatesFound() {
    return this.currentPage.indexOf("updatesfound") == 0;
  },

  





  assertUpdateApplied : function softwareUpdate_assertUpdateApplied(updateData) {
    
    
    var check = this._vc.compare(updateData.postVersion, updateData.preVersion);
  
    controller.assertJS("subject.newVersionGreater == true",
                        {newVersionGreater: check >= 0});
  
    
    if (check == 0) {
      controller.assertJS("subject.postBuildId == subject.updateBuildId",
                          {postBuildId: updateData.postBuildId, updateBuildId: updateData.updateBuildId});
    }
  
    
    controller.assertJS("subject.postLocale == subject.preLocale",
                        {postLocale: updateData.postLocale, preLocale: updateData.preLocale});
  },

  


  closeDialog: function softwareUpdate_closeDialog() {
    if (this._controller) {
      this._controller.keypress(null, "VK_ESCAPE", {});
      this._controller.sleep(500);
      this._controller = null;
      this._wizard = null;
    }
  },

  








  download : function softwareUpdate_download(channel, waitForFinish, timeout) {
    waitForFinish = waitForFinish ? waitForFinish : true;

    
    var prefChannel = this._PrefsAPI.preferences.getPref('app.update.channel', '');
    this._controller.assertJS("subject.currentChannel == subject.expectedChannel",
                              {currentChannel: channel, expectedChannel: prefChannel});

    
    var next = this.getElement({type: "button", subtype: "next"});
    this._controller.click(next);

    
    this.waitForWizardPage(WIZARD_PAGES.downloading);

    if (waitForFinish)
      this.waitforDownloadFinished(timeout);
  },

  


  forceFallback : function softwareUpdate_forceFallback() {
    var dirService = Cc["@mozilla.org/file/directory_service;1"]
                        .getService(Ci.nsIProperties);

    var updateDir;
    var updateStatus;

    
    try {
      updateDir = dirService.get("UpdRootD", Ci.nsIFile);
      updateDir.append("updates");
      updateDir.append("0");

      updateStatus = updateDir.clone();
      updateStatus.append("update.status");
    } catch (ex) {
    }

    if (updateStatus == undefined || !updateStatus.exists()) {
      updateDir = dirService.get("XCurProcD", Ci.nsIFile);
      updateDir.append("updates");
      updateDir.append("0");

      updateStatus = updateDir.clone();
      updateStatus.append("update.status");
    }

    var foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                      .createInstance(Ci.nsIFileOutputStream);
    var status = "failed: 6\n";
    foStream.init(updateStatus, 0x02 | 0x08 | 0x20, -1, 0);
    foStream.write(status, status.length);
    foStream.close();
  },

  





  getDtds : function softwareUpdate_getDtds() {
    var dtds = ["chrome://mozapps/locale/update/history.dtd",
                "chrome://mozapps/locale/update/updates.dtd"]
    return dtds;
  },

  










  getElement : function softwareUpdate_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      



      case "button":
        elem = new elementslib.Lookup(this._controller.window.document,
                                      WIZARD_BUTTONS_BOX + WIZARD_BUTTON[spec.subtype]);
        break;
      case "wizard":
        elem = new elementslib.Lookup(this._controller.window.document, WIZARD);
        break;
      case "wizard_page":
        elem = new elementslib.Lookup(this._controller.window.document, WIZARD_DECK +
                                      '/id(' + spec.subtype + ')');
        break;
      case "download_progress":
        elem = new elementslib.ID(this._controller.window.document, "downloadProgress");
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
  },

  





  openDialog: function softwareUpdate_openDialog(browserController) {
    
    

    
    
    var appVersion = this._UtilsAPI.appInfo.version;

    if (!mozmill.isMac && this._vc.compare(appVersion, "4.0b7pre") >= 0) {
      
      aboutItem = new elementslib.Elem(browserController.menus.helpMenu.aboutName);
      browserController.click(aboutItem);

      this._UtilsAPI.handleWindow("type", "Browser:About", function(controller) {
        
        
        var updateButton = new elementslib.ID(controller.window.document,
                                              "checkForUpdatesButton");
        
        controller.waitForElement(updateButton, gTimeout);
      });

      
      var updatePrompt = Cc["@mozilla.org/updates/update-prompt;1"].
                         createInstance(Ci.nsIUpdatePrompt);
      updatePrompt.checkForUpdates();
    } else {
      updateItem = new elementslib.Elem(browserController.menus.helpMenu.checkForUpdates);
      browserController.click(updateItem);
    }

    this.waitForDialogOpen(browserController);
  },

  



  waitForCheckFinished : function softwareUpdate_waitForCheckFinished(timeout) {
    timeout = timeout ? timeout : gTimeoutUpdateCheck;

    this._controller.waitForEval("subject.wizard.currentPage != subject.checking", timeout, 100,
                                 {wizard: this, checking: WIZARD_PAGES.checking});
  },

  





  waitForDialogOpen : function softwareUpdate_waitForDialogOpen(browserController) {
    this._controller = this._UtilsAPI.handleWindow("type", "Update:Wizard",
                                                   null, true);
    this._wizard = this.getElement({type: "wizard"});

    
    this._controller.waitForEval("subject.wizard.currentPage != subject.dummy", gTimeout, 100,
                                 {wizard: this, dummy: WIZARD_PAGES.dummy});
    this._controller.window.focus();
  },

  





  waitforDownloadFinished: function softwareUpdate_waitForDownloadFinished(timeout) {
    timeout = timeout ? timeout : gTimeoutUpdateDownload;

    
    var progress =  this.getElement({type: "download_progress"});
    this._controller.waitForEval("subject.progress.value == 100", timeout, 100,
                                 {progress: progress.getNode()});

    this.waitForWizardPage(WIZARD_PAGES.finished);
  },

  


  waitForWizardPage : function softwareUpdate_waitForWizardPage(step) {
    this._controller.waitForEval("subject.currentPage == '" + step + "'",
                                 gTimeout, 100, this);
  }
}
