









let menuitems = [], menupopups = [], huds = [], tabs = [];

function test()
{
  
  addTab("data:text/html,Web Console test for bug 602572: log bodies checkbox. tab 1");
  tabs.push(tab);

  browser.addEventListener("load", function(aEvent) {
    browser.removeEventListener(aEvent.type, arguments.callee, true);

    openConsole();

    
    addTab("data:text/html,Web Console test for bug 602572: log bodies checkbox. tab 2");
    tabs.push(tab);

    browser.addEventListener("load", function(aEvent) {
      browser.removeEventListener(aEvent.type, arguments.callee, true);

      openConsole();
      executeSoon(startTest);
    }, true);
  }, true);
}

function startTest()
{
  
  let win2 = tabs[1].linkedBrowser.contentWindow;
  let hudId2 = HUDService.getHudIdByWindow(win2);
  huds[1] = HUDService.hudReferences[hudId2];
  HUDService.disableAnimation(hudId2);

  menuitems[1] = huds[1].HUDBox.querySelector("menuitem[buttonType=saveBodies]");
  menupopups[1] = huds[1].HUDBox.querySelector("menupopup");

  
  menupopups[1].addEventListener("popupshown", onpopupshown2, false);
  menupopups[1].openPopup(huds[1].outputNode, "overlap", 10, 10, true, false);
}

function onpopupshown2(aEvent)
{
  menupopups[1].removeEventListener(aEvent.type, arguments.callee, false);

  
  isnot(menuitems[1].getAttribute("checked"), "true",
        "menuitems[1] is not checked");

  ok(!HUDService.saveRequestAndResponseBodies, "bodies are not logged");

  
  HUDService.saveRequestAndResponseBodies = true;

  menupopups[1].addEventListener("popuphidden", function(aEvent) {
    menupopups[1].removeEventListener(aEvent.type, arguments.callee, false);

    
    menupopups[1].addEventListener("popupshown", onpopupshown2b, false);
    menupopups[1].openPopup(huds[1].outputNode, "overlap", 11, 11, true, false);
  }, false);
  menupopups[1].hidePopup();
}

function onpopupshown2b(aEvent)
{
  menupopups[1].removeEventListener(aEvent.type, arguments.callee, false);
  is(menuitems[1].getAttribute("checked"), "true", "menuitems[1] is checked");

  menupopups[1].addEventListener("popuphidden", function(aEvent) {
    menupopups[1].removeEventListener(aEvent.type, arguments.callee, false);

    
    gBrowser.selectedTab = tabs[0];
    waitForFocus(function() {
      
      let win1 = tabs[0].linkedBrowser.contentWindow;
      let hudId1 = HUDService.getHudIdByWindow(win1);
      huds[0] = HUDService.hudReferences[hudId1];
      HUDService.disableAnimation(hudId1);

      menuitems[0] = huds[0].HUDBox.querySelector("menuitem[buttonType=saveBodies]");
      menupopups[0] = huds[0].HUDBox.querySelector("menupopup");

      menupopups[0].addEventListener("popupshown", onpopupshown1, false);
      menupopups[0].openPopup(huds[0].outputNode, "overlap", 12, 12, true, false);
    }, tabs[0].linkedBrowser.contentWindow);
  }, false);
  menupopups[1].hidePopup();
}

function onpopupshown1(aEvent)
{
  menupopups[0].removeEventListener(aEvent.type, arguments.callee, false);

  
  is(menuitems[0].getAttribute("checked"), "true", "menuitems[0] is checked");

  
  HUDService.saveRequestAndResponseBodies = false;

  
  menupopups[0].addEventListener("popuphidden", function(aEvent) {
    menupopups[0].removeEventListener(aEvent.type, arguments.callee, false);

    gBrowser.selectedTab = tabs[1];
    waitForFocus(function() {
      
      menupopups[1].addEventListener("popupshown", onpopupshown2c, false);
      menupopups[1].openPopup(huds[1].outputNode, "overlap", 13, 13, true, false);
    }, tabs[1].linkedBrowser.contentWindow);
  }, false);
  menupopups[0].hidePopup();
}

function onpopupshown2c(aEvent)
{
  menupopups[1].removeEventListener(aEvent.type, arguments.callee, false);

  isnot(menuitems[1].getAttribute("checked"), "true",
        "menuitems[1] is not checked");

  menupopups[1].addEventListener("popuphidden", function(aEvent) {
    menupopups[1].removeEventListener(aEvent.type, arguments.callee, false);

    
    huds = menuitems = menupopups = tabs = null;
    HUDService.deactivateHUDForContext(gBrowser.selectedTab);
    gBrowser.removeCurrentTab();
    executeSoon(finishTest);
  }, false);
  menupopups[1].hidePopup();
}
