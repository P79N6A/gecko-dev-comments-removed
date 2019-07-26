


"use strict";

var WebrtcUI = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "getUserMedia:request") {
      this.handleRequest(aSubject, aTopic, aData);
    }
  },

  handleRequest: function handleRequest(aSubject, aTopic, aData) {
    let { windowID: windowID, callID: callID } = JSON.parse(aData);

    let contentWindow = Services.wm.getOuterWindowWithId(windowID);
    let browser = BrowserApp.getBrowserForWindow(contentWindow);
    let params = aSubject.QueryInterface(Ci.nsIMediaStreamOptions);

    browser.ownerDocument.defaultView.navigator.mozGetUserMediaDevices(
      function (devices) {
        WebrtcUI.prompt(browser, callID, params.audio, params.video, devices);
      },
      function (error) {
        Cu.reportError(error);
      }
    );
  },

  getDeviceButtons: function(audioDevices, videoDevices, aCallID) {
    return [{
      label: Strings.browser.GetStringFromName("getUserMedia.denyRequest.label"),
      callback: function() {
        Services.obs.notifyObservers(null, "getUserMedia:response:deny", aCallID);
      }
    },
    {
      label: Strings.browser.GetStringFromName("getUserMedia.shareRequest.label"),
      callback: function(checked , inputs) {
        let allowedDevices = Cc["@mozilla.org/supports-array;1"].createInstance(Ci.nsISupportsArray);

        let audioId = 0;
        if (inputs && inputs.audioDevice != undefined)
          audioId = inputs.audioDevice;
        if (audioDevices[audioId])
          allowedDevices.AppendElement(audioDevices[audioId]);

        let videoId = 0;
        if (inputs && inputs.videoDevice != undefined)
          videoId = inputs.videoDevice;
        if (videoDevices[videoId])
          allowedDevices.AppendElement(videoDevices[videoId]);

        Services.obs.notifyObservers(allowedDevices, "getUserMedia:response:allow", aCallID);
        
      }
    }];
  },

  
  _getList: function(aDevices, aType) {
    let defaultCount = 0;
    return aDevices.map(function(device) {
        
        let res = /Camera\ \d+,\ Facing (front|back)/.exec(device.name);
        if (res)
          return Strings.browser.GetStringFromName("getUserMedia." + aType + "." + res[1]);

        if (device.name.trim() == "") {
          defaultCount++;
          return Strings.browser.formatStringFromName("getUserMedia." + aType + ".default", [defaultCount], 1);
        }
        return device.name
      }, this);
  },

  _addDevicesToOptions: function(aDevices, aType, aOptions, extraOptions) {
    if (aDevices.length) {

      
      let list = this._getList(aDevices, aType);
      if (extraOptions)
        list = list.concat(extraOptions);

      if (list.length > 0) {
        aOptions.inputs.push({
          id: aType,
          type: "menulist",
          label: Strings.browser.GetStringFromName("getUserMedia." + aType + ".prompt"),
          values: list
        });

      }
    }
  },

  prompt: function prompt(aBrowser, aCallID, aAudioRequested, aVideoRequested, aDevices) {
    let audioDevices = [];
    let videoDevices = [];
    for (let device of aDevices) {
      device = device.QueryInterface(Ci.nsIMediaDevice);
      switch (device.type) {
      case "audio":
        if (aAudioRequested)
          audioDevices.push(device);
        break;
      case "video":
        if (aVideoRequested)
          videoDevices.push(device);
        break;
      }
    }

    let requestType;
    if (audioDevices.length && videoDevices.length)
      requestType = "CameraAndMicrophone";
    else if (audioDevices.length)
      requestType = "Microphone";
    else if (videoDevices.length)
      requestType = "Camera";
    else
      return;

    let host = aBrowser.contentDocument.documentURIObject.asciiHost;
    let requestor = BrowserApp.manifest ? "'" + BrowserApp.manifest.name  + "'" : host;
    let message = Strings.browser.formatStringFromName("getUserMedia.share" + requestType + ".message", [ requestor ], 1);

    let options = { inputs: [] };
    
    
    
    var extraItems = null;
    if (videoDevices.length > 1 || audioDevices.length > 0) {
      
      if (audioDevices.length > 0)
        extraItems = [ Strings.browser.GetStringFromName("getUserMedia.videoDevice.none") ];
      this._addDevicesToOptions(videoDevices, "videoDevice", options, extraItems);
    }

    if (audioDevices.length > 1 || videoDevices.length > 0) {
      
      if (videoDevices.length > 0)
        extraItems = [ Strings.browser.GetStringFromName("getUserMedia.audioDevice.none") ];
      this._addDevicesToOptions(audioDevices, "audioDevice", options, extraItems);
    }

    let buttons = this.getDeviceButtons(audioDevices, videoDevices, aCallID);

    NativeWindow.doorhanger.show(message, "webrtc-request", buttons, BrowserApp.selectedTab.id, options);
  }
}
