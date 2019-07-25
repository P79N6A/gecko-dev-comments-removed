





































const TIMEOUT = 5000;

const LOCAL_TEST_FOLDER = collector.addHttpResource('../test-files/');
const LOCAL_TEST_PAGES = [
  {url: LOCAL_TEST_FOLDER + 'layout/mozilla.html', id: 'community'},
  {url: LOCAL_TEST_FOLDER + 'layout/mozilla_mission.html', id: 'mission_statement'},
  {url: LOCAL_TEST_FOLDER + 'layout/mozilla_grants.html', id: 'accessibility'} 
];

var setupModule = function() {
  controller = mozmill.getBrowserController();
}




var testBackAndForward = function() {
  var backButton = new elementslib.ID(controller.window.document, "back-button");
  var forwardButton = new elementslib.ID(controller.window.document, "forward-button");
  
  
  for each (var localPage in LOCAL_TEST_PAGES) {
    controller.open(localPage.url);
    controller.waitForPageLoad();
   
    var element = new elementslib.ID(controller.tabs.activeTab, localPage.id);
    controller.assertNode(element);
  }

  
  for (var i = LOCAL_TEST_PAGES.length - 2; i >= 0; i--) {
    controller.click(backButton);

    var element = new elementslib.ID(controller.tabs.activeTab, LOCAL_TEST_PAGES[i].id);
    controller.waitForElement(element, TIMEOUT);
  }

  
  for (var j = 1; j < LOCAL_TEST_PAGES.length; j++) {
    controller.click(forwardButton);

    var element = new elementslib.ID(controller.tabs.activeTab, LOCAL_TEST_PAGES[j].id);
    controller.waitForElement(element, TIMEOUT);
  }
}





