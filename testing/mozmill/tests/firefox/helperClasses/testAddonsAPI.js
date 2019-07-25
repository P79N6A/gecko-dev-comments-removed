



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['AddonsAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  am = new AddonsAPI.addonsManager();
}

var teardownModule = function(module) {
  am.close(true);
}

var testAddonsManager = function() {
  am.open(controller);

  am.paneId = "themes";

  am.search("rss");
  am.clearSearchField();

  am.close();
}
