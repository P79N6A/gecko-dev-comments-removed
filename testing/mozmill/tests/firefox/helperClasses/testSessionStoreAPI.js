



































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['SessionStoreAPI'];

const gDelay = 0;
const gTimeout = 5000;

var setupModule = function(module)
{
  module.controller = mozmill.getBrowserController();

  module.session = new SessionStoreAPI.aboutSessionRestore(controller);
}

var testAboutSessionRestoreErrorPage = function()
{
  controller.open("about:sessionrestore");
  controller.sleep(400);

  
  var list = session.getElement({type: "tabList"});
  var windows = session.getWindows();

  for (var ii = 0; ii < windows.length; ii++) {
    var window = windows[ii];
    var tabs = session.getTabs(window);

    for (var jj = 0; jj < tabs.length; jj++) {
      var tab = tabs[jj];

      if (jj == 0) {
        session.toggleRestoreState(tab);
      }
    }
  }

  
  var button = session.getElement({type: "button_restoreSession"});
  controller.assertJS("subject.getAttribute('oncommand') == 'restoreSession();'", button.getNode());

  controller.keypress(null, "t", {accelKey: true});
  controller.open("http://www.google.com");
  controller.waitForPageLoad();
  controller.keypress(null, "w", {accelKey: true});

  SessionStoreAPI.undoClosedTab(controller, {type: "shortcut"});
  
}
