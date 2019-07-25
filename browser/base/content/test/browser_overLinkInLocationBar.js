









































var gTestIter;



let gTests = [

  function smokeTestGenerator() {
    
    gBrowser.contentWindow.focus();

    if (ensureOverLinkHidden())
      yield;

    setOverLinkWait("http://example.com/");
    yield;
    checkURLBar(true);

    setOverLinkWait("");
    yield;
    checkURLBar(false);
  },

  function hostPathLabels() {
    updateOverLink("http://example.com/");
    hostLabelIs("http://example.com/");
    pathLabelIs("");

    updateOverLink("http://example.com/foo");
    hostLabelIs("http://example.com/");
    pathLabelIs("foo");

    updateOverLink("javascript:popup('http://example.com/')");
    hostLabelIs("");
    pathLabelIs("javascript:popup('http://example.com/')");

    updateOverLink("javascript:popup('http://example.com/foo')");
    hostLabelIs("");
    pathLabelIs("javascript:popup('http://example.com/foo')");

    updateOverLink("about:home");
    hostLabelIs("");
    pathLabelIs("about:home");
  }

];

function test() {
  waitForExplicitFinish();
  runNextTest();
}

function runNextTest() {
  let nextTest = gTests.shift();
  if (nextTest) {
    dump("Running next test: " + nextTest.name + "\n");
    gTestIter = nextTest();

    
    if (gTestIter)
      cont();
    else
      runNextTest();
  }
  else
    finish();
}







function cont() {
  try {
    gTestIter.next();
  }
  catch (err if err instanceof StopIteration) {
    runNextTest();
  }
  catch (err) {
    
    
    ok(false, "Exception: " + err);
    throw err;
  }
}







function checkURLBar(shouldShowOverLink) {
  let overLink = window.getComputedStyle(gURLBar._overLinkBox, null);
  let origin = window.getComputedStyle(gURLBar._originLabel, null);
  let editLayer = window.getComputedStyle(gURLBar._textboxContainer, null);
  if (shouldShowOverLink) {
    isnot(origin.color, "transparent",
          "Origin color in over-link layer should not be transparent");
    is(overLink.opacity, 1, "Over-link should be opaque");
    is(editLayer.color, "transparent",
       "Edit layer color should be transparent");
  }
  else {
    is(origin.color, "transparent",
       "Origin color in over-link layer should be transparent");
    is(overLink.opacity, 0, "Over-link should be transparent");
    isnot(editLayer.color, "transparent",
          "Edit layer color should not be transparent");
  }
}









function setOverLinkWait(str) {
  dump("Setting over-link and waiting: " + str + "\n");

  let overLink = gURLBar._overLinkBox;
  let opacity = window.getComputedStyle(overLink, null).opacity;
  dump("Opacity is " + opacity + "\n");

  ok(str || opacity != 0,
     "Test assumption: either showing or if hiding, not already hidden");
  ok(!str || opacity != 1,
     "Test assumption: either hiding or if showing, not already shown");

  overLink.addEventListener("transitionend", function onTrans(event) {
    dump("transitionend received: " + (event.target == overLink) + " " +
         event.propertyName + "\n");
    if (event.target == overLink && event.propertyName == "opacity") {
      dump("target transitionend received\n");
      overLink.removeEventListener("transitionend", onTrans, false);
      cont();
    }
  }, false);
  gURLBar.setOverLink(str);
}








function setOverLink(str) {
  dump("Setting over-link but not waiting: " + str + "\n");

  let overLink = gURLBar._overLinkBox;
  let opacity = window.getComputedStyle(overLink, null).opacity;
  dump("Opacity is " + opacity + "\n");

  ok(str || opacity == 0,
     "Test assumption: either showing or if hiding, already hidden");
  ok(!str || opacity == 1,
     "Test assumption: either hiding or if showing, already shown");

  gURLBar.setOverLink(str);
}









function updateOverLink(str) {
  gURLBar._updateOverLink(str);
}








function ensureOverLinkHidden() {
  dump("Ensuring over-link is hidden" + "\n");

  let overLink = gURLBar._overLinkBox;
  let opacity = window.getComputedStyle(overLink, null).opacity;
  dump("Opacity is " + opacity + "\n");

  if (opacity == 0)
    return false;

  setOverLinkWait("");
  return true;
}







function hostLabelIs(str) {
  let host = gURLBar._overLinkHostLabel;
  is(host.value, str, "Over-link host label should be correct");
}







function pathLabelIs(str) {
  let path = gURLBar._overLinkPathLabel;
  is(path.value, str, "Over-link path label should be correct");
}
