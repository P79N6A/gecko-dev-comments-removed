










const Cu = Components.utils;
const { byteSize } = Cu.getJSTestingFunctions();

function run_test()
{
  const objects = [
    {},
    { w: 1, x: 2, y: 3, z:4, a: 5 },
    [],
    Array(10).fill(null),
    new RegExp("(2|two) problems", "g"),
    new Date(),
    new Uint8Array(64),
    Promise.resolve(1),
    function f() {},
    Object
  ];

  for (let obj of objects) {
    do_print(uneval(obj));
    ok(byteSize(obj), "We should get some (non-zero) byte size");
  }
}
