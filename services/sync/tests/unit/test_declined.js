


Cu.import("resource://services-sync/stages/declined.js");
Cu.import("resource://services-sync/stages/enginesync.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-common/observers.js");

function run_test() {
  run_next_test();
}

function PetrolEngine() {}
PetrolEngine.prototype.name = "petrol";

function DieselEngine() {}
DieselEngine.prototype.name = "diesel";

function DummyEngine() {}
DummyEngine.prototype.name = "dummy";

function ActualEngine() {}
ActualEngine.prototype = {__proto__: Engine.prototype,
                          name: 'actual'};

function getEngineManager() {
  let manager = new EngineManager(Service);
  Service.engineManager = manager;
  manager._engines = {
    "petrol": new PetrolEngine(),
    "diesel": new DieselEngine(),
    "dummy": new DummyEngine(),
    "actual": new ActualEngine(),
  };
  return manager;
}











add_test(function testOldMeta() {
  let meta = {
    payload: {
      engines: {
        "petrol": 1,
        "diesel": 2,
        "nonlocal": 3,             
      },
    },
  };

  _("Record: " + JSON.stringify(meta));

  let manager = getEngineManager();

  
  let engineSync = new EngineSynchronizer(Service);
  engineSync._updateEnabledFromMeta(meta, 3, manager);

  Assert.ok(manager._engines["petrol"].enabled, "'petrol' locally enabled.");
  Assert.ok(manager._engines["diesel"].enabled, "'diesel' locally enabled.");
  Assert.ok(!("nonlocal" in manager._engines), "We don't know anything about the 'nonlocal' engine.");
  Assert.ok(!manager._engines["actual"].enabled, "'actual' not locally enabled.");
  Assert.ok(!manager.isDeclined("actual"), "'actual' not declined, though.");

  let declinedEngines = new DeclinedEngines(Service);

  function onNotDeclined(subject, topic, data) {
    Observers.remove("weave:engines:notdeclined", onNotDeclined);
    Assert.ok(subject.undecided.has("actual"), "EngineManager observed that 'actual' was undecided.");

    let declined = manager.getDeclined();
    _("Declined: " + JSON.stringify(declined));

    Assert.ok(!meta.changed, "No need to upload a new meta/global.");
    run_next_test();
  }

  Observers.add("weave:engines:notdeclined", onNotDeclined);

  declinedEngines.updateDeclined(meta, manager);
});








add_test(function testDeclinedMeta() {
  let meta = {
    payload: {
      engines: {
        "petrol": 1,
        "diesel": 2,
        "nonlocal": 3,             
      },
      declined: ["nonexistent"],   
    },
  };

  _("Record: " + JSON.stringify(meta));

  let manager = getEngineManager();
  manager._engines["petrol"].enabled = true;
  manager._engines["diesel"].enabled = true;
  manager._engines["dummy"].enabled = true;
  manager._engines["actual"].enabled = false;   

  manager.decline(["localdecline"]);            

  let declinedEngines = new DeclinedEngines(Service);

  function onNotDeclined(subject, topic, data) {
    Observers.remove("weave:engines:notdeclined", onNotDeclined);
    Assert.ok(subject.undecided.has("actual"), "EngineManager observed that 'actual' was undecided.");

    let declined = manager.getDeclined();
    _("Declined: " + JSON.stringify(declined));

    Assert.equal(declined.indexOf("actual"), -1, "'actual' is locally disabled, but not marked as declined.");

    Assert.equal(declined.indexOf("clients"), -1, "'clients' is enabled and not remotely declined.");
    Assert.equal(declined.indexOf("petrol"), -1, "'petrol' is enabled and not remotely declined.");
    Assert.equal(declined.indexOf("diesel"), -1, "'diesel' is enabled and not remotely declined.");
    Assert.equal(declined.indexOf("dummy"), -1, "'dummy' is enabled and not remotely declined.");

    Assert.ok(0 <= declined.indexOf("nonexistent"), "'nonexistent' was declined on the server.");

    Assert.ok(0 <= declined.indexOf("localdecline"), "'localdecline' was declined locally.");

    
    Assert.ok(0 <= meta.payload.declined.indexOf("nonexistent"), "meta/global's declined contains 'nonexistent'.");
    Assert.ok(0 <= meta.payload.declined.indexOf("localdecline"), "meta/global's declined contains 'localdecline'.");
    Assert.strictEqual(true, meta.changed, "meta/global was changed.");

    run_next_test();
  }

  Observers.add("weave:engines:notdeclined", onNotDeclined);

  declinedEngines.updateDeclined(meta, manager);
});

