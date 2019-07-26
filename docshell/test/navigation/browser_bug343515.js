


var testPath = "http://mochi.test:8888/browser/docshell/test/navigation/";
var ctx = {};


function isActive(aWindow) {
  var docshell = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIWebNavigation)
                        .QueryInterface(Ci.nsIDocShell);
  return docshell.isActive;
}






function nShotsListener(aElem, aType, aCallback, aCount) {
  let count = aCount;
  aElem.addEventListener(aType, function listenerCallback() {
    if (--count == 0) {
      aElem.removeEventListener(aType, listenerCallback, true);

      
      
      executeSoon(aCallback);
    }
  }, true);
}

function oneShotListener(aElem, aType, aCallback) {
  nShotsListener(aElem, aType, aCallback, 1);
}


function test() {

  
  waitForExplicitFinish();

  
  step1();
}

function step1() {

  
  ctx.tab0 = gBrowser.selectedTab;
  ctx.tab0Browser = gBrowser.getBrowserForTab(ctx.tab0);
  ctx.tab0Window = ctx.tab0Browser.contentWindow;

  
  ok(isActive(ctx.tab0Window), "Tab 0 should be active at test start");

  
  ctx.tab1 = gBrowser.addTab(testPath + "bug343515_pg1.html");
  ctx.tab1Browser = gBrowser.getBrowserForTab(ctx.tab1);
  ctx.tab1Window = ctx.tab1Browser.contentWindow;
  oneShotListener(ctx.tab1Browser, "load", step2);
}

function step2() {
  is(testPath + "bug343515_pg1.html", ctx.tab1Browser.currentURI.spec,
     "Got expected tab 1 url in step 2");

  
  ok(isActive(ctx.tab0Window), "Tab 0 should still be active");
  ok(!isActive(ctx.tab1Window), "Tab 1 should not be active");

  
  gBrowser.selectedTab = ctx.tab1;

  
  ok(!isActive(ctx.tab0Window), "Tab 0 should be inactive");
  ok(isActive(ctx.tab1Window), "Tab 1 should be active");

  
  ctx.tab2 = gBrowser.addTab(testPath + "bug343515_pg2.html");
  ctx.tab2Browser = gBrowser.getBrowserForTab(ctx.tab2);
  ctx.tab2Window = ctx.tab2Browser.contentWindow;

  
  
  nShotsListener(ctx.tab2Browser, "load", step3, 3);
}

function step3() {
  is(testPath + "bug343515_pg2.html", ctx.tab2Browser.currentURI.spec,
     "Got expected tab 2 url in step 3");

  
  ok(!isActive(ctx.tab0Window), "Tab 0 should be inactive");
  ok(isActive(ctx.tab1Window), "Tab 1 should be active");

  
  ok(!isActive(ctx.tab2Window), "Tab 2 should be inactive");
  is(ctx.tab2Window.frames.length, 2, "Tab 2 should have 2 iframes");
  for (var i = 0; i < ctx.tab2Window.frames.length; i++)
    info("step 3, frame " + i + " info: " + ctx.tab2Window.frames[i].location);
  ok(!isActive(ctx.tab2Window.frames[0]), "Tab2 iframe 0 should be inactive");
  ok(!isActive(ctx.tab2Window.frames[1]), "Tab2 iframe 1 should be inactive");

  
  ctx.tab2Window.location = testPath + "bug343515_pg3.html";

  
  
  nShotsListener(ctx.tab2Browser, "load", step4, 4);
}

function step4() {
  is(testPath + "bug343515_pg3.html", ctx.tab2Browser.currentURI.spec,
     "Got expected tab 2 url in step 4");

  
  ok(!isActive(ctx.tab0Window), "Tab 0 should be inactive");
  ok(isActive(ctx.tab1Window), "Tab 1 should be active");

  
  ok(!isActive(ctx.tab2Window), "Tab 2 should be inactive");
  is(ctx.tab2Window.frames.length, 2, "Tab 2 should have 2 iframes");
  for (var i = 0; i < ctx.tab2Window.frames.length; i++)
    info("step 4, frame " + i + " info: " + ctx.tab2Window.frames[i].location);
  is(ctx.tab2Window.frames[0].frames.length, 1, "Tab 2 iframe 0 should have 1 iframes");
  ok(!isActive(ctx.tab2Window.frames[0]), "Tab2 iframe 0 should be inactive");
  ok(!isActive(ctx.tab2Window.frames[0].frames[0]), "Tab2 iframe 0 subiframe 0 should be inactive");
  ok(!isActive(ctx.tab2Window.frames[1]), "Tab2 iframe 1 should be inactive");

  
  gBrowser.selectedTab = ctx.tab2;

  
  ok(!isActive(ctx.tab0Window), "Tab 0 should be inactive");
  ok(!isActive(ctx.tab1Window), "Tab 1 should be inactive");
  ok(isActive(ctx.tab2Window), "Tab 2 should be active");
  ok(isActive(ctx.tab2Window.frames[0]), "Tab2 iframe 0 should be active");
  ok(isActive(ctx.tab2Window.frames[0].frames[0]), "Tab2 iframe 0 subiframe 0 should be active");
  ok(isActive(ctx.tab2Window.frames[1]), "Tab2 iframe 1 should be active");

  
  oneShotListener(ctx.tab2Browser, "pageshow", step5);
  ctx.tab2Browser.goBack();

}

function step5() {
  
  ok(!isActive(ctx.tab0Window), "Tab 0 should be inactive");
  ok(!isActive(ctx.tab1Window), "Tab 1 should be inactive");
  ok(isActive(ctx.tab2Window), "Tab 2 should be active");
  ok(isActive(ctx.tab2Window.frames[0]), "Tab2 iframe 0 should be active");
  ok(isActive(ctx.tab2Window.frames[1]), "Tab2 iframe 1 should be active");

  
  gBrowser.selectedTab = ctx.tab1;

  
  ctx.tab1Window.location = testPath + "bug343515_pg3.html";

  
  
  nShotsListener(ctx.tab1Browser, "load", step6, 4);
}

function step6() {

  
  ok(!isActive(ctx.tab0Window), "Tab 0 should be inactive");
  ok(isActive(ctx.tab1Window), "Tab 1 should be active");
  ok(isActive(ctx.tab1Window.frames[0]), "Tab1 iframe 0 should be active");
  ok(isActive(ctx.tab1Window.frames[0].frames[0]), "Tab1 iframe 0 subiframe 0 should be active");
  ok(isActive(ctx.tab1Window.frames[1]), "Tab1 iframe 1 should be active");
  ok(!isActive(ctx.tab2Window), "Tab 2 should be inactive");
  ok(!isActive(ctx.tab2Window.frames[0]), "Tab2 iframe 0 should be inactive");
  ok(!isActive(ctx.tab2Window.frames[1]), "Tab2 iframe 1 should be inactive");

  
  oneShotListener(ctx.tab2Browser, "pageshow", step7);
  var tab2docshell = ctx.tab2Window.QueryInterface(Ci.nsIInterfaceRequestor)
                                   .getInterface(Ci.nsIWebNavigation);
  tab2docshell.goForward();
}

function step7() {

  ctx.tab2Window = ctx.tab2Browser.contentWindow;

  
  ok(!isActive(ctx.tab0Window), "Tab 0 should be inactive");
  ok(isActive(ctx.tab1Window), "Tab 1 should be active");
  ok(isActive(ctx.tab1Window.frames[0]), "Tab1 iframe 0 should be active");
  ok(isActive(ctx.tab1Window.frames[0].frames[0]), "Tab1 iframe 0 subiframe 0 should be active");
  ok(isActive(ctx.tab1Window.frames[1]), "Tab1 iframe 1 should be active");
  ok(!isActive(ctx.tab2Window), "Tab 2 should be inactive");
  ok(!isActive(ctx.tab2Window.frames[0]), "Tab2 iframe 0 should be inactive");
  ok(!isActive(ctx.tab2Window.frames[0].frames[0]), "Tab2 iframe 0 subiframe 0 should be inactive");
  ok(!isActive(ctx.tab2Window.frames[1]), "Tab2 iframe 1 should be inactive");

  
  allDone();
}

function allDone() {

  
  gBrowser.removeTab(ctx.tab1);
  gBrowser.removeTab(ctx.tab2);

  
  finish();
}
