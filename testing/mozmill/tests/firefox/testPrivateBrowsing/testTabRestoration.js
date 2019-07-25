



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrivateBrowsingAPI', 'TabbedBrowsingAPI', 'UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var websites = [
                {url: 'https://addons.mozilla.org/', id: 'search-query'},
                {url: 'https://bugzilla.mozilla.org', id: 'quicksearch_top'}
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




var testTabRestoration = function()
{
  
  pb.enabled = false;
  pb.showPrompt = false;

  
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

  
  pb.stop();
  controller.waitForPageLoad();

  
  controller.assertJS("subject.tabs.length == " + (websites.length + 1),
                      controller);

  
  for (var ii = 0; ii < websites.length; ii++) {
    var elem = new elementslib.ID(controller.tabs.getTab(ii), websites[ii].id);
    controller.waitForElement(elem, gTimeout);
  }
}





