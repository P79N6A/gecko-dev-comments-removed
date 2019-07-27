# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

function gEMEListener(msg ) {
  let browser = msg.target;
  let notificationId = "drmContentPlaying";
  
  if (!Services.prefs.getBoolPref("browser.eme.ui.enabled") ||
      PopupNotifications.getNotification(notificationId, browser)) {
    return;
  }

  let msgId = "emeNotifications.drmContentPlaying.message";
  let brandName = document.getElementById("bundle_brand").getString("brandShortName");
  let message = gNavigatorBundle.getFormattedString(msgId, [msg.data.drmProvider, brandName]);
  let anchorId = "eme-notification-icon";

  let mainAction = {
    label: gNavigatorBundle.getString("emeNotifications.drmContentPlaying.button.label"),
    accessKey: gNavigatorBundle.getString("emeNotifications.drmContentPlaying.button.accesskey"),
    callback: function() { openPreferences("paneContent"); },
    dismiss: true
  };
  let options = {
    dismissed: true,
    eventCallback: aTopic => aTopic == "swapping",
  };
  PopupNotifications.show(browser, notificationId, message, anchorId, mainAction, null, options);
};

window.messageManager.addMessageListener("EMEVideo:MetadataLoaded", gEMEListener);
window.addEventListener("unload", function() {
  window.messageManager.removeMessageListener("EMEVideo:MetadataLoaded", gEMEListener);
}, false);
