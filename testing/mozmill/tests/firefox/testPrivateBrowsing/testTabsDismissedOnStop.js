



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrivateBrowsingAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var websites = [
                {url: 'http://www.mozilla.org', id: 'q'},
                {url: 'about:', id: 'aboutPageList'}
               ];

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();
  pb = new PrivateBrowsingAPI.privateBrowsing(controller);

  TabbedBrowsingAPI.closeAllTabs(controller);
}

var teardownModule = function(module)
{
  pb.reset();
}





var testAllTabsClosedOnStop = function()
{
  
  pb.enabled = false;
  pb.showPrompt = false;

  
  pb.start();

  
  var newTab = new elementslib.Elem(controller.menus['file-menu'].menu_newNavigatorTab);
  for (var ii = 0; ii < websites.length; ii++) {
    controller.open(websites[ii].url);
    controller.click(newTab);
  }

  
  for (var ii = 0; ii < websites.length; ii++) {
    var elem = new elementslib.ID(controller.tabs.getTab(ii), websites[ii].id);
    controller.waitForElement(elem, gTimeout);
  }

  pb.stop();

  
  controller.assertJS("subject.tabs.length == 1", controller);
}





