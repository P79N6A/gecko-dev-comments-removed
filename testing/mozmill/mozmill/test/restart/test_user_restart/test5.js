var setupModule = function(module) {
  controller = mozmill.getBrowserController();
}




var testRestartAfterTimeout = function(){
  controller.startUserShutdown(1000, true);
  controller.sleep(2000);
  controller.window.Application.restart();
}
