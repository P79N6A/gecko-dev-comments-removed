



var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
};

var testGetNode = function() {
  controller.open("about:support");
  controller.waitForPageLoad();

  var appbox = findElement.ID(controller.tabs.activeTab, "application-box");
  assert.waitFor(() => appbox.getNode().textContent == 'Firefox', 'correct app name');
};
