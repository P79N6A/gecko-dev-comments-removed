



this.EXPORTED_SYMBOLS = ["ClusterManager"];

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/util.js");




this.ClusterManager = function ClusterManager(service) {
  this._log = Log.repository.getLogger("Sync.Service");
  this._log.level = Log.Level[Svc.Prefs.get("log.logger.service.main")];

  this.service = service;
}
ClusterManager.prototype = {
  get identity() {
    return this.service.identity;
  },

  




  _findCluster: function _findCluster() {
    this._log.debug("Finding cluster for user " + this.identity.username);

    
    
    let fail;
    let url = this.service.userAPIURI + this.identity.username + "/node/weave";
    let res = this.service.resource(url);
    try {
      let node = res.get();
      switch (node.status) {
        case 400:
          this.service.status.login = LOGIN_FAILED_LOGIN_REJECTED;
          fail = "Find cluster denied: " + this.service.errorHandler.errorStr(node);
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
          this.service.errorHandler.checkServerError(node);
          fail = "Unexpected response code: " + node.status;
          break;
      }
    } catch (e) {
      this._log.debug("Network error on findCluster");
      this.service.status.login = LOGIN_FAILED_NETWORK_ERROR;
      this.service.errorHandler.checkServerError(e);
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
