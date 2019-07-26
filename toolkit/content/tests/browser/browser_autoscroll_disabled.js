function test()
{
  const kPrefName_AutoScroll = "general.autoScroll";
  Services.prefs.setBoolPref(kPrefName_AutoScroll, false);

  var doc;

  function startLoad(dataUri) {
    gBrowser.selectedBrowser.addEventListener("pageshow", onLoad, false);
    gBrowser.loadURI(dataUri);
  }

  function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("pageshow", onLoad, false);
    waitForFocus(onFocus, content);
  }

  function onFocus() {
    doc = gBrowser.contentDocument;
    runChecks();
  }

  function endTest() {
    
    if (Services.prefs.prefHasUserValue(kPrefName_AutoScroll))
      Services.prefs.clearUserPref(kPrefName_AutoScroll);

    
    waitForFocus(finish);
  }

  waitForExplicitFinish();

  let dataUri = 'data:text/html,<html><body id="i" style="overflow-y: scroll"><div style="height: 2000px"></div>\
      <iframe id="iframe" style="display: none;"></iframe>\
</body></html>';
  startLoad(dataUri);

  function runChecks() {
    var elem = doc.getElementById('i');
    
    
    var skipFrames = 1;
    var checkScroll = function () {
      if (skipFrames--) {
        window.mozRequestAnimationFrame(checkScroll);
        return;
      }
      ok(elem.scrollTop == 0, "element should not have scrolled vertically");
      ok(elem.scrollLeft == 0, "element should not have scrolled horizontally");

      endTest();
    };
    EventUtils.synthesizeMouse(elem, 50, 50, { button: 1 },
                               gBrowser.contentWindow);

    var iframe = gBrowser.contentDocument.getElementById("iframe");
    var e = iframe.contentDocument.createEvent("pagetransition");
    e.initPageTransitionEvent("pagehide", true, true, false);
    iframe.contentDocument.dispatchEvent(e);
    iframe.contentDocument.documentElement.dispatchEvent(e);

    EventUtils.synthesizeMouse(elem, 100, 100,
                               { type: "mousemove", clickCount: "0" },
                               gBrowser.contentWindow);
    




    window.mozRequestAnimationFrame(checkScroll);
  }
}
