function test()
{
  var embed = '<embed type="application/x-test" allowscriptaccess="always" allowfullscreen="true" wmode="window" width="640" height="480"></embed>'

  waitForExplicitFinish();
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED);

  
  var tabs = [
    gBrowser.tabs[0],
    gBrowser.addTab("about:blank", {skipAnimation: true}),
    gBrowser.addTab("about:blank", {skipAnimation: true}),
    gBrowser.addTab("about:blank", {skipAnimation: true}),
    gBrowser.addTab("about:blank", {skipAnimation: true})
  ];

  function setLocation(i, url) {
    tabs[i].linkedBrowser.contentWindow.location = url;
  }
  function moveTabTo(a, b) {
    gBrowser.swapBrowsersAndCloseOther(gBrowser.tabs[b], gBrowser.tabs[a]);
  }
  function clickTest(tab, doc, win) {
    var clicks = doc.defaultView.clicks;

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

    is(doc.defaultView.clicks, clicks+1, "adding 1 more click on BODY");
  }
  function test1() {
    moveTabTo(2, 3); 
    is(gBrowser.tabs[1], tabs[1], "tab1");
    is(gBrowser.tabs[2], tabs[3], "tab3");

    var plugin = tabs[4].linkedBrowser.contentDocument.wrappedJSObject.body.firstChild;
    var tab4_plugin_object = plugin.getObjectValue();

    gBrowser.selectedTab = gBrowser.tabs[2];
    moveTabTo(3, 2); 
    gBrowser.selectedTab = tabs[4];
    var doc = gBrowser.tabs[2].linkedBrowser.contentDocument.wrappedJSObject;
    plugin = doc.body.firstChild;
    ok(plugin && plugin.checkObjectValue(tab4_plugin_object), "same plugin instance");
    is(gBrowser.tabs[1], tabs[1], "tab1");
    is(gBrowser.tabs[2], tabs[3], "tab4");
    is(doc.defaultView.clicks, 0, "no click on BODY so far");
    clickTest(gBrowser.tabs[2], doc, window);

    moveTabTo(2, 1); 
    is(gBrowser.tabs[1], tabs[1], "tab1");
    doc = gBrowser.tabs[1].linkedBrowser.contentDocument.wrappedJSObject;
    plugin = doc.body.firstChild;
    ok(plugin && plugin.checkObjectValue(tab4_plugin_object), "same plugin instance");
    clickTest(gBrowser.tabs[1], doc, window);

    
    
    
    var t = tabs[1];
    var b = t.linkedBrowser;
    gBrowser.selectedTab = t;
    b.addEventListener("load", function() {
      b.removeEventListener("load", arguments.callee, true);

      executeSoon(function () {
        var win = gBrowser.replaceTabWithWindow(t);
        whenDelayedStartupFinished(win, function () {
          
          is(gBrowser.tabs[0], tabs[0], "tab0");
          is(gBrowser.tabs[0].linkedBrowser.contentWindow.location, "about:blank", "tab0 uri");

          executeSoon(function () {
            win.gBrowser.addEventListener("pageshow", function () {
              win.gBrowser.removeEventListener("pageshow", arguments.callee, false);
              executeSoon(function () {
                t = win.gBrowser.tabs[0];
                b = t.linkedBrowser;
                var doc = b.contentDocument.wrappedJSObject;
                clickTest(t, doc, win);
                win.close();
                finish();
              });
            }, false);
            win.gBrowser.goBack();
          });
        });
      });
    }, true);
    b.loadURI("about:blank");

  }

  var loads = 0;
  function waitForLoad(event, tab, listenerContainer) {
    var b = tabs[tab].linkedBrowser;
    if (b.contentDocument != event.target) {
      return;
    }
    gBrowser.tabs[tab].linkedBrowser.removeEventListener("load", listenerContainer.listener, true);
    ++loads;
    if (loads == tabs.length - 1) {
      executeSoon(test1);
    }
  }

  function fn(f, arg) {
    var listenerContainer = { listener: null }
    listenerContainer.listener = function (event) { return f(event, arg, listenerContainer); };
    return listenerContainer.listener;
  }
  for (var i = 1; i < tabs.length; ++i) {
    tabs[i].linkedBrowser.addEventListener("load", fn(waitForLoad,i), true);
  }

  setLocation(1, "data:text/html;charset=utf-8,<title>tab1</title><body>tab1<iframe>");
  setLocation(2, "data:text/plain;charset=utf-8,tab2");
  setLocation(3, "data:text/html;charset=utf-8,<title>tab3</title><body>tab3<iframe>");
  setLocation(4, "data:text/html;charset=utf-8,<body onload='clicks=0' onclick='++clicks'>"+embed);
  gBrowser.selectedTab = tabs[3];

}
