



"use strict";

this.EXPORTED_SYMBOLS = ["MockPolicyListener"];

const {utils: Cu} = Components;

Cu.import("resource://services-common/log4moz.js");


this.MockPolicyListener = function MockPolicyListener() {
  this._log = Log4Moz.repository.getLogger("HealthReport.Testing.MockPolicyListener");
  this._log.level = Log4Moz.Level["Debug"];

  this.requestDataSubmissionCount = 0;
  this.lastDataRequest = null;

  this.notifyUserCount = 0;
  this.lastNotifyRequest = null;
}

MockPolicyListener.prototype = {
  onRequestDataSubmission: function onRequestDataSubmission(request) {
    this._log.info("onRequestDataSubmission invoked.");
    this.requestDataSubmissionCount++;
    this.lastDataRequest = request;
  },

  onNotifyDataPolicy: function onNotifyDataPolicy(request) {
    this._log.info("onNotifyUser invoked.");
    this.notifyUserCount++;
    this.lastNotifyRequest = request;
  },
};
