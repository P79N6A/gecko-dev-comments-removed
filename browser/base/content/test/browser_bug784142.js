const windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);

function test()
{
  waitForExplicitFinish();

  
  Services.prefs.setBoolPref("prompts.tab_modal.enabled", false);

  windowMediator.addListener(promptListener);

  
  
  var script = "window.open('data:text/html,<button id=\"button\" onclick=\"window.close(); alert(5);\">Close</button>', null, 'width=200,height=200');";
  gBrowser.selectedTab = 
    gBrowser.addTab("data:text/html,<body onload='setTimeout(dotest, 0)'><script>function dotest() { " + script + "}</script></body>");
}

function windowOpened(win)
{
  
  waitForFocus(clickButton, win.content);
}

function clickButton(win)
{
  
  
  promptListener.window = win;

  
  EventUtils.synthesizeMouseAtCenter(win.content.document.getElementById("button"), { }, win);

  windowMediator.removeListener(promptListener);
  gBrowser.removeTab(gBrowser.selectedTab);

  is(promptListener.message, "window appeared", "modal prompt closer didn't crash");
  Services.prefs.clearUserPref("prompts.tab_modal.enabled", false);
  finish();
}

var promptListener = {
  onWindowTitleChange: function () {},
  onOpenWindow: function (win) {
    let domWin = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);
    if (!promptListener.window) {
      
      waitForFocus(windowOpened, domWin);
    }
    else {
      
      ok(promptListener.window.closed, "window has closed");

      
      
      promptListener.message = "window appeared";
      executeSoon(function () { domWin.close() });
    }
  },
  onCloseWindow: function () {}
};
