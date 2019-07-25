









































var gTestIter;



let gTests = [

  function smokeTestGenerator() {
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
    setOverLink("http://example.com/");
    hostLabelIs("http://example.com/");
    pathLabelIs("");

    setOverLink("http://example.com/foo");
    hostLabelIs("http://example.com/");
    pathLabelIs("foo");

    setOverLink("javascript:popup('http://example.com/')");
    hostLabelIs("");
    pathLabelIs("javascript:popup('http://example.com/')");

    setOverLink("javascript:popup('http://example.com/foo')");
    hostLabelIs("");
    pathLabelIs("javascript:popup('http://example.com/foo')");

    setOverLink("about:home");
    hostLabelIs("");
    pathLabelIs("about:home");

    
    if (ensureOverLinkHidden())
      yield;
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
  let overLink = gURLBar._overLinkBox;
  overLink.addEventListener("transitionend", function onTrans(event) {
    if (event.target == overLink && event.propertyName == "opacity") {
      overLink.removeEventListener("transitionend", onTrans, false);
      cont();
    }
  }, false);
  gURLBar.setOverLink(str);
}








function setOverLink(str) {
  gURLBar.setOverLink(str);
}








function ensureOverLinkHidden() {
  let overLink = gURLBar._overLinkBox;
  if (window.getComputedStyle(overLink, null).opacity == 0) {
    setOverLink("");
    return false;
  }

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
