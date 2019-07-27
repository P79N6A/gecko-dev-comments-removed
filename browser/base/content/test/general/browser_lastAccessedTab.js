




const CURRENT_TIME_TOLERANCE_MS = 15;

function isCurrent(tab, msg) {
  const DIFF = Math.abs(Date.now() - tab.lastAccessed);
  ok(DIFF <= CURRENT_TIME_TOLERANCE_MS, msg + " (difference: " + DIFF + ")");
}

function nextStep(fn) {
  setTimeout(fn, CURRENT_TIME_TOLERANCE_MS + 10);
}

let originalTab;
let newTab;

function test() {
  waitForExplicitFinish();

  originalTab = gBrowser.selectedTab;
  nextStep(step2);
}

function step2() {
  isCurrent(originalTab, "selected tab has the current timestamp");
  newTab = gBrowser.addTab("about:blank", {skipAnimation: true});
  nextStep(step3);
}

function step3() {
  ok(newTab.lastAccessed < Date.now(), "new tab hasn't been selected so far");
  gBrowser.selectedTab = newTab;
  isCurrent(newTab, "new tab has the current timestamp after being selected");
  nextStep(step4);
}

function step4() {
  ok(originalTab.lastAccessed < Date.now(),
     "original tab has old timestamp after being deselected");
  isCurrent(newTab, "new tab has the current timestamp since it's still selected");

  gBrowser.removeTab(newTab);
  finish();
}
