function swapTabsAndCloseOther(a, b) {
  gBrowser.swapBrowsersAndCloseOther(gBrowser.tabs[b], gBrowser.tabs[a]);
}

let getClicks = function(tab) {
  return ContentTask.spawn(tab.linkedBrowser, {}, function() {
    return content.wrappedJSObject.clicks;
  });
}

let clickTest = Task.async(function*(tab) {
  let clicks = yield getClicks(tab);

  yield ContentTask.spawn(tab.linkedBrowser, {}, function() {
    let target = content.document.body;
    let rect = target.getBoundingClientRect();
    let left = (rect.left + rect.right) / 2;
    let top = (rect.top + rect.bottom) / 2;

    let utils = content.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                       .getInterface(Components.interfaces.nsIDOMWindowUtils);
    utils.sendMouseEvent("mousedown", left, top, 0, 1, 0, false, 0, 0);
    utils.sendMouseEvent("mouseup", left, top, 0, 1, 0, false, 0, 0);
  });

  let newClicks = yield getClicks(tab);
  is(newClicks, clicks + 1, "adding 1 more click on BODY");
});

function loadURI(tab, url) {
  tab.linkedBrowser.loadURI(url);
  return BrowserTestUtils.browserLoaded(tab.linkedBrowser);
}

add_task(function*() {
  let embed = '<embed type="application/x-test" allowscriptaccess="always" allowfullscreen="true" wmode="window" width="640" height="480"></embed>'
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED);

  
  let tabs = [
    gBrowser.tabs[0],
    gBrowser.addTab("about:blank", {skipAnimation: true}),
    gBrowser.addTab("about:blank", {skipAnimation: true}),
    gBrowser.addTab("about:blank", {skipAnimation: true}),
    gBrowser.addTab("about:blank", {skipAnimation: true})
  ];

  
  yield loadURI(tabs[1], "data:text/html;charset=utf-8,<title>tab1</title><body>tab1<iframe>");
  yield loadURI(tabs[2], "data:text/plain;charset=utf-8,tab2");
  yield loadURI(tabs[3], "data:text/html;charset=utf-8,<title>tab3</title><body>tab3<iframe>");
  yield loadURI(tabs[4], "data:text/html;charset=utf-8,<body onload='clicks=0' onclick='++clicks'>"+embed);
  gBrowser.selectedTab = tabs[3];

  swapTabsAndCloseOther(2, 3); 
  is(gBrowser.tabs[1], tabs[1], "tab1");
  is(gBrowser.tabs[2], tabs[3], "tab3");
  is(gBrowser.tabs[3], tabs[4], "tab4");

  let plugin = tabs[4].linkedBrowser.contentDocument.wrappedJSObject.body.firstChild;
  let tab4_plugin_object = plugin.getObjectValue();

  swapTabsAndCloseOther(3, 2); 
  gBrowser.selectedTab = gBrowser.tabs[2];

  let doc = gBrowser.tabs[2].linkedBrowser.contentDocument.wrappedJSObject;
  plugin = doc.body.firstChild;
  ok(plugin && plugin.checkObjectValue(tab4_plugin_object), "same plugin instance");

  is(gBrowser.tabs[1], tabs[1], "tab1");
  is(gBrowser.tabs[2], tabs[3], "tab4");

  let clicks = yield getClicks(gBrowser.tabs[2]);
  is(clicks, 0, "no click on BODY so far");
  yield clickTest(gBrowser.tabs[2]);

  swapTabsAndCloseOther(2, 1); 
  is(gBrowser.tabs[1], tabs[1], "tab1");

  doc = gBrowser.tabs[1].linkedBrowser.contentDocument.wrappedJSObject;
  plugin = doc.body.firstChild;
  ok(plugin && plugin.checkObjectValue(tab4_plugin_object), "same plugin instance");

  yield clickTest(gBrowser.tabs[1]);

  
  
  
  gBrowser.selectedTab = tabs[1];
  yield loadURI(tabs[1], "about:blank");
  let key = tabs[1].linkedBrowser.permanentKey;

  let win = gBrowser.replaceTabWithWindow(tabs[1]);
  yield new Promise(resolve => whenDelayedStartupFinished(win, resolve));

  
  is(gBrowser.tabs[0], tabs[0], "tab0");
  is(gBrowser.tabs[0].linkedBrowser.currentURI.spec, "about:blank", "tab0 uri");

  let tab = win.gBrowser.tabs[0];
  is(tab.linkedBrowser.permanentKey, key, "Should have kept the key");

  let pageshowPromise = ContentTask.spawn(tab.linkedBrowser, {}, function*() {
    return new Promise(resolve => {
      let listener = function() {
        removeEventListener("pageshow", listener, false);
        resolve();
      }
      addEventListener("pageshow", listener, false);
    });
  });
  win.gBrowser.goBack();
  yield pageshowPromise;

  yield clickTest(tab);
  win.close();
});
