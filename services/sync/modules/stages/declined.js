








"use strict";

this.EXPORTED_SYMBOLS = ["DeclinedEngines"];

const {utils: Cu} = Components;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-common/observers.js");
Cu.import("resource://gre/modules/Preferences.jsm");



this.DeclinedEngines = function (service) {
  this._log = Log.repository.getLogger("Sync.Declined");
  this._log.level = Log.Level[new Preferences(PREFS_BRANCH).get("log.logger.declined")];

  this.service = service;
}
this.DeclinedEngines.prototype = {
  updateDeclined: function (meta, engineManager=this.service.engineManager) {
    let enabled = new Set([e.name for each (e in engineManager.getEnabled())]);
    let known = new Set([e.name for each (e in engineManager.getAll())]);
    let remoteDeclined = new Set(meta.payload.declined || []);
    let localDeclined = new Set(engineManager.getDeclined());

    this._log.debug("Handling remote declined: " + JSON.stringify([...remoteDeclined]));
    this._log.debug("Handling local declined: " + JSON.stringify([...localDeclined]));

    
    
    
    
    
    let newDeclined = CommonUtils.union(localDeclined, CommonUtils.difference(remoteDeclined, enabled));

    
    
    let declinedChanged = !CommonUtils.setEqual(newDeclined, remoteDeclined);
    this._log.debug("Declined changed? " + declinedChanged);
    if (declinedChanged) {
      meta.changed = true;
      meta.payload.declined = [...newDeclined];
    }

    
    engineManager.setDeclined(newDeclined);

    
    
    let undecided = CommonUtils.difference(CommonUtils.difference(known, enabled), newDeclined);
    if (undecided.size) {
      let subject = {
        declined: newDeclined,
        enabled: enabled,
        known: known,
        undecided: undecided,
      };
      CommonUtils.nextTick(() => {
        Observers.notify("weave:engines:notdeclined", subject);
      });
    }

    return declinedChanged;
  },
};
