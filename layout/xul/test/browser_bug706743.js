add_task(function* () {
  const url = "data:text/html,<html><head></head><body>" +
    "<a id=\"target\" href=\"about:blank\" title=\"This is tooltip text\" " +
            "style=\"display:block;height:20px;margin:10px;\" " +
            "onclick=\"return false;\">here is an anchor element</a></body></html>";

  let tab = yield BrowserTestUtils.openNewForegroundTab(gBrowser, url);
  let browser = gBrowser.selectedBrowser;

  yield new Promise(resolve => {
    SpecialPowers.pushPrefEnv({"set": [["ui.tooltipDelay", 0]]}, resolve);
  });

  
  yield BrowserTestUtils.synthesizeMouse("#target", -5, -5, { type: "mousemove" }, browser);

  
  let popupShownPromise = BrowserTestUtils.waitForEvent(document, "popupshown");
  yield BrowserTestUtils.synthesizeMouse("#target", 5, 15, { type: "mousemove" }, browser);
  yield popupShownPromise;

  
  let popupHiddenPromise = BrowserTestUtils.waitForEvent(document, "popuphidden");
  yield BrowserTestUtils.synthesizeMouse("#target", -5, 15, { type: "mousemove" }, browser);
  yield popupHiddenPromise;

  
  
  
  

  
  

  function tooltipNotExpected()
  {
    ok(false, "tooltip is shown during drag");
  }
  addEventListener("popupshown", tooltipNotExpected, true);

  let dragService = Components.classes["@mozilla.org/widget/dragservice;1"].
                      getService(Components.interfaces.nsIDragService);
  dragService.startDragSession();
  yield BrowserTestUtils.synthesizeMouse("#target", 5, 15, { type: "mousemove" }, browser);

  yield new Promise(resolve => setTimeout(resolve, 100));
  removeEventListener("popupshown", tooltipNotExpected, true);
  dragService.endDragSession(true);

  yield BrowserTestUtils.synthesizeMouse("#target", -5, -5, { type: "mousemove" }, browser);

  
  

  
  popupShownPromise = BrowserTestUtils.waitForEvent(document, "popupshown");
  yield BrowserTestUtils.synthesizeMouse("#target", 5, 15, { type: "mousemove" }, browser);
  yield popupShownPromise;

  
  popupHiddenPromise = BrowserTestUtils.waitForEvent(document, "popuphidden");
  yield BrowserTestUtils.synthesizeMouse("#target", -5, 15, { type: "mousemove" }, browser);
  yield popupHiddenPromise;

  
  popupShownPromise = BrowserTestUtils.waitForEvent(document, "popupshown");
  yield BrowserTestUtils.synthesizeMouse("#target", 5, 15, { type: "mousemove" }, browser);
  yield popupShownPromise;

  popupHiddenPromise = BrowserTestUtils.waitForEvent(document, "popuphidden");
  yield BrowserTestUtils.synthesizeMouse("#target", 5, 15, { type: "mousedown" }, browser);
  yield popupHiddenPromise;

  yield BrowserTestUtils.synthesizeMouse("#target", 5, 15, { type: "mouseup" }, browser);
  yield BrowserTestUtils.synthesizeMouse("#target", -5, 15, { type: "mousemove" }, browser);

  ok(true, "tooltips appear properly");

  gBrowser.removeCurrentTab();
});




