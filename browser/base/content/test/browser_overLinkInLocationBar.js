









































var gTestIter;



function smokeTestGenerator() {
  if (ensureOverLinkHidden())
    yield;

  setOverLink("http://example.com/");
  yield;
  checkURLBar(true);

  setOverLink("");
  yield;
  checkURLBar(false);
}

function test() {
  waitForExplicitFinish();
  gTestIter = smokeTestGenerator();
  cont();
}







function cont() {
  try {
    gTestIter.next();
  }
  catch (err if err instanceof StopIteration) {
    finish();
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









function setOverLink(aStr) {
  let overLink = gURLBar._overLinkBox;
  overLink.addEventListener("transitionend", function onTrans(event) {
    if (event.target == overLink && event.propertyName == "opacity") {
      overLink.removeEventListener("transitionend", onTrans, false);
      cont();
    }
  }, false);
  gURLBar.setOverLink(aStr);
}








function ensureOverLinkHidden() {
  let overLink = gURLBar._overLinkBox;
  if (window.getComputedStyle(overLink, null).opacity == 0)
    return false;

  setOverLink("");
  return true;
}
