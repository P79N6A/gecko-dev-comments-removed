




let gActiveListeners = {};

function registerPopupEventHandler(eventName, callback) {
  gActiveListeners[eventName] = function (event) {
    if (event.target != PopupNotifications.panel)
      return;
    PopupNotifications.panel.removeEventListener(eventName,
                                                 gActiveListeners[eventName],
                                                 false);
    delete gActiveListeners[eventName];

    callback.call(PopupNotifications.panel);
  }
  PopupNotifications.panel.addEventListener(eventName,
                                            gActiveListeners[eventName],
                                            false);
}

function unregisterPopupEventHandler(eventName)
{
  PopupNotifications.panel.removeEventListener(eventName,
                                               gActiveListeners[eventName],
                                               false);
  delete gActiveListeners[eventName];
}

function unregisterAllPopupEventHandlers()
{
  for (let eventName in gActiveListeners) {
    PopupNotifications.panel.removeEventListener(eventName,
                                                 gActiveListeners[eventName],
                                                 false);
  }
  gActiveListeners = {};
}

function triggerMainCommand(popup)
{
  info("triggering main command");
  let notifications = popup.childNodes;
  ok(notifications.length > 0, "at least one notification displayed");
  let notification = notifications[0];
  info("triggering command: " + notification.getAttribute("buttonlabel"));

  
  EventUtils.synthesizeMouse(notification.button, 20, 10, {});
}

function triggerSecondaryCommand(popup, index)
{
  info("triggering secondary command, " + index);
  let notifications = popup.childNodes;
  ok(notifications.length > 0, "at least one notification displayed");
  let notification = notifications[0];

  
  
  document.getAnonymousNodes(popup)[0].style.transition = "none";

  notification.button.focus();

  popup.addEventListener("popupshown", function () {
    popup.removeEventListener("popupshown", arguments.callee, false);

    
    for (let i = 0; i <= index; i++)
      EventUtils.synthesizeKey("VK_DOWN", {});

    
    EventUtils.synthesizeKey("VK_ENTER", {});
  }, false);

  
  EventUtils.synthesizeKey("VK_DOWN", { altKey: (navigator.platform.indexOf("Mac") == -1) });
}

function dismissNotification(popup)
{
  info("dismissing notification");
  executeSoon(function () {
    EventUtils.synthesizeKey("VK_ESCAPE", {});
  });
}

function setFinishedCallback(callback, win)
{
  if (!win) {
    win = window;
  }
  let testPage = win.gBrowser.selectedBrowser.contentWindow.wrappedJSObject;
  testPage.testFinishedCallback = function(result, exception) {
    setTimeout(function() {
      info("got finished callback");
      callback(result, exception);
    }, 0);
  }
}

function dispatchEvent(eventName)
{
  info("dispatching event: " + eventName);
  let event = document.createEvent("Events");
  event.initEvent(eventName, false, false);
  gBrowser.selectedBrowser.contentWindow.dispatchEvent(event);
}

function setPermission(url, permission, value)
{
  const nsIPermissionManager = Components.interfaces.nsIPermissionManager;

  switch (value) {
    case "allow":
      value = nsIPermissionManager.ALLOW_ACTION;
      break;
    case "deny":
      value = nsIPermissionManager.DENY_ACTION;
      break;
    case "unknown":
      value = nsIPermissionManager.UNKNOWN_ACTION;
      break;
    default:
      throw new Error("No idea what to set here!");
  }

  let uri = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService)
                      .newURI(url, null, null);
  let principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                    .getService(Ci.nsIScriptSecurityManager)
                    .getNoAppCodebasePrincipal(uri);

  Components.classes["@mozilla.org/permissionmanager;1"]
            .getService(Components.interfaces.nsIPermissionManager)
            .addFromPrincipal(principal, permission, value);
}

function removePermission(url, permission)
{
  let uri = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService)
                      .newURI(url, null, null);
  let principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                    .getService(Ci.nsIScriptSecurityManager)
                    .getNoAppCodebasePrincipal(uri);

  Components.classes["@mozilla.org/permissionmanager;1"]
            .getService(Components.interfaces.nsIPermissionManager)
            .removeFromPrincipal(principal, permission);
}

function getPermission(url, permission)
{
  let uri = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService)
                      .newURI(url, null, null);
  let principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                    .getService(Ci.nsIScriptSecurityManager)
                    .getNoAppCodebasePrincipal(uri);

  return Components.classes["@mozilla.org/permissionmanager;1"]
                   .getService(Components.interfaces.nsIPermissionManager)
                   .testPermissionFromPrincipal(principal, permission);
}
