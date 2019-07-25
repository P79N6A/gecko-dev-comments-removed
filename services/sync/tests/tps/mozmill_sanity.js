




































var jum = {}; Components.utils.import('resource://mozmill/modules/jum.js', jum);

var setupModule = function(module) {
  controller = mozmill.getBrowserController();
  jum.assert(true, "SetupModule passes");
}

var setupTest = function(module) {
  jum.assert(true, "SetupTest passes");
}

var testTestStep = function() {
  jum.assert(true, "test Passes");
  controller.open("http://www.mozilla.org");
}

var teardownTest = function () {
  jum.assert(true, "teardownTest passes");
}

var teardownModule = function() {
  jum.assert(true, "teardownModule passes");
}
