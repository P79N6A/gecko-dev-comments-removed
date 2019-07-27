
























function run_test() {
  do_test_pending();

  const debuggeree = newGlobal();
  const debuggee = debuggeree.debuggee = newGlobal();

  debuggeree.eval(
    `
    const dbg = new Debugger(this.debuggee);
    let fired = 0;
    dbg.memory.onGarbageCollection = _ => {
      fired++;
      gc(this);
    };
    `
  );

  const dbg = new Debugger(debuggeree);
  let fired = 0;
  dbg.memory.onGarbageCollection = _ => {
    fired++;
  };

  debuggee.eval(`gc(this)`);

  
  executeSoon(() => {

    
    executeSoon(() => {

      
      
      
      ok(debuggeree.fired >= 1);
      ok(fired >= 1);

      debuggeree.dbg.enabled = dbg.enabled = false;
      do_test_finished();
    });
  });
}
