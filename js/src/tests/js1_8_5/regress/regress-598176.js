





function make() {
  var r = {};
  r.desc = {get: function() {}};
  r.a = Object.defineProperty({}, "prop", r.desc);
  r.info = Object.getOwnPropertyDescriptor(r.a, "prop");
  return r;
}

r1 = make();
assertEq(r1.desc.get, r1.info.get);


r2 = make();
assertEq(r1.desc.get === r2.desc.get, false);

r1.info.get.foo = 42;

assertEq(r1.desc.get.hasOwnProperty('foo'), !r2.desc.get.hasOwnProperty('foo'));
assertEq(r1.info.get.hasOwnProperty('foo'), !r2.info.get.hasOwnProperty('foo'));

reportCompare(0, 0, "ok");
