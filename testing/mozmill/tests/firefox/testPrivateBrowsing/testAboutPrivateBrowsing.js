



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrivateBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  
  pb = new PrivateBrowsingAPI.privateBrowsing(controller);
}

var setupTest = function(module)
{
  
  pb.enabled = false;
  pb.showPrompt = false;
}

var teardownTest = function(test)
{
  pb.reset();
}




var testCheckRegularMode = function()
{
  controller.open("about:privatebrowsing");
  controller.waitForPageLoad();

  
  var statusText = new elementslib.ID(controller.tabs.activeTab, "errorShortDescTextNormal");

  
  var button = new elementslib.ID(controller.tabs.activeTab, "startPrivateBrowsing");
  controller.click(button);
  controller.waitForPageLoad();

  controller.waitForEval("subject.privateBrowsing.enabled == true", gTimeout, 100,
                         {privateBrowsing: pb});
}




var testCheckPrivateBrowsingMode = function()
{
  
  pb.start();
  controller.waitForPageLoad();

  var moreInfo = new elementslib.ID(controller.tabs.activeTab, "moreInfoLink");
  controller.waitThenClick(moreInfo, gTimeout);

  
  var targetUrl = UtilsAPI.formatUrlPref("app.support.baseURL") + "private-browsing";

  controller.waitForEval("subject.length == 2", gTimeout, 100, controller.tabs);
  controller.waitForPageLoad();
  UtilsAPI.assertLoadedUrlEqual(controller, targetUrl);
}





