



const root = newGlobal();
const dbg = root.dbg = new Debugger();
const wrappedRoot = dbg.addDebuggee(root)

root.eval(`
  this.tests = [];

  function Ctor() { }

  function AsmModule(stdlib, foreign, heap) {
    "use asm";

    function test() {
      return 5|0;
    }

    return { test: test };
  }
  const buf = new ArrayBuffer(1024*8);

  (function immediate() {
    this.dbg.memory.trackingTenurePromotions = true;

    this.tests.push( new Object() );
    this.tests.push( new Array() );
    this.tests.push( new Uint8Array(256) );
    this.tests.push( (function () { return arguments; }()) );
    this.tests.push( AsmModule(this, {}, buf) );
    this.tests.push( /2manyproblemz/g );
    this.tests.push( [1,2,3][Symbol.iterator]() );
    this.tests.push( Error() );
    this.tests.push( new Ctor );
    this.tests.push( {} );
    this.tests.push( new Date );
    this.tests.push( [1,2,3] );

  }).call(this);

  minorgc();
`);

const promotions = dbg.memory.drainTenurePromotionsLog();
print(uneval(promotions));
assertEq(promotions.every(p => p.size >= 1), true);
