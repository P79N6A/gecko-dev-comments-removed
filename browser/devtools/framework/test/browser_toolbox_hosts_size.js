









const URL = "data:text/html;charset=utf8,test for host sizes";

add_task(function*() {
  
  
  Services.prefs.setIntPref("devtools.toolbox.footer.height", 10000);
  Services.prefs.setIntPref("devtools.toolbox.sidebar.width", 10000);

  let tab = yield addTab(URL);
  let nbox = gBrowser.getNotificationBox();
  let {clientHeight: nboxHeight, clientWidth: nboxWidth} = nbox;
  let toolbox = yield gDevTools.showToolbox(TargetFactory.forTab(tab));

  is (nbox.clientHeight, nboxHeight, "Opening the toolbox hasn't changed the height of the nbox");
  is (nbox.clientWidth, nboxWidth, "Opening the toolbox hasn't changed the width of the nbox");

  let iframe = document.getAnonymousElementByAttribute(nbox, "class", "devtools-toolbox-bottom-iframe");
  is (iframe.clientHeight, nboxHeight - 10, "The iframe fits within the available space ");

  yield toolbox.switchHost(devtools.Toolbox.HostType.SIDE);
  iframe = document.getAnonymousElementByAttribute(nbox, "class", "devtools-toolbox-side-iframe");
  iframe.style.minWidth = "1px"; 
  is (iframe.clientWidth, nboxWidth - 10, "The iframe fits within the available space");

  yield cleanup(toolbox);
});

add_task(function*() {
  
  
  Services.prefs.setIntPref("devtools.toolbox.footer.height", 100);
  Services.prefs.setIntPref("devtools.toolbox.sidebar.width", 100);

  let tab = yield addTab(URL);
  let nbox = gBrowser.getNotificationBox();
  let {clientHeight: nboxHeight, clientWidth: nboxWidth} = nbox;
  let toolbox = yield gDevTools.showToolbox(TargetFactory.forTab(tab));

  is (nbox.clientHeight, nboxHeight, "Opening the toolbox hasn't changed the height of the nbox");
  is (nbox.clientWidth, nboxWidth, "Opening the toolbox hasn't changed the width of the nbox");

  let iframe = document.getAnonymousElementByAttribute(nbox, "class", "devtools-toolbox-bottom-iframe");
  is (iframe.clientHeight, 100, "The iframe is resized properly");

  yield toolbox.switchHost(devtools.Toolbox.HostType.SIDE);
  iframe = document.getAnonymousElementByAttribute(nbox, "class", "devtools-toolbox-side-iframe");
  iframe.style.minWidth = "1px"; 
  is (iframe.clientWidth, 100, "The iframe is resized properly");

  yield cleanup(toolbox);
});

function* cleanup(toolbox) {
  Services.prefs.clearUserPref("devtools.toolbox.host");
  Services.prefs.clearUserPref("devtools.toolbox.footer.height");
  Services.prefs.clearUserPref("devtools.toolbox.sidebar.width");
  yield toolbox.destroy();
  gBrowser.removeCurrentTab();
}
