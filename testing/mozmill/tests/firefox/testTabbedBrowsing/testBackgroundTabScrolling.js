




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['PrefsAPI', 'TabbedBrowsingAPI'];

const localTestFolder = collector.addHttpResource('./files');

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  tabBrowser = new TabbedBrowsingAPI.tabBrowser(controller);
  tabBrowser.closeAllTabs();

  container = tabBrowser.getElement({type: "tabs_container"});
  animateBox = tabBrowser.getElement({type: "tabs_animateBox"});
  allTabsButton = tabBrowser.getElement({type: "tabs_allTabsButton"});
  allTabsPopup = tabBrowser.getElement({type: "tabs_allTabsPopup"});
}

var teardownModule = function()
{
  PrefsAPI.preferences.clearUserPref("browser.tabs.loadInBackground");

  
  allTabsPopup.getNode().hidePopup();
}

var testScrollBackgroundTabIntoView = function()
{
  
  PrefsAPI.openPreferencesDialog(prefDialogCallback);

  
  controller.open(localTestFolder + "/openinnewtab.html");
  controller.waitForPageLoad();

  var link1 = new elementslib.Name(controller.tabs.activeTab, "link_1");
  var link2 = new elementslib.Name(controller.tabs.activeTab, "link_2");

  
  var count = 1;
  do {
    controller.middleClick(link1);

    
    controller.waitForEval("subject.length == " + (++count), gTimeout, 100, tabBrowser);
  } while ((container.getNode().getAttribute("overflow") != 'true') || count > 50)

  
  controller.assertJS("subject.getAttribute('overflow') == 'true'",
                      container.getNode())

  
  controller.middleClick(link2);

  
  controller.waitForEval("subject.window.getComputedStyle(subject.animateBox, null).opacity != 0",
                         gTimeout, 10, {window : controller.window, animateBox: animateBox.getNode()});
  controller.waitForEval("subject.window.getComputedStyle(subject.animateBox, null).opacity == 0",
                         gTimeout, 100, {window : controller.window, animateBox: animateBox.getNode()});

  
  var lastIndex = controller.tabs.length - 1;
  var linkId = new elementslib.ID(controller.tabs.getTab(lastIndex), "id");
  controller.assertText(linkId, "2");

  
  controller.click(allTabsButton);
  controller.waitForEval("subject.state == 'open'", gTimeout, 100, allTabsPopup.getNode());

  for (var ii = 0; ii <= lastIndex; ii++) {
    if (ii < lastIndex)
      controller.assertJS("subject.childNodes[" + ii + "].label != '2'",
                          allTabsPopup.getNode());
    else
      controller.assertJS("subject.childNodes[" + ii + "].label == '2'",
                          allTabsPopup.getNode());
  }

  controller.click(allTabsButton);
  controller.waitForEval("subject.state == 'closed'", gTimeout, 100, allTabsPopup.getNode());
}







var prefDialogCallback = function(controller)
{
  var prefDialog = new PrefsAPI.preferencesDialog(controller);
  prefDialog.paneId = 'paneTabs';

  
  var switchToTabsPref = new elementslib.ID(controller.window.document, "switchToNewTabs");
  controller.waitForElement(switchToTabsPref, gTimeout);
  controller.check(switchToTabsPref, false);

  prefDialog.close(true);
}





