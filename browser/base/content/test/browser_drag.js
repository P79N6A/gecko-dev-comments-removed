function test()
{
  waitForExplicitFinish();

  
  var value = content.location.href;
  var urlString = value + "\n" + content.document.title;
  var htmlString = "<a href=\"" + value + "\">" + value + "</a>";
  var expected = [ [
    { type  : "text/x-moz-url",
      data  : urlString },
    { type  : "text/uri-list",
      data  : value },
    { type  : "text/plain",
      data  : value },
    { type  : "text/html",
      data  : htmlString }
  ] ];
  
  var oldstate = gURLBar.getAttribute("pageproxystate");
  gURLBar.setAttribute("pageproxystate", "valid");
  var dt = EventUtils.synthesizeDragStart(document.getElementById("identity-box"), expected);
  is(dt, null, "drag on proxy icon");
  gURLBar.setAttribute("pageproxystate", oldstate);
  
  
  EventUtils.synthesizeKey("VK_ESCAPE", {}, window);

  
  var tab = gBrowser.addTab("about:blank", {skipAnimation: true});
  var browser = gBrowser.getBrowserForTab(tab);

  browser.addEventListener("load", function () {
    is(browser.contentWindow.location, "http://mochi.test:8888/", "drop on tab");
    gBrowser.removeTab(tab);
    finish();
  }, true);

  EventUtils.synthesizeDrop(tab, tab, [[{type: "text/uri-list", data: "http://mochi.test:8888/"}]], "copy", window);
}
