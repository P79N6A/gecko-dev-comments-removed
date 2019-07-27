







navigator.mozLoop = {
  ensureRegistered: function() {},
  getLoopCharPref: function() {},
  getLoopBoolPref: function(pref) {
    
    if (pref === "rooms.enabled") {
      return true;
    }
  },
  releaseCallData: function() {},
  contacts: {
    getAll: function(callback) {
      callback(null, []);
    },
    on: function() {}
  },
  fxAEnabled: true
};
