




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['SearchAPI'];

const gDelay   = 200;
const gTimeout = 5000;


var gSharedData = {preEngines: [ ]};

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  search = new SearchAPI.searchBar(controller);
}

var teardownModule = function(module)
{
  search.restoreDefaultEngines();
}




var testRestoreDefaultEngines = function()
{
  
  search.openEngineManager(removeEngines);

  
  search.openEngineManager(restoreEngines);

  
  controller.sleep(0);

  
  var engines = search.visibleEngines;
  for (var ii = 0; ii < engines.length; ii++) {
    controller.assertJS("subject.visibleEngine.name == subject.expectedEngine.name",
                        {visibleEngine: engines[ii], expectedEngine: gSharedData.preEngines[ii]});
  }
}







var removeEngines = function(controller)
{
  var manager = new SearchAPI.engineManager(controller);

  
  gSharedData.preEngines = manager.engines;

  
  for (var ii = manager.engines.length; ii > 3; ii--) {
    var index = Math.floor(Math.random() * ii);

    manager.removeEngine(manager.engines[index].name);
    manager.controller.sleep(gDelay);
  }

  manager.close(true);
}







var restoreEngines = function(controller)
{
  var manager = new SearchAPI.engineManager(controller);

  manager.controller.assertJS("subject.numberOfEngines == 3",
                              {numberOfEngines: manager.engines.length});

  manager.restoreDefaults();

  manager.close(true);
}
