






const root1 = newGlobal();
const dbg1 = new Debugger();
dbg1.addDebuggee(root1)

const root2 = newGlobal();
const dbg2 = new Debugger();
dbg2.addDebuggee(root2)

let fired1 = false;
let fired2 = false;
dbg1.memory.onGarbageCollection = _ => fired1 = true;
dbg2.memory.onGarbageCollection = _ => fired2 = true;

function reset() {
  fired1 = false;
  fired2 = false;
}

function run_test() {
  do_test_pending();

  gc();
  executeSoon(() => {
    reset();

    
    root1.eval(`gc(this)`);
    executeSoon(() => {
      equal(fired1, true);

      
      reset();
      root2.eval(`gc(this)`);
      executeSoon(() => {
        equal(fired2, true);

        
        reset();
        gc();
        executeSoon(() => {
          equal(fired1, true);
          equal(fired2, true);

          
          reset();
          dbg1.removeAllDebuggees();
          dbg2.removeAllDebuggees();
          gc();
          executeSoon(() => {
            equal(fired1, false);
            equal(fired2, false);

            

            dbg1.addDebuggee(root1);
            dbg1.addDebuggee(root2);

            
            reset();
            root1.eval(`gc(this)`);
            executeSoon(() => {
              equal(fired1, true);
              equal(fired2, false);

              
              reset();
              root2.eval(`gc(this)`);
              executeSoon(() => {
                equal(fired1, true);
                equal(fired2, false);

                
                reset();
                gc();
                executeSoon(() => {
                  equal(fired1, true);
                  equal(fired2, false);
                  do_test_finished();
                });
              });
            });
          });
        });
      });
    });
  });
}
