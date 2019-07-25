



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrivateBrowsingAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

const websites = [
                  {url: 'http://www.mozilla.org', id: 'q'},
                  {url: 'about:', id: 'aboutPageList'}
                 ];

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  modifier = controller.window.document.documentElement
                       .getAttribute("titlemodifier_privatebrowsing");

  
  pb = new PrivateBrowsingAPI.privateBrowsing(controller);
  pb.handler = pbStartHandler;

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var teardownModule = function(module)
{
  pb.reset();
}




var testEnablePrivateBrowsingMode = function()
{
  
  pb.enabled = false;
  pb.showPrompt = true;

  
  var newTab = new elementslib.Elem(controller.menus['file-menu'].menu_newNavigatorTab);
  for (var ii = 0; ii < websites.length; ii++) {
    controller.open(websites[ii].url);
    controller.click(newTab);
  }

  
  for (var ii = 0; ii < websites.length; ii++) {
    var elem = new elementslib.ID(controller.tabs.getTab(ii), websites[ii].id);
    controller.waitForElement(elem, gTimeout);
  }

  
  pb.start();

  
  controller.assertJS("subject.tabs.length == 1", controller);

  
  controller.assertJS("subject.title.indexOf('" + modifier + "') != -1",
                      controller.window.document);

  
  
  var longDescElem = new elementslib.ID(controller.tabs.activeTab, "errorLongDescText")
  var moreInfoElem = new elementslib.ID(controller.tabs.activeTab, "moreInfoLink");

  controller.waitForElement(longDescElem, gTimeout);
  controller.waitForElement(moreInfoElem, gTimeout);
}




var testStopPrivateBrowsingMode = function()
{
  
  pb.enabled = true;

  
  pb.stop();

  
  controller.assertJS("subject.tabs.length == " + (websites.length + 1),
                      controller);

  for (var ii = 0; ii < websites.length; ii++) {
    var elem = new elementslib.ID(controller.tabs.getTab(ii), websites[ii].id);
    controller.waitForElement(elem, gTimeout);
  }

  
  controller.assertJS("subject.title.indexOf('" + modifier + "') == -1",
                      controller.window.document);
}




var testKeyboardShortcut = function()
{
  
  pb.enabled = false;
  pb.showPrompt = true;

  
  pb.start(true);

  
  pb.stop(true);
}







var pbStartHandler = function(controller)
{
  
  var checkbox = new elementslib.ID(controller.window.document, 'checkbox');
  controller.waitThenClick(checkbox, gTimeout);

  var okButton = new elementslib.Lookup(controller.window.document, '/id("commonDialog")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  controller.click(okButton);
}







