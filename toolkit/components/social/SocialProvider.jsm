





const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FrameWorker.jsm");

const EXPORTED_SYMBOLS = ["SocialProvider"];








function SocialProvider(input) {
  if (!input.name)
    throw new Error("SocialProvider must be passed a name");
  if (!input.origin)
    throw new Error("SocialProvider must be passed an origin");

  this.name = input.name;
  this.workerURL = input.workerURL;
  this.origin = input.origin;
}

SocialProvider.prototype = {
  


  terminate: function shutdown() {
    if (this.workerURL) {
      try {
        getFrameWorkerHandle(this.workerURL, null).terminate();
      } catch (e) {
        Cu.reportError("SocialProvider FrameWorker termination failed: " + e);
      }
    }
  },

  







  getWorkerPort: function getWorkerPort(window) {
    if (!this.workerURL)
      return null;
    return getFrameWorkerHandle(this.workerURL, window).port;
  }
}
