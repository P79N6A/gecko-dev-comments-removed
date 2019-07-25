


var testPath = "http://mochi.test:8888/browser/docshell/test/navigation/";
var Ci = Components.interfaces;
var Cc = Components.classes;
var ctx = {};


function isActive(aWindow) {
  var docshell = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIWebNavigation)
                        .QueryInterface(Ci.nsIDocShell);
  return docshell.isActive;
}





function autoRemovedListener(aElem, aType, aCallback) {

  var elem = aElem;
  var type = aType;
  var callback = aCallback;

  function remover() {
    elem.removeEventListener(type, remover, true);
    executeSoon(callback);
  }

  return remover;
}




function frameLoadWaiter(aInitialWindow, aFinalCallback) {

  
  var curr = aInitialWindow;

  
  var waitQueue = [];

  
  var finalCallback = aFinalCallback;

  function frameLoadCallback() {

    
    for (var i = 0; i < curr.frames.length; ++i)
      waitQueue.push(curr.frames[i]);

    
    if (waitQueue.length >= 1) {
      curr = waitQueue.shift();
      if (curr.document.readyState == "complete")
        frameLoadCallback();
      else
        curr.addEventListener("load", autoRemovedListener(curr, "load", frameLoadCallback), true);
      return;
    }

    
    finalCallback();
  }

  return frameLoadCallback;
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
  ctx.tab1Browser.addEventListener("load",
                                   autoRemovedListener(ctx.tab1Browser, "load", step2),
                                   true);
}

function step2() {

  
  ok(isActive(ctx.tab0Window), "Tab 0 should still be active");
  ok(!isActive(ctx.tab1Window), "Tab 1 should not be active");

  
  gBrowser.selectedTab = ctx.tab1;

  
  ok(!isActive(ctx.tab0Window), "Tab 0 should be inactive");
  ok(isActive(ctx.tab1Window), "Tab 1 should be active");

  
  ctx.tab2 = gBrowser.addTab(testPath + "bug343515_pg2.html");
  ctx.tab2Browser = gBrowser.getBrowserForTab(ctx.tab2);
  ctx.tab2Window = ctx.tab2Browser.contentWindow;
  ctx.tab2Browser.addEventListener("load",
                                   autoRemovedListener(ctx.tab2Browser, "load",
                                                       frameLoadWaiter(ctx.tab2Window, step3)),
                                   true);
}

function step3() {

  
  ok(!isActive(ctx.tab0Window), "Tab 0 should be inactive");
  ok(isActive(ctx.tab1Window), "Tab 1 should be active");

  
  ok(!isActive(ctx.tab2Window), "Tab 2 should be inactive");
  is(ctx.tab2Window.frames.length, 2, "Tab 2 should have 2 iframes");
  ok(!isActive(ctx.tab2Window.frames[0]), "Tab2 iframe 0 should be inactive");
  ok(!isActive(ctx.tab2Window.frames[1]), "Tab2 iframe 1 should be inactive");

  
  ctx.tab2Window.location = testPath + "bug343515_pg3.html";
  ctx.tab2Browser.addEventListener("load",
                                  autoRemovedListener(ctx.tab2Browser, "load",
                                                      frameLoadWaiter(ctx.tab2Window, step4)),
                                  true);
}

function step4() {

  
  ok(!isActive(ctx.tab0Window), "Tab 0 should be inactive");
  ok(isActive(ctx.tab1Window), "Tab 1 should be active");

  
  ok(!isActive(ctx.tab2Window), "Tab 2 should be inactive");
  is(ctx.tab2Window.frames.length, 2, "Tab 2 should have 2 iframes");
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

  
  ctx.tab2Browser.addEventListener("pageshow",
                                   autoRemovedListener(ctx.tab2Browser, "pageshow",
                                                       frameLoadWaiter(ctx.tab2Window, step5)),
                                   true);
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
  ctx.tab1Browser.addEventListener("load",
                                   autoRemovedListener(ctx.tab1Browser, "load",
                                                       frameLoadWaiter(ctx.tab1Window, step6)),
                                   true);
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

  
  ctx.tab2Browser.addEventListener("pageshow",
                                   autoRemovedListener(ctx.tab2Browser, "pageshow",
                                                       frameLoadWaiter(ctx.tab2Window, step7)),
                                   true);
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

  
  gBrowser.removeCurrentTab();
  gBrowser.tabContainer.advanceSelectedTab(1, true);
  gBrowser.removeCurrentTab();

  
  finish();
}
