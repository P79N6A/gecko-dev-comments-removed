




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['SearchAPI'];

const gDelay   = 0;
const gTimeout = 5000;


var gSharedData = {preEngines: [ ], postEngines: [ ]};

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  search = new SearchAPI.searchBar(controller);
}

var teardownModule = function(module)
{
  search.restoreDefaultEngines();
}




var testReorderEngines = function()
{
  
  search.openEngineManager(reorderEngines);

  
  search.openEngineManager(retrieveEngines);

  
  controller.assertJS("subject.preEngines[0].name == subject.postEngines[2].name",
                      {preEngines: gSharedData.preEngines, postEngines: gSharedData.postEngines});
  controller.assertJS("subject.preEngines[1].name == subject.postEngines[1].name",
                      {preEngines: gSharedData.preEngines, postEngines: gSharedData.postEngines});
  controller.assertJS("subject.preEngines[2].name == subject.postEngines[0].name",
                      {preEngines: gSharedData.preEngines, postEngines: gSharedData.postEngines});
  controller.assertJS("subject.preEngines[subject.length - 1].name == subject.postEngines[subject.length - 2].name",
                      {preEngines: gSharedData.preEngines, postEngines: gSharedData.postEngines,
                       length: gSharedData.preEngines.length});

  
  controller.sleep(0);

  
  var engines = search.visibleEngines;
  for (var ii = 0; ii < engines.length; ii++) {
    controller.assertJS("subject.visibleEngine.name == subject.expectedEngine.name",
                        {visibleEngine: engines[ii], expectedEngine: gSharedData.postEngines[ii]});
  }
}







var reorderEngines = function(controller)
{
  var manager = new SearchAPI.engineManager(controller);
  var engines = manager.engines;

  
  manager.moveDownEngine(engines[0].name); 
  manager.controller.sleep(gDelay);
  manager.moveDownEngine(engines[0].name); 
  manager.controller.sleep(gDelay);
  manager.moveDownEngine(engines[1].name); 
  manager.controller.sleep(gDelay);

  
  manager.moveUpEngine(engines[engines.length - 1].name);
  manager.controller.sleep(gDelay);

  
  gSharedData.preEngines = engines;

  manager.close(true);
}







var retrieveEngines = function(controller)
{
  var manager = new SearchAPI.engineManager(controller);

  
  gSharedData.postEngines = manager.engines;

  manager.close(true);
}
