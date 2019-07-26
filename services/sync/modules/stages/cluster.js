



const EXPORTED_SYMBOLS = ["ClusterManager"];

const {utils: Cu} = Components;

Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/util.js");




function ClusterManager(service) {
  this._log = Log4Moz.repository.getLogger("Sync.Service");
  this._log.level = Log4Moz.Level[Svc.Prefs.get("log.logger.service.main")];

  this.service = service;
}
ClusterManager.prototype = {
  get identity() {
    return this.service._identity;
  },

  




  _findCluster: function _findCluster() {
    this._log.debug("Finding cluster for user " + this.identity.username);

    let fail;
    let res = new Resource(this.service.userAPI + this.identity.username +
                           "/node/weave");
    try {
      let node = res.get();
      switch (node.status) {
        case 400:
          Status.login = LOGIN_FAILED_LOGIN_REJECTED;
          fail = "Find cluster denied: " + ErrorHandler.errorStr(node);
          break;
        case 404:
          this._log.debug("Using serverURL as data cluster (multi-cluster support disabled)");
          return this.service.serverURL;
        case 0:
        case 200:
          if (node == "null") {
            node = null;
          }
          this._log.trace("_findCluster successfully returning " + node);
          return node;
        default:
          ErrorHandler.checkServerError(node);
          fail = "Unexpected response code: " + node.status;
          break;
      }
    } catch (e) {
      this._log.debug("Network error on findCluster");
      Status.login = LOGIN_FAILED_NETWORK_ERROR;
      ErrorHandler.checkServerError(e);
      fail = e;
    }
    throw fail;
  },

  


  setCluster: function setCluster() {
    
    let cluster = this._findCluster();
    this._log.debug("Cluster value = " + cluster);
    if (cluster == null) {
      return false;
    }

    
    if (cluster == this.service.clusterURL) {
      return false;
    }

    this._log.debug("Setting cluster to " + cluster);
    this.service.clusterURL = cluster;
    Svc.Prefs.set("lastClusterUpdate", Date.now().toString());

    return true;
  },
};
Object.freeze(ClusterManager.prototype);
