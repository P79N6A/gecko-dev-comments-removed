





const { BreakpointStore, ThreadActor } = devtools.require("devtools/server/actors/script");

function run_test()
{
  Cu.import("resource://gre/modules/jsdebugger.jsm");
  addDebuggerToGlobal(this);

  test_has_breakpoint();
  test_add_breakpoint();
  test_remove_breakpoint();
  test_find_breakpoints();
  test_duplicate_breakpoints();
  test_move_breakpoint();
}

function test_has_breakpoint() {
  let bpStore = new BreakpointStore();
  let location = {
    source: { actor: 'actor1' },
    line: 3
  };
  let columnLocation = {
    source: { actor: 'actor2' },
    line: 5,
    column: 15
  };

  
  do_check_eq(null, bpStore.hasBreakpoint(location),
              "Breakpoint not added and shouldn't exist.");

  bpStore.addBreakpoint(location);
  do_check_true(!!bpStore.hasBreakpoint(location),
                "Breakpoint added but not found in Breakpoint Store.");

  bpStore.removeBreakpoint(location);
  do_check_eq(null, bpStore.hasBreakpoint(location),
              "Breakpoint removed but still exists.");

  
  do_check_eq(null, bpStore.hasBreakpoint(columnLocation),
              "Breakpoint with column not added and shouldn't exist.");

  bpStore.addBreakpoint(columnLocation);
  do_check_true(!!bpStore.hasBreakpoint(columnLocation),
                "Breakpoint with column added but not found in Breakpoint Store.");

  bpStore.removeBreakpoint(columnLocation);
  do_check_eq(null, bpStore.hasBreakpoint(columnLocation),
              "Breakpoint with column removed but still exists in Breakpoint Store.");
}

function test_add_breakpoint() {
  
  let bpStore = new BreakpointStore();
  let location = {
    source: { actor: 'actor1' },
    line: 10,
    column: 9
  };
  bpStore.addBreakpoint(location);
  do_check_true(!!bpStore.hasBreakpoint(location),
                "We should have the column breakpoint we just added");

  
  location = {
    source: { actor: 'actor2' },
    line: 103
  };
  bpStore.addBreakpoint(location);
  do_check_true(!!bpStore.hasBreakpoint(location),
                "We should have the whole line breakpoint we just added");
}

function test_remove_breakpoint() {
  
  let bpStore = new BreakpointStore();
  let location = {
    source: { actor: 'actor1' },
    line: 10,
    column: 9
  };
  bpStore.addBreakpoint(location);
  bpStore.removeBreakpoint(location);
  do_check_eq(bpStore.hasBreakpoint(location), null,
              "We should not have the column breakpoint anymore");

  
  location = {
    source: { actor: 'actor2' },
    line: 103
  };
  bpStore.addBreakpoint(location);
  bpStore.removeBreakpoint(location);
  do_check_eq(bpStore.hasBreakpoint(location), null,
              "We should not have the whole line breakpoint anymore");
}

function test_find_breakpoints() {
  let bps = [
    { source: { actor: "actor1" }, line: 10 },
    { source: { actor: "actor1" }, line: 10, column: 3 },
    { source: { actor: "actor1" }, line: 10, column: 10 },
    { source: { actor: "actor1" }, line: 23, column: 89 },
    { source: { actor: "actor2" }, line: 10, column: 1 },
    { source: { actor: "actor2" }, line: 20, column: 5 },
    { source: { actor: "actor2" }, line: 30, column: 34 },
    { source: { actor: "actor2" }, line: 40, column: 56 }
  ];

  let bpStore = new BreakpointStore();

  for (let bp of bps) {
    bpStore.addBreakpoint(bp);
  }

  

  let bpSet = Set(bps);
  for (let bp of bpStore.findBreakpoints()) {
    bpSet.delete(bp);
  }
  do_check_eq(bpSet.size, 0,
              "Should be able to iterate over all breakpoints");

  

  bpSet = Set(bps.filter(bp => { return bp.source.actor === "actor1" }));
  for (let bp of bpStore.findBreakpoints({ source: { actor: "actor1" } })) {
    bpSet.delete(bp);
  }
  do_check_eq(bpSet.size, 0,
              "Should be able to filter the iteration by url");

  

  bpSet = Set(bps.filter(bp => { return bp.source.actor === "actor1" && bp.line === 10; }));
  let first = true;
  for (let bp of bpStore.findBreakpoints({ source: { actor: "actor1" }, line: 10 })) {
    if (first) {
      do_check_eq(bp.column, undefined,
                  "Should always get the whole line breakpoint first");
      first = false;
    } else {
      do_check_neq(bp.column, undefined,
                   "Should not get the whole line breakpoint any time other than first.");
    }
    bpSet.delete(bp);
  }
  do_check_eq(bpSet.size, 0,
              "Should be able to filter the iteration by url and line");
}

function test_duplicate_breakpoints() {
  let bpStore = new BreakpointStore();

  
  let location = {
    source: { actor: "foo-actor" },
    line: 10,
    column: 9
  };
  bpStore.addBreakpoint(location);
  bpStore.addBreakpoint(location);
  do_check_eq(bpStore.size, 1, "We should have only 1 column breakpoint");
  bpStore.removeBreakpoint(location);

  
  location = {
    source: { actor: "foo-actor" },
    line: 15
  };
  bpStore.addBreakpoint(location);
  bpStore.addBreakpoint(location);
  do_check_eq(bpStore.size, 1, "We should have only 1 whole line breakpoint");
  bpStore.removeBreakpoint(location);
}

function test_move_breakpoint() {
  let bpStore = new BreakpointStore();

  let oldLocation = {
    source: { actor: "foo-actor" },
    line: 10
  };

  let newLocation = {
    source: { actor: "foo-actor" },
    line: 12
  };

  bpStore.addBreakpoint(oldLocation);
  bpStore.moveBreakpoint(oldLocation, newLocation);

  equal(bpStore.size, 1, "Moving a breakpoint maintains the correct size.");

  let bp = bpStore.getBreakpoint(newLocation);
  ok(bp, "We should be able to get a breakpoint at the new location.");
  equal(bp.line, newLocation.line,
        "We should get the moved line.");

  equal(bpStore.hasBreakpoint({ source: { actor: "foo-actor" }, line: 10 }),
        null,
        "And we shouldn't be able to get any BP at the old location.");
}
