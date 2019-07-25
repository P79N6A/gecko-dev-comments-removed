





const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FrameWorker.jsm");
Cu.import("resource://gre/modules/WorkerAPI.jsm");

const EXPORTED_SYMBOLS = ["SocialProvider"];








function SocialProvider(input) {
  if (!input.name)
    throw new Error("SocialProvider must be passed a name");
  if (!input.origin)
    throw new Error("SocialProvider must be passed an origin");

  this.name = input.name;
  this.workerURL = input.workerURL;
  this.origin = input.origin;

  let workerAPIPort = this.getWorkerPort();
  if (workerAPIPort)
    this.workerAPI = new WorkerAPI(workerAPIPort);
}

SocialProvider.prototype = {
  


  terminate: function terminate() {
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
    try {
      return getFrameWorkerHandle(this.workerURL, window).port;
    } catch (ex) {
      Cu.reportError("SocialProvider: retrieving worker port failed:" + ex);
      return null;
    }
  }
}
