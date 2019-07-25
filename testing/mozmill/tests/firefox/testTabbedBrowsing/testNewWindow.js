





































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['UtilsAPI'];

const gDelay = 0;
const gTimeout = 5000;


var newWindow = null;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
}

var teardownModule = function(module) {
  if (newWindow != controller.window) {
    newWindow.close();
  }
}

var testNewWindow = function () {
  
  controller.click(new elementslib.ID(controller.window.document, "home-button"));
  controller.waitForPageLoad();

  var homePageURL = controller.window.document.getElementById("urlbar").value;
  controller.sleep(gDelay);

  
  controller.open('about:blank');
  controller.waitForPageLoad();

  
  var windowCountBeforeTest = mozmill.utils.getWindows().length;

  
  controller.click(new elementslib.Elem(controller.menus['file-menu'].menu_newNavigator));
  controller.sleep(1000);

  
  newWindow = mozmill.wm.getMostRecentWindow("navigator:browser");
  var controller2 = new mozmill.controller.MozMillController(newWindow);
  controller2.waitForPageLoad();

  
  var windowCountAfterTest = mozmill.utils.getWindows().length;
  controller.assertJS("subject.countAfter == subject.countBefore",
                      {countAfter: windowCountAfterTest, countBefore: windowCountBeforeTest + 1});

  
  var locationBar = new elementslib.ID(controller2.window.document, "urlbar");
  controller2.waitForElement(locationBar, gTimeout);
  controller2.assertValue(locationBar, homePageURL);
}





