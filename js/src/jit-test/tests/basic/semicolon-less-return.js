


load(libdir + "class.js");

if (options().indexOf("werror") == -1)
  options("werror");

function testWarn(code, lineNumber, columnNumber) {
  var caught = false;
  try {
    eval(code);
  } catch (e) {
    caught = true;
    assertEq(e.message, "unreachable expression after semicolon-less return statement", code);
    assertEq(e.lineNumber, lineNumber);
    assertEq(e.columnNumber, columnNumber);
  }
  assertEq(caught, true, "warning should be caught for " + code);

  caught = false;
  try {
    Reflect.parse(code);
  } catch (e) {
    caught = true;
    assertEq(e.message, "unreachable expression after semicolon-less return statement", code);
  }
  assertEq(caught, true, "warning should be caught for " + code);
}

function testPass(code) {
  var caught = false;
  try {
    eval(code);
  } catch (e) {
    caught = true;
  }
  assertEq(caught, false, "warning should not be caught for " + code);

  caught = false;
  try {
    Reflect.parse(code);
  } catch (e) {
    caught = true;
  }
  assertEq(caught, false, "warning should not be caught for " + code);
}



testPass(`
function f() {
  return (
    1 + 2
  );
}
`);
testPass(`
function f() {
  return;
  1 + 2;
}
`);




testWarn(`
function f() {
  var i = 0;
  return
    ++i;
}
`, 5, 4);


testWarn(`
function f() {
  var i = 0;
  return
    --i;
}
`, 5, 4);


testWarn(`
function f() {
  return
    [1, 2, 3];
}
`, 4, 4);


testWarn(`
function f() {
  return
    {x: 10};
}
`, 4, 4);
testWarn(`
function f() {
  return
  {
    method()
    {
      f();
    }
  };
}
`, 4, 2);


testWarn(`
function f() {
  return
    (1 + 2);
}
`, 4, 4);


testWarn(`
function f() {
  return
    f;
}
`, 4, 4);


testWarn(`
function f() {
  return
    1 + 2;
}
`, 4, 4);
testWarn(`
function f() {
  return
    .1 + .2;
}
`, 4, 4);


testWarn(`
function f() {
  return
    "foo";
}
`, 4, 4);
testWarn(`
function f() {
  return
    "use struct";
}
`, 4, 4);
testWarn(`
function f() {
  return
    'foo';
}
`, 4, 4);


testWarn(`
function f() {
  return
    \`foo\${1 + 2}\`;
}
`, 4, 4);


testWarn(`
function f() {
  return
    \`foo\`;
}
`, 4, 4);


testWarn(`
function f() {
  return
    /foo/;
}
`, 4, 4);


testWarn(`
function f() {
  return
    true;
}
`, 4, 4);


testWarn(`
function f() {
  return
    false;
}
`, 4, 4);


testWarn(`
function f() {
  return
    null;
}
`, 4, 4);


testWarn(`
function f() {
  return
    this;
}
`, 4, 4);


testWarn(`
function f() {
  return
    new Array();
}
`, 4, 4);


testWarn(`
function f() {
  var a = {x: 10};
  return
    delete a.x;
}
`, 5, 4);


testWarn(`
function* f() {
  return
    yield 1;
}
`, 4, 4);


if (classesEnabled()) {
  testWarn(`
function f() {
  return
    class A { constructor() {} };
}
`, 4, 4);
}


testWarn(`
function f() {
  return
    +1;
}
`, 4, 4);


testWarn(`
function f() {
  return
    -1;
}
`, 4, 4);


testWarn(`
function f() {
  return
    !1;
}
`, 4, 4);


testWarn(`
function f() {
  return
    ~1;
}
`, 4, 4);




testPass(`
var f = new Function("return\\n");
`);


testPass(`
function f() {
  return
  ;
}
`);


testPass(`
function f() {
  {
    return
  }
}
`);


testPass(`
function f() {
  g();
  return
  function g() {}
}
`);


testPass(`
function f() {
  return
  if (true)
    1 + 2;
}
`);


testPass(`
function f() {
  if (true)
    return
  else
    1 + 2;
}
`);


testPass(`
function f() {
  return
  switch (1) {
    case 1:
      break;
  }
}
`);


testPass(`
function f() {
  switch (1) {
    case 0:
      return
    case 1:
      break;
  }
}
`);


testPass(`
function f() {
  switch (1) {
    case 0:
      return
    default:
      break;
  }
}
`);


testPass(`
function f() {
  return
  while (false)
    1 + 2;
}
`);
testPass(`
function f() {
  do
    return
  while (false);
}
`);


testPass(`
function f() {
  return
  do {
    1 + 2;
  } while (false);
}
`);


testPass(`
function f() {
  return
  for (;;) {
    break;
  }
}
`);


testPass(`
function f() {
  for (;;) {
    return
    break;
  }
}
`);


testPass(`
function f() {
  for (;;) {
    return
    continue;
  }
}
`);


testPass(`
function f() {
  return
  var a = 1;
}
`);


testPass(`
function f() {
  return
  const a = 1;
}
`);


testPass(`
function f() {
  return
  with ({}) {
    1;
  }
}
`);


testPass(`
function f() {
  return
  return;
}
`);


testPass(`
function f() {
  return
  try {
  } catch (e) {
  }
}
`);


testPass(`
function f() {
  return
  throw 1;
}
`);


testPass(`
function f() {
  return
  debugger;
}
`);


testPass(`
function f() {
  return
  let a = 1;
}
`);





testWarn(`
function f() {
  return
  a: 1;
}
`, 4, 2);
