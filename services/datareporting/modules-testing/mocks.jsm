



"use strict";

this.EXPORTED_SYMBOLS = ["MockPolicyListener"];

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");


this.MockPolicyListener = function MockPolicyListener() {
  this._log = Log.repository.getLogger("Services.DataReporting.Testing.MockPolicyListener");
  this._log.level = Log.Level["Debug"];

  this.requestDataUploadCount = 0;
  this.lastDataRequest = null;

  this.requestRemoteDeleteCount = 0;
  this.lastRemoteDeleteRequest = null;

  this.notifyUserCount = 0;
  this.lastNotifyRequest = null;
}

MockPolicyListener.prototype = {
  onRequestDataUpload: function onRequestDataUpload(request) {
    this._log.info("onRequestDataUpload invoked.");
    this.requestDataUploadCount++;
    this.lastDataRequest = request;
  },

  onRequestRemoteDelete: function onRequestRemoteDelete(request) {
    this._log.info("onRequestRemoteDelete invoked.");
    this.requestRemoteDeleteCount++;
    this.lastRemoteDeleteRequest = request;
  },

  onNotifyDataPolicy: function onNotifyDataPolicy(request) {
    this._log.info("onNotifyUser invoked.");
    this.notifyUserCount++;
    this.lastNotifyRequest = request;
  },
};

