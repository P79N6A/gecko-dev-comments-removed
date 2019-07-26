


'use strict';

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci } = require('chrome');
const { Class } = require('../core/heritage');
const { EventTarget } = require('../event/target');
const { Branch } = require('./service');
const { emit, off } = require('../event/core');
const { when: unload } = require('../system/unload');

const prefTargetNS = require('../core/namespace').ns();

const PrefsTarget = Class({
  extends: EventTarget,
  initialize: function(options) {
    options = options || {};
    EventTarget.prototype.initialize.call(this, options);

    let branchName = options.branchName || '';
    let branch = Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefService).
        getBranch(branchName).
        QueryInterface(Ci.nsIPrefBranch2);
    prefTargetNS(this).branch = branch;

    
    this.prefs = Branch(branchName);

    
    let observer = prefTargetNS(this).observer = onChange.bind(this);
    branch.addObserver('', observer, false);

    
    unload(destroy.bind(this));
  }
});
exports.PrefsTarget = PrefsTarget;



function onChange(subject, topic, name) {
  if (topic === 'nsPref:changed') {
    emit(this, name, name);
    emit(this, '', name);
  }
}

function destroy() {
  off(this);

  
  let branch = prefTargetNS(this).branch;
  branch.removeObserver('', prefTargetNS(this).observer, false);
  prefTargetNS(this).observer = null;
}
