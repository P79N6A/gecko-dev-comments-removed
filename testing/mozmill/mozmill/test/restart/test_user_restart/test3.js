var setupModule = function(module) {
  controller = mozmill.getBrowserController();
}





var testNoExpectedRestartByTimeout = function(){
  controller.startUserShutdown(1000, true);
  controller.sleep(2000);
}
