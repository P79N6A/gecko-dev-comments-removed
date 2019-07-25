




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['DownloadsAPI', 'PrefsAPI', 'PrivateBrowsingAPI', 'UtilsAPI'];

const gDelay = 100;
const gTimeout = 5000;

const downloads = [
                   "http://www.adobe.com/education/pdf/etd/etd_lesson2.pdf",
                   "http://switch.dl.sourceforge.net/project/rarexpander/RAR%20Expander/0.8.4/rar_expander_v084.dmg"
                  ];

const downloadsPB = [
                     "http://www.bzip.org/1.0.5/bzip2-1.0.5.tar.gz"
                    ];

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  
  
  dm = new DownloadsAPI.downloadManager();
  dm.cleanAll();

  
  curDownloads = [];

  
  pb = new PrivateBrowsingAPI.privateBrowsing(controller);
  pb.enabled = false;
  pb.showPrompt = false;
}

var teardownModule = function(module)
{
  dm.cleanAll();
  pb.reset();

  PrefsAPI.preferences.clearUserPref("browser.download.manager.showWhenStarting");
}




var testDownloadManagerClosed = function()
{
  
  PrefsAPI.openPreferencesDialog(handlePrefDialog);

  
  for (var ii = 0; ii < downloads.length; ii++)
    DownloadsAPI.downloadFileOfUnknownType(controller, downloads[ii]);

  
  controller.sleep(100);
  controller.waitForEval("subject.activeDownloadCount == 0", 30000, 100, dm);

  
  curDownloads = dm.getAllDownloads();

  
  pb.start();

  
  dm.open(controller);

  var downloadView = new elementslib.ID(dm.controller.window.document, "downloadView");
  dm.controller.waitForElement(downloadView, gTimeout);
  dm.controller.waitForEval("subject.itemCount == 0",
                            gTimeout, 100, downloadView.getNode());

  
  dm.close();

  
  DownloadsAPI.downloadFileOfUnknownType(controller, downloadsPB[0]);

  
  controller.sleep(100);
  controller.waitForEval("subject.activeDownloadCount == 0", 30000, 100, dm);

  
  curDownloads = curDownloads.concat(dm.getAllDownloads());

  pb.stop();

  
  dm.open(controller);

  downloadView = new elementslib.ID(dm.controller.window.document, "downloadView");
  dm.controller.waitForElement(downloadView, gTimeout);
  dm.controller.waitForEval("subject.itemCount == " + downloads.length,
                            gTimeout, 100, downloadView.getNode());

  for (var ii = 0; ii < downloads.length; ii++) {
    var item = new elementslib.ID(dm.controller.window.document, "dl" + (ii + 1));
    dm.controller.assertJS("subject.getAttribute('uri') == '" + downloads[ii] + "'",
                           item.getNode());
  }

  dm.close();
}







var handlePrefDialog = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneMain';

  
  var show = new elementslib.ID(controller.window.document, "showWhenDownloading");
  controller.waitForElement(show, gTimeout);
  controller.check(show, false);

  prefDialog.close(true);
}





