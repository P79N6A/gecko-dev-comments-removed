










































const MODULE_NAME = 'SoftwareUpdateAPI';

const RELATIVE_ROOT = '.';
const MODULE_REQUIRES = ['PrefsAPI'];

const gTimeout                = 5000;
const gTimeoutUpdateCheck     = 10000;
const gTimeoutUpdateDownload  = 360000;




function softwareUpdate()
{
  this._controller = null;
  this._wizard = null;

  this._aus = Cc["@mozilla.org/updates/update-service;1"]
                 .getService(Ci.nsIApplicationUpdateService);
  this._ums = Cc["@mozilla.org/updates/update-manager;1"]
                 .getService(Ci.nsIUpdateManager);

  
  
  if (mozmill.isMac) {
    var template = '/id("updates")/anon({"anonid":"Buttons"})/anon({"flex":"1"})' +
                   '/{"class":"wizard-buttons-btm"}/';
    this._buttons = {
                      back: template + '{"dlgtype":"back"}',
                      next: template + '{"dlgtype":"next"}',
                      cancel: template + '{"dlgtype":"cancel"}',
                      finish: template + '{"dlgtype":"finish"}',
                      extra1: template + '{"dlgtype":"extra1"}',
                      extra2: template + '{"dlgtype":"extra2"}'
                    };
  } else if (mozmill.isLinux || mozmill.isWindows) {
    var template = '/id("updates")/anon({"anonid":"Buttons"})/anon({"flex":"1"})' +
                   '/{"class":"wizard-buttons-box-2"}/';
    this._buttons = {
                      back: template + '{"dlgtype":"back"}',
                      next: template + 'anon({"anonid":"WizardButtonDeck"})/[1]' +
                                       '/{"dlgtype":"next"}',
                      cancel: template + '{"dlgtype":"cancel"}',
                      finish: template + 'anon({"anonid":"WizardButtonDeck"})/[0]' +
                                         '/{"dlgtype":"finish"}',
                      extra1: template + '{"dlgtype":"extra1"}',
                      extra2: template + '{"dlgtype":"extra2"}'
                    };
  }
}




softwareUpdate.prototype = {

  


  get activeUpdate() {
    return this._ums.activeUpdate;
  },

  


  get allowed() {
    return this._aus.canUpdate;
  },

  


  get currentStep() {
    return this._wizard.getAttribute('currentpageid');
  },

  


  get isCompleteUpdate() {
    
    if (this.activeUpdate.patchCount > 1) {
      var patch1 = this.activeUpdate.getPatchAt(0);
      var patch2 = this.activeUpdate.getPatchAt(1);

      return (patch1.URL == patch2.URL);
    } else {
      return (this.activeUpdate.getPatchAt(0).type == "complete");
    }
  },

  


  get updateType() {
    updateType = new elementslib.ID(this._controller.window.document, "updateType");
    return updateType.getNode().getAttribute("severity");
  },

  






  assertUpdateStep : function softwareUpdate_assertUpdateStep(step) {
    this._controller.waitForEval("subject.currentStep == '" + step + "'",
                                 gTimeout, 100, this);
  },

  


  closeDialog: function softwareUpdate_closeDialog() {
    if (this._controller) {
      this._controller.keypress(null, "VK_ESCAPE", {});
      this._controller.sleep(500);
      this._controller = null;
    }
  },

  






  download : function softwareUpdate_download(updateType, channel, timeout) {
    timeout = timeout ? timeout : gTimeoutUpdateDownload;

    if (this.currentStep == "updatesfound") {
      
      var prefs = collector.getModule('PrefsAPI').preferences;

      this._controller.assertJS("subject.currentChannel == subject.expectedChannel",
                                {currentChannel: channel, expectedChannel: prefs.getPref('app.update.channel','')});

      
      this._controller.assertJS("subject.expectedUpdateType == subject.lastUpdateType",
                                {expectedUpdateType: updateType, lastUpdateType: this.updateType});
    }

    
    var next = new elementslib.Lookup(this._controller.window.document,
                                      this._buttons.next);
    this._controller.click(next);

    
    var progress = this._controller.window.document.getElementById("downloadProgress");
    this._controller.waitForEval("subject.value == 100", timeout, 100, progress);
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

  




  openDialog: function softwareUpdate_openDialog(browserController) {
    
    if (this._controller)
      return;

    var updateMenu = new elementslib.Elem(browserController.menus.helpMenu.checkForUpdates);
    browserController.click(updateMenu);

    this.waitForDialogOpen(browserController);
  },

  



  waitForCheckFinished : function softwareUpdate_waitForCheckFinished(timeout) {
    timeout = timeout ? timeout : gTimeoutUpdateCheck;

    this._controller.waitForEval("subject.currentStep != 'checking'", timeout, 100, this);
  },

  




  waitForDialogOpen : function softwareUpdate_waitForDialogOpen(browserController) {
    browserController.sleep(500);

    var window = mozmill.wm.getMostRecentWindow('Update:Wizard');
    this._controller = new mozmill.controller.MozMillController(window);
    this._wizard = this._controller.window.document.getElementById('updates');

    
    this._controller.waitForEval("subject.currentStep != 'dummy'", gTimeout, 100, this);
    this._controller.window.focus();
  }
}
