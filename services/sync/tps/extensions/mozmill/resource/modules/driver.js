






var driver = exports;

Cu.import("resource://gre/modules/Services.jsm");


var assertions = {}; Cu.import('resource://mozmill/modules/assertions.js', assertions);
var mozmill = {}; Cu.import("resource://mozmill/driver/mozmill.js", mozmill);
var utils = {}; Cu.import('resource://mozmill/stdlib/utils.js', utils);









function getBrowserWindow(aOpenIfNone) {
  
  if (typeof aOpenIfNone === 'undefined') {
    aOpenIfNone = true;
  }

  
  let win = getTopmostWindowByType("navigator:browser", !aOpenIfNone);

  
  
  if (!win)
    win = openBrowserWindow();

  return win;
}








function getHiddenWindow() {
  return Services.appShell.hiddenDOMWindow;
}








function openBrowserWindow() {
  
  
  
  var win = mozmill.isMac ? getHiddenWindow() :
                            getTopmostWindowByType("navigator:browser", true);
  return win.OpenBrowserWindow();
}








var sleep = utils.sleep;







var waitFor = assertions.Assert.waitFor;

















function _getWindows(aEnumerator, aFilterCallback, aStrict) {
  
  if (typeof aStrict === 'undefined')
    aStrict = true;

  let windows = [];

  while (aEnumerator.hasMoreElements()) {
    let window = aEnumerator.getNext();

    if (!aFilterCallback || aFilterCallback(window)) {
      windows.push(window);
    }
  }

  
  if (windows.length === 0 && aStrict) {
    var message = 'No windows were found';

    
    if (aFilterCallback && aFilterCallback.name)
      message += ' using filter "' + aFilterCallback.name + '"';

    throw new Error(message);
  }

  return windows;
}












function windowFilterByMethod(aName) {
  return function byMethod(aWindow) { return (aName in aWindow); }
}








function windowFilterByTitle(aTitle) {
  return function byTitle(aWindow) { return (aWindow.document.title === aTitle); }
}









function windowFilterByType(aType) {
  return function byType(aWindow) {
           var type = aWindow.document.documentElement.getAttribute("windowtype");
           return (type === aType);
         }
}















function getWindowsByAge(aFilterCallback, aStrict) {
  var windows = _getWindows(Services.wm.getEnumerator(""),
                            aFilterCallback, aStrict);

  
  return windows.reverse();
}












function getWindowsByZOrder(aFilterCallback, aStrict) {
  return _getWindows(Services.wm.getZOrderDOMWindowEnumerator("", true),
                     aFilterCallback, aStrict);
}














function getNewestWindow(aFilterCallback, aStrict) {
  var windows = getWindowsByAge(aFilterCallback, aStrict);
  return windows.length ? windows[0] : null;
}










function getTopmostWindow(aFilterCallback, aStrict) {
  var windows = getWindowsByZOrder(aFilterCallback, aStrict);
  return windows.length ? windows[0] : null;
}















function getTopmostWindowByType(aWindowType, aStrict) {
  if (typeof aStrict === 'undefined')
    aStrict = true;

  var win = Services.wm.getMostRecentWindow(aWindowType);

  if (win === null && aStrict) {
    var message = 'No windows of type "' + aWindowType + '" were found';
    throw new errors.UnexpectedError(message);
  }

  return win;
}



driver.getBrowserWindow = getBrowserWindow;
driver.getHiddenWindow = getHiddenWindow;
driver.openBrowserWindow = openBrowserWindow;
driver.sleep = sleep;
driver.waitFor = waitFor;

driver.windowFilterByMethod = windowFilterByMethod;
driver.windowFilterByTitle = windowFilterByTitle;
driver.windowFilterByType = windowFilterByType;

driver.getWindowsByAge = getWindowsByAge;
driver.getNewestWindow = getNewestWindow;
driver.getTopmostWindowByType = getTopmostWindowByType;






