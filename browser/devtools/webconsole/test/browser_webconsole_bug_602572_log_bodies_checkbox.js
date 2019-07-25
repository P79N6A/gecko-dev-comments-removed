









let menuitems = [], menupopups = [], huds = [], tabs = [];

function test()
{
  
  addTab("data:text/html;charset=utf-8,Web Console test for bug 602572: log bodies checkbox. tab 1");
  tabs.push(tab);

  browser.addEventListener("load", function onLoad1(aEvent) {
    browser.removeEventListener(aEvent.type, onLoad1, true);

    openConsole(null, function(aHud) {
      info("iframe1 height " + aHud.iframe.clientHeight);
      info("iframe1 root height " + aHud.ui.rootElement.clientHeight);

      
      addTab("data:text/html;charset=utf-8,Web Console test for bug 602572: log bodies checkbox. tab 2");
      tabs.push(tab);

      browser.addEventListener("load", function onLoad2(aEvent) {
        browser.removeEventListener(aEvent.type, onLoad2, true);

        openConsole(null, function(aHud) {
          info("iframe2 height " + aHud.iframe.clientHeight);
          info("iframe2 root height " + aHud.ui.rootElement.clientHeight);
          waitForFocus(startTest, aHud.iframeWindow);
        });
      }, true);
    });
  }, true);
}

function startTest()
{
  
  let win2 = tabs[1].linkedBrowser.contentWindow;
  let hudId2 = HUDService.getHudIdByWindow(win2);
  huds[1] = HUDService.hudReferences[hudId2];
  HUDService.disableAnimation(hudId2);

  menuitems[1] = huds[1].ui.rootElement.querySelector("#saveBodies");
  menupopups[1] = huds[1].ui.rootElement.querySelector("menupopup");

  
  menupopups[1].addEventListener("popupshown", onpopupshown2, false);
  executeSoon(function() {
    menupopups[1].openPopup();
  });
}

function onpopupshown2(aEvent)
{
  menupopups[1].removeEventListener(aEvent.type, onpopupshown2, false);

  
  isnot(menuitems[1].getAttribute("checked"), "true",
        "menuitems[1] is not checked");

  ok(!huds[1].ui.saveRequestAndResponseBodies, "bodies are not logged");

  
  huds[1].ui.saveRequestAndResponseBodies = true;

  menupopups[1].addEventListener("popuphidden", function _onhidden(aEvent) {
    menupopups[1].removeEventListener(aEvent.type, _onhidden, false);

    
    menupopups[1].addEventListener("popupshown", onpopupshown2b, false);
    executeSoon(function() {
      menupopups[1].openPopup();
    });
  }, false);

  executeSoon(function() {
    menupopups[1].hidePopup();
  });
}

function onpopupshown2b(aEvent)
{
  menupopups[1].removeEventListener(aEvent.type, onpopupshown2b, false);
  is(menuitems[1].getAttribute("checked"), "true", "menuitems[1] is checked");

  menupopups[1].addEventListener("popuphidden", function _onhidden(aEvent) {
    menupopups[1].removeEventListener(aEvent.type, _onhidden, false);

    
    gBrowser.selectedTab = tabs[0];
    waitForFocus(function() {
      
      let win1 = tabs[0].linkedBrowser.contentWindow;
      let hudId1 = HUDService.getHudIdByWindow(win1);
      huds[0] = HUDService.hudReferences[hudId1];
      HUDService.disableAnimation(hudId1);

      info("iframe1 height " + huds[0].iframe.clientHeight);
      info("iframe1 root height " + huds[0].ui.rootElement.clientHeight);

      menuitems[0] = huds[0].ui.rootElement.querySelector("#saveBodies");
      menupopups[0] = huds[0].ui.rootElement.querySelector("menupopup");

      menupopups[0].addEventListener("popupshown", onpopupshown1, false);
      menupopups[0].openPopup();
    }, tabs[0].linkedBrowser.contentWindow);
  }, false);

  executeSoon(function() {
    menupopups[1].hidePopup();
  });
}

function onpopupshown1(aEvent)
{
  menupopups[0].removeEventListener(aEvent.type, onpopupshown1, false);

  
  isnot(menuitems[0].getAttribute("checked"), "true",
        "menuitems[0] is not checked");

  
  huds[0].ui.saveRequestAndResponseBodies = true;

  
  menupopups[0].addEventListener("popuphidden", function _onhidden(aEvent) {
    menupopups[0].removeEventListener(aEvent.type, _onhidden, false);

    gBrowser.selectedTab = tabs[1];
    waitForFocus(function() {
      
      menupopups[1].addEventListener("popupshown", onpopupshown2c, false);
      menupopups[1].openPopup();
    }, tabs[1].linkedBrowser.contentWindow);
  }, false);

  executeSoon(function() {
    menupopups[0].hidePopup();
  });
}

function onpopupshown2c(aEvent)
{
  menupopups[1].removeEventListener(aEvent.type, onpopupshown2c, false);

  is(menuitems[1].getAttribute("checked"), "true", "menuitems[1] is checked");

  menupopups[1].addEventListener("popuphidden", function _onhidden(aEvent) {
    menupopups[1].removeEventListener(aEvent.type, _onhidden, false);

    
    huds = menuitems = menupopups = tabs = null;
    closeConsole(gBrowser.selectedTab, function() {
      gBrowser.removeCurrentTab();
      executeSoon(finishTest);
    });
  }, false);

  executeSoon(function() {
    menupopups[1].hidePopup();
  });
}
