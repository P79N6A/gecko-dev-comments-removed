


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/ClientID.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function run_test() {
  do_get_profile();
  run_next_test();
}

add_task(function* () {
  const drsPath = OS.Path.join(OS.Constants.Path.profileDir, "datareporting", "state.json");
  const fhrDir  = OS.Path.join(OS.Constants.Path.profileDir, "healthreport");
  const fhrPath = OS.Path.join(fhrDir, "state.json");
  const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;

  yield OS.File.makeDir(fhrDir);

  
  let clientID = CommonUtils.generateUUID();
  yield CommonUtils.writeJSON({clientID: clientID}, fhrPath);
  Assert.equal(clientID, yield ClientID.getClientID());

  
  yield ClientID._reset();
  yield CommonUtils.writeJSON({clientID: CommonUtils.generateUUID()}, fhrPath);
  Assert.equal(clientID, yield ClientID.getClientID());

  
  yield ClientID._reset();
  yield OS.File.remove(drsPath);
  yield CommonUtils.writeJSON({clientID: -1}, fhrPath);
  clientID = yield ClientID.getClientID();
  Assert.equal(typeof(clientID), 'string');
  Assert.ok(uuidRegex.test(clientID));

  
  yield ClientID._reset();
  yield OS.File.remove(drsPath);
  yield OS.File.writeAtomic(fhrPath, "abcd", {encoding: "utf-8", tmpPath: fhrPath + ".tmp"});
  clientID = yield ClientID.getClientID();
  Assert.equal(typeof(clientID), 'string');
  Assert.ok(uuidRegex.test(clientID));

  
  
  yield ClientID._reset();
  clientID = CommonUtils.generateUUID();
  yield CommonUtils.writeJSON({clientID: clientID}, fhrPath);
  yield CommonUtils.writeJSON({clientID: -1}, drsPath);
  Assert.equal(clientID, yield ClientID.getClientID());

  
  yield ClientID._reset();
  yield OS.File.remove(fhrPath);
  yield OS.File.writeAtomic(drsPath, "abcd", {encoding: "utf-8", tmpPath: drsPath + ".tmp"});
  clientID = yield ClientID.getClientID();
  Assert.equal(typeof(clientID), 'string');
  Assert.ok(uuidRegex.test(clientID));

  
  yield ClientID._reset();
  yield CommonUtils.writeJSON({clientID: -1}, fhrPath);
  yield CommonUtils.writeJSON({clientID: -1}, drsPath);
  clientID = yield ClientID.getClientID();
  Assert.equal(typeof(clientID), 'string');
  Assert.ok(uuidRegex.test(clientID));
});
