






const acorn = require("acorn/acorn");
Cu.import("resource://gre/modules/reflect.jsm");

const testCode = "" + function main () {
  function makeAcc(n) {
    return function () {
      return ++n;
    };
  }

  var acc = makeAcc(10);

  for (var i = 0; i < 10; i++) {
    acc();
  }

  console.log(acc());
};

function run_test() {
  const reflectAST = Reflect.parse(testCode);
  const acornAST = acorn.parse(testCode);

  do_print("Reflect AST:");
  do_print(JSON.stringify(reflectAST, null, 2));
  do_print("acorn AST:");
  do_print(JSON.stringify(acornAST, null, 2));

  checkEquivalentASTs(reflectAST, acornAST);
}
