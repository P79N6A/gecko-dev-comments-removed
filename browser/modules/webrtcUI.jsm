



"use strict";

this.EXPORTED_SYMBOLS = ["webrtcUI"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "MediaManagerService",
                                   "@mozilla.org/mediaManagerService;1",
                                   "nsIMediaManagerService");

this.webrtcUI = {
  init: function () {
    Services.obs.addObserver(handleRequest, "getUserMedia:request", false);
    Services.obs.addObserver(updateIndicators, "recording-device-events", false);
    Services.obs.addObserver(removeBrowserSpecificIndicator, "recording-window-ended", false);
  },

  uninit: function () {
    Services.obs.removeObserver(handleRequest, "getUserMedia:request");
    Services.obs.removeObserver(updateIndicators, "recording-device-events");
    Services.obs.removeObserver(removeBrowserSpecificIndicator, "recording-window-ended");
  },

  showGlobalIndicator: false,
  showCameraIndicator: false,
  showMicrophoneIndicator: false,
  showScreenSharingIndicator: "", 

  get activeStreams() {
    let contentWindowSupportsArray = MediaManagerService.activeMediaCaptureWindows;
    let count = contentWindowSupportsArray.Count();
    let activeStreams = [];
    for (let i = 0; i < count; i++) {
      let contentWindow = contentWindowSupportsArray.GetElementAt(i);
      let browser = getBrowserForWindow(contentWindow);
      let browserWindow = browser.ownerDocument.defaultView;
      let tab = browserWindow.gBrowser &&
                browserWindow.gBrowser._getTabForContentWindow(contentWindow.top);
      activeStreams.push({
        uri: contentWindow.location.href,
        tab: tab,
        browser: browser
      });
    }
    return activeStreams;
  },

  updateMainActionLabel: function(aMenuList) {
    let type = aMenuList.selectedItem.getAttribute("devicetype");
    let document = aMenuList.ownerDocument;
    document.getElementById("webRTC-all-windows-shared").hidden = type != "Screen";

    
    
    if (!document.getElementById("webRTC-selectMicrophone").hidden)
      type = "";

    let bundle = document.defaultView.gNavigatorBundle;
    let stringId = "getUserMedia.share" + (type || "SelectedItems") + ".label";
    let popupnotification = aMenuList.parentNode.parentNode;
    popupnotification.setAttribute("buttonlabel", bundle.getString(stringId));
  }
}

function getBrowserForWindowId(aWindowID) {
  return getBrowserForWindow(Services.wm.getOuterWindowWithId(aWindowID));
}

function getBrowserForWindow(aContentWindow) {
  return aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIWebNavigation)
                       .QueryInterface(Ci.nsIDocShell)
                       .chromeEventHandler;
}

function handleRequest(aSubject, aTopic, aData) {
  let constraints = aSubject.getConstraints();
  let secure = aSubject.isSecure;
  let contentWindow = Services.wm.getOuterWindowWithId(aSubject.windowID);

  contentWindow.navigator.mozGetUserMediaDevices(
    constraints,
    function (devices) {
      prompt(contentWindow, aSubject.callID, constraints.audio,
             constraints.video || constraints.picture, devices, secure);
    },
    function (error) {
      
      
      denyRequest(aSubject.callID, error);
    },
    aSubject.innerWindowID);
}

function denyRequest(aCallID, aError) {
  let msg = null;
  if (aError) {
    msg = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
    msg.data = aError;
  }
  Services.obs.notifyObservers(msg, "getUserMedia:response:deny", aCallID);
}

function prompt(aContentWindow, aCallID, aAudio, aVideo, aDevices, aSecure) {
  let audioDevices = [];
  let videoDevices = [];

  
  let sharingScreen = aVideo && typeof(aVideo) != "boolean" &&
                      aVideo.mediaSource != "camera";
  for (let device of aDevices) {
    device = device.QueryInterface(Ci.nsIMediaDevice);
    switch (device.type) {
      case "audio":
        if (aAudio)
          audioDevices.push(device);
        break;
      case "video":
        
        
        if (aVideo && (device.mediaSource == "camera") != sharingScreen)
          videoDevices.push(device);
        break;
    }
  }

  let requestTypes = [];
  if (videoDevices.length)
    requestTypes.push(sharingScreen ? "Screen" : "Camera");
  if (audioDevices.length)
    requestTypes.push("Microphone");

  if (!requestTypes.length) {
    denyRequest(aCallID, "NO_DEVICES_FOUND");
    return;
  }

  let uri = aContentWindow.document.documentURIObject;
  let browser = getBrowserForWindow(aContentWindow);
  let chromeDoc = browser.ownerDocument;
  let chromeWin = chromeDoc.defaultView;
  let stringBundle = chromeWin.gNavigatorBundle;
  let stringId = "getUserMedia.share" + requestTypes.join("And") + ".message";
  let message = stringBundle.getFormattedString(stringId, [uri.host]);

  let mainLabel;
  if (sharingScreen) {
    mainLabel = stringBundle.getString("getUserMedia.shareSelectedItems.label");
  }
  else {
    let string = stringBundle.getString("getUserMedia.shareSelectedDevices.label");
    mainLabel = PluralForm.get(requestTypes.length, string);
  }
  let mainAction = {
    label: mainLabel,
    accessKey: stringBundle.getString("getUserMedia.shareSelectedDevices.accesskey"),
    
    
    
    callback: function() {}
  };

  let secondaryActions = [
    {
      label: stringBundle.getString("getUserMedia.denyRequest.label"),
      accessKey: stringBundle.getString("getUserMedia.denyRequest.accesskey"),
      callback: function () {
        denyRequest(aCallID);
      }
    }
  ];

  if (!sharingScreen) { 
    secondaryActions.push({
      label: stringBundle.getString("getUserMedia.never.label"),
      accessKey: stringBundle.getString("getUserMedia.never.accesskey"),
      callback: function () {
        denyRequest(aCallID);
        
        
        let perms = Services.perms;
        if (audioDevices.length)
          perms.add(uri, "microphone", perms.DENY_ACTION);
        if (videoDevices.length)
          perms.add(uri, "camera", perms.DENY_ACTION);
      }
    });
  }

  if (aSecure && !sharingScreen) {
    
    
    
    secondaryActions.unshift({
      label: stringBundle.getString("getUserMedia.always.label"),
      accessKey: stringBundle.getString("getUserMedia.always.accesskey"),
      callback: function () {
        mainAction.callback(true);
      }
    });
  }

  let options = {
    eventCallback: function(aTopic, aNewBrowser) {
      if (aTopic == "swapping")
        return true;

      let chromeDoc = this.browser.ownerDocument;

      if (aTopic == "shown") {
        let PopupNotifications = chromeDoc.defaultView.PopupNotifications;
        let popupId = "Devices";
        if (requestTypes.length == 1 && requestTypes[0] == "Microphone")
          popupId = "Microphone";
        if (requestTypes.indexOf("Screen") != -1)
          popupId = "Screen";
        PopupNotifications.panel.firstChild.setAttribute("popupid", "webRTC-share" + popupId);
      }

      if (aTopic != "showing")
        return false;

      
      
      
      if (aSecure) {
        let perms = Services.perms;

        let micPerm = perms.testExactPermission(uri, "microphone");
        if (micPerm == perms.PROMPT_ACTION)
          micPerm = perms.UNKNOWN_ACTION;

        let camPerm = perms.testExactPermission(uri, "camera");
        if (camPerm == perms.PROMPT_ACTION)
          camPerm = perms.UNKNOWN_ACTION;

        
        if (videoDevices.length && sharingScreen)
          camPerm = perms.UNKNOWN_ACTION;

        
        
        
        
        if ((!audioDevices.length || micPerm) && (!videoDevices.length || camPerm)) {
          
          let allowedDevices = Cc["@mozilla.org/supports-array;1"]
                                 .createInstance(Ci.nsISupportsArray);
          if (videoDevices.length && camPerm == perms.ALLOW_ACTION)
            allowedDevices.AppendElement(videoDevices[0]);
          if (audioDevices.length && micPerm == perms.ALLOW_ACTION)
            allowedDevices.AppendElement(audioDevices[0]);
          Services.obs.notifyObservers(allowedDevices, "getUserMedia:response:allow", aCallID);
          this.remove();
          return true;
        }
      }

      function listDevices(menupopup, devices) {
        while (menupopup.lastChild)
          menupopup.removeChild(menupopup.lastChild);

        let deviceIndex = 0;
        for (let device of devices) {
          addDeviceToList(menupopup, device.name, deviceIndex);
          deviceIndex++;
        }
      }

      function listScreenShareDevices(menupopup, devices) {
        while (menupopup.lastChild)
          menupopup.removeChild(menupopup.lastChild);

        
        
        addDeviceToList(menupopup,
                        stringBundle.getString("getUserMedia.noWindowOrScreen.label"),
                        "-1");

        
        for (let i = 0; i < devices.length; ++i) {
          if (devices[i].mediaSource == "screen") {
            menupopup.appendChild(chromeDoc.createElement("menuseparator"));
            addDeviceToList(menupopup,
                            stringBundle.getString("getUserMedia.shareEntireScreen.label"),
                            i, "Screen");
            break;
          }
        }

        
        let separatorNeeded = true;
        for (let i = 0; i < devices.length; ++i) {
          if (devices[i].mediaSource == "window") {
            if (separatorNeeded) {
              menupopup.appendChild(chromeDoc.createElement("menuseparator"));
              separatorNeeded = false;
            }
            addDeviceToList(menupopup, devices[i].name, i, "Window");
          }
        }

        
        chromeDoc.getElementById("webRTC-selectWindow-menulist").removeAttribute("value");
        chromeDoc.getElementById("webRTC-all-windows-shared").hidden = true;
      }

      function addDeviceToList(menupopup, deviceName, deviceIndex, type) {
        let menuitem = chromeDoc.createElement("menuitem");
        menuitem.setAttribute("value", deviceIndex);
        menuitem.setAttribute("label", deviceName);
        menuitem.setAttribute("tooltiptext", deviceName);
        if (type)
          menuitem.setAttribute("devicetype", type);
        menupopup.appendChild(menuitem);
      }

      chromeDoc.getElementById("webRTC-selectCamera").hidden = !videoDevices.length || sharingScreen;
      chromeDoc.getElementById("webRTC-selectWindowOrScreen").hidden = !sharingScreen || !videoDevices.length;
      chromeDoc.getElementById("webRTC-selectMicrophone").hidden = !audioDevices.length;

      let camMenupopup = chromeDoc.getElementById("webRTC-selectCamera-menupopup");
      let windowMenupopup = chromeDoc.getElementById("webRTC-selectWindow-menupopup");
      let micMenupopup = chromeDoc.getElementById("webRTC-selectMicrophone-menupopup");
      if (sharingScreen)
        listScreenShareDevices(windowMenupopup, videoDevices);
      else
        listDevices(camMenupopup, videoDevices);
      listDevices(micMenupopup, audioDevices);
      if (requestTypes.length == 2) {
        let stringBundle = chromeDoc.defaultView.gNavigatorBundle;
        if (!sharingScreen)
          addDeviceToList(camMenupopup, stringBundle.getString("getUserMedia.noVideo.label"), "-1");
        addDeviceToList(micMenupopup, stringBundle.getString("getUserMedia.noAudio.label"), "-1");
      }

      this.mainAction.callback = function(aRemember) {
        let allowedDevices = Cc["@mozilla.org/supports-array;1"]
                               .createInstance(Ci.nsISupportsArray);
        let perms = Services.perms;
        if (videoDevices.length) {
          let listId = "webRTC-select" + (sharingScreen ? "Window" : "Camera") + "-menulist";
          let videoDeviceIndex = chromeDoc.getElementById(listId).value;
          let allowCamera = videoDeviceIndex != "-1";
          if (allowCamera)
            allowedDevices.AppendElement(videoDevices[videoDeviceIndex]);
          if (aRemember) {
            perms.add(uri, "camera",
                      allowCamera ? perms.ALLOW_ACTION : perms.DENY_ACTION);
          }
        }
        if (audioDevices.length) {
          let audioDeviceIndex = chromeDoc.getElementById("webRTC-selectMicrophone-menulist").value;
          let allowMic = audioDeviceIndex != "-1";
          if (allowMic)
            allowedDevices.AppendElement(audioDevices[audioDeviceIndex]);
          if (aRemember) {
            perms.add(uri, "microphone",
                      allowMic ? perms.ALLOW_ACTION : perms.DENY_ACTION);
          }
        }

        if (allowedDevices.Count() == 0) {
          denyRequest(aCallID);
          return;
        }

        Services.obs.notifyObservers(allowedDevices, "getUserMedia:response:allow", aCallID);
      };
      return false;
    }
  };

  let anchorId = "webRTC-shareDevices-notification-icon";
  if (requestTypes.length == 1 && requestTypes[0] == "Microphone")
    anchorId = "webRTC-shareMicrophone-notification-icon";
  if (requestTypes.indexOf("Screen") != -1)
    anchorId = "webRTC-shareScreen-notification-icon";
  chromeWin.PopupNotifications.show(browser, "webRTC-shareDevices", message,
                                    anchorId, mainAction, secondaryActions, options);
}

function updateIndicators() {
  let contentWindowSupportsArray = MediaManagerService.activeMediaCaptureWindows;
  let count = contentWindowSupportsArray.Count();

  webrtcUI.showGlobalIndicator = count > 0;

  let e = Services.wm.getEnumerator("navigator:browser");
  while (e.hasMoreElements())
    e.getNext().WebrtcIndicator.updateButton();

  webrtcUI.showCameraIndicator = false;
  webrtcUI.showMicrophoneIndicator = false;
  webrtcUI.showScreenSharingIndicator = "";

  for (let i = 0; i < count; ++i) {
    let contentWindow = contentWindowSupportsArray.GetElementAt(i);
    let camera = {}, microphone = {}, screen = {}, window = {};
    MediaManagerService.mediaCaptureWindowState(contentWindow, camera,
                                                microphone, screen, window);
    if (camera.value)
      webrtcUI.showCameraIndicator = true;
    if (microphone.value)
      webrtcUI.showMicrophoneIndicator = true;
    if (screen.value)
      webrtcUI.showScreenSharingIndicator = "Screen";
    else if (window.value && !webrtcUI.showScreenSharingIndicator)
      webrtcUI.showScreenSharingIndicator = "Window";

    showBrowserSpecificIndicator(getBrowserForWindow(contentWindow));
  }
}

function showBrowserSpecificIndicator(aBrowser) {
  let camera = {}, microphone = {}, screen = {}, window = {};
  MediaManagerService.mediaCaptureWindowState(aBrowser.contentWindow,
                                              camera, microphone, screen, window);
  let captureState;
  if (camera.value && microphone.value) {
    captureState = "CameraAndMicrophone";
  } else if (camera.value) {
    captureState = "Camera";
  } else if (microphone.value) {
    captureState = "Microphone";
  } else if (!screen.value && !window.value) {
    Cu.reportError("showBrowserSpecificIndicator: got neither video nor audio access");
    return;
  }

  let chromeWin = aBrowser.ownerDocument.defaultView;
  let stringBundle = chromeWin.gNavigatorBundle;

  let uri = aBrowser.contentWindow.document.documentURIObject;
  let windowId = aBrowser.contentWindow
                         .QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils)
                         .currentInnerWindowID;
  let mainAction = {
    label: stringBundle.getString("getUserMedia.continueSharing.label"),
    accessKey: stringBundle.getString("getUserMedia.continueSharing.accesskey"),
    callback: function () {},
    dismiss: true
  };
  let secondaryActions = [{
    label: stringBundle.getString("getUserMedia.stopSharing.label"),
    accessKey: stringBundle.getString("getUserMedia.stopSharing.accesskey"),
    callback: function () {
      let perms = Services.perms;
      if (camera.value &&
          perms.testExactPermission(uri, "camera") == perms.ALLOW_ACTION)
        perms.remove(uri.host, "camera");
      if (microphone.value &&
          perms.testExactPermission(uri, "microphone") == perms.ALLOW_ACTION)
        perms.remove(uri.host, "microphone");

      Services.obs.notifyObservers(null, "getUserMedia:revoke", windowId);

      
      
      let outerWindowID = Services.wm.getCurrentInnerWindowWithId(windowId)
                                     .QueryInterface(Ci.nsIInterfaceRequestor)
                                     .getInterface(Ci.nsIDOMWindowUtils)
                                     .outerWindowID;
      removeBrowserSpecificIndicator(null, null, outerWindowID);
    }
  }];
  let options = {
    hideNotNow: true,
    dismissed: true,
    eventCallback: function(aTopic) {
      if (aTopic == "shown") {
        let PopupNotifications = this.browser.ownerDocument.defaultView.PopupNotifications;
        let popupId = captureState == "Microphone" ? "Microphone" : "Devices";
        PopupNotifications.panel.firstChild.setAttribute("popupid", "webRTC-sharing" + popupId);
      }
      return aTopic == "swapping";
    }
  };
  if (captureState) {
    let anchorId = captureState == "Microphone" ? "webRTC-sharingMicrophone-notification-icon"
                                                : "webRTC-sharingDevices-notification-icon";
    let message = stringBundle.getString("getUserMedia.sharing" + captureState + ".message2");
    chromeWin.PopupNotifications.show(aBrowser, "webRTC-sharingDevices", message,
                                      anchorId, mainAction, secondaryActions, options);
  }

  
  if (!screen.value && !window.value)
    return;

  options = {
    hideNotNow: true,
    dismissed: true,
    eventCallback: function(aTopic) {
      if (aTopic == "shown") {
        let PopupNotifications = this.browser.ownerDocument.defaultView.PopupNotifications;
        PopupNotifications.panel.firstChild.setAttribute("popupid", "webRTC-sharingScreen");
      }
      return aTopic == "swapping";
    }
  };
  
  let stringId = "getUserMedia.sharing" + (screen.value ? "Screen" : "Window") + ".message";
  chromeWin.PopupNotifications.show(aBrowser, "webRTC-sharingScreen",
                                    stringBundle.getString(stringId),
                                    "webRTC-sharingScreen-notification-icon",
                                    mainAction, secondaryActions, options);
}

function removeBrowserSpecificIndicator(aSubject, aTopic, aData) {
  let browser = getBrowserForWindowId(aData);
  let PopupNotifications = browser.ownerDocument.defaultView.PopupNotifications;
  if (!PopupNotifications)
    return;

  for (let notifId of ["webRTC-sharingDevices", "webRTC-sharingScreen"]) {
    let notification = PopupNotifications.getNotification(notifId, browser);
    if (notification)
      PopupNotifications.remove(notification);
  }
}
