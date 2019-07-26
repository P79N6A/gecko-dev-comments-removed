






if (typeof withSourceHook != 'function')
  quit(0);

let g = newGlobal();
let dbg = new Debugger(g);

function test(source) {
  
  
  let frobbed = source.replace(/debugger/, 'reggubed');
  let log = '';

  withSourceHook(function (url) {
    log += 's';
    assertEq(url, "BanalBivalve.jsm");
    return frobbed;
  }, () => {
    dbg.onDebuggerStatement = function (frame) {
      log += 'd';
      assertEq(frame.script.source.text, frobbed);
    }

    g.evaluate(source, { fileName: "BanalBivalve.jsm",
                         sourcePolicy: "LAZY_SOURCE"});
  });

  assertEq(log, 'ds');
}

test("debugger; // Ignominious Iguana");
test("(function () { debugger; /* Meretricious Marmoset */})();");
test("(() => { debugger; })(); // Gaunt Gibbon");
