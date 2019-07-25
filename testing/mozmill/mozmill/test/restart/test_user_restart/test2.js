var setupModule = function(module) {
  module.controller = mozmill.getBrowserController();
}




var testShutdownBeforeTimeout = function() {
  controller.startUserShutdown(4000, false);
  controller.click(new elementslib.Elem(controller.menus["file-menu"].menu_FileQuitItem));
}
