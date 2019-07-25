





const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FrameWorker.jsm");
Cu.import("resource://gre/modules/WorkerAPI.jsm");

const EXPORTED_SYMBOLS = ["SocialProvider"];









function SocialProvider(input, enabled) {
  if (!input.name)
    throw new Error("SocialProvider must be passed a name");
  if (!input.origin)
    throw new Error("SocialProvider must be passed an origin");

  this.name = input.name;
  this.iconURL = input.iconURL;
  this.workerURL = input.workerURL;
  this.origin = input.origin;
  this.ambientNotificationIcons = {};

  
  this._enabled = !(enabled == false);
  if (this._enabled)
    this._activate();
}

SocialProvider.prototype = {
  
  
  _enabled: true,
  get enabled() {
    return this._enabled;
  },
  set enabled(val) {
    let enable = !!val;
    if (enable == this._enabled)
      return;

    this._enabled = enable;

    if (enable) {
      this._activate();
    } else {
      this._terminate();
    }
  },

  
  
  port: null,

  
  
  workerAPI: null,

  
  
  
  
  
  profile: null,

  
  
  
  
  ambientNotificationIcons: null,

  
  updateUserProfile: function(profile) {
    this.profile = profile;

    if (profile.iconURL)
      this.iconURL = profile.iconURL;

    if (!profile.displayName)
      profile.displayName = profile.userName;

    
    
    
    if (!profile.userName) {
      this.profile = {};
      this.ambientNotificationIcons = {};
      Services.obs.notifyObservers(null, "social:ambient-notification-changed", this.origin);
    }

    Services.obs.notifyObservers(null, "social:profile-changed", this.origin);
  },

  
  setAmbientNotification: function(notification) {
    if (!this.profile.userName)
      throw new Error("unable to set notifications while logged out");
    this.ambientNotificationIcons[notification.name] = notification;

    Services.obs.notifyObservers(null, "social:ambient-notification-changed", this.origin);
  },

  
  _activate: function _activate() {
    
    
    let workerAPIPort = this._getWorkerPort();
    if (workerAPIPort)
      this.workerAPI = new WorkerAPI(this, workerAPIPort);

    this.port = this._getWorkerPort();
  },

  _terminate: function _terminate() {
    if (this.workerURL) {
      try {
        getFrameWorkerHandle(this.workerURL, null).terminate();
      } catch (e) {
        Cu.reportError("SocialProvider FrameWorker termination failed: " + e);
      }
    }
    this.port = null;
    this.workerAPI = null;
  },

  







  _getWorkerPort: function _getWorkerPort(window) {
    if (!this.workerURL || !this.enabled)
      return null;
    try {
      return getFrameWorkerHandle(this.workerURL, window).port;
    } catch (ex) {
      Cu.reportError("SocialProvider: retrieving worker port failed:" + ex);
      return null;
    }
  }
}
