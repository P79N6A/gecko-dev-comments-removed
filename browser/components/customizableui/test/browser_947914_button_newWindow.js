



"use strict";

add_task(function() {
  info("Check new window button existence and functionality");
  yield PanelUI.show();

  var windowWasHandled = false;
  var newWindow = null;

  let observerWindowOpened = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == "domwindowopened") {
        newWindow = aSubject.QueryInterface(Components.interfaces.nsIDOMWindow);
        newWindow.addEventListener("load", function newWindowHandler() {
          newWindow.removeEventListener("load", newWindowHandler, false);
          is(newWindow.location.href, "chrome://browser/content/browser.xul",
                                "A new browser window was opened");
          windowWasHandled = true;
        }, false);
      }
    }
  }

  Services.ww.registerNotification(observerWindowOpened);

  let newWindowButton = document.getElementById("new-window-button");
  ok(newWindowButton, "New Window button exists in Panel Menu");
  newWindowButton.click();

  try{
    yield waitForCondition(() => windowWasHandled);
    yield promiseWindowClosed(newWindow);
  }
  catch(e) {
    ok(false, "The new browser window was not properly handled");
  }
  finally {
    Services.ww.unregisterNotification(observerWindowOpened);
  }
});
