










if (!getBuildConfiguration()['moz-memory'])
  quit(0);

if (getBuildConfiguration()['pointer-byte-size'] == 4)
  var s = (s32, s64) => s32
else
  var s = (s32, s64) => s64

function tenure(obj) {
  gc();
  return obj;
}



function tByteSize(obj) {
  var size = byteSize(obj);
  minorgc();
  if (size != byteSize(obj))
    return 0;
  return size;
}

assertEq(tByteSize({}),                                 s(16,  32));


assertEq(tByteSize({ w: 1 }),                           s(32,  48));
assertEq(tByteSize({ w: 1, x: 2 }),                     s(32,  48));
assertEq(tByteSize({ w: 1, x: 2, y: 3 }),               s(48,  64));
assertEq(tByteSize({ w: 1, x: 2, y: 3, z:4 }),          s(48,  64));
assertEq(tByteSize({ w: 1, x: 2, y: 3, z:4, a: 5 }),    s(80,  96));


assertEq(tByteSize({ 0:0 }),                            s(96,  112));
assertEq(tByteSize({ 0:0, 1:1 }),                       s(96,  112));
assertEq(tByteSize({ 0:0, 1:1, 2:2 }),                  s(112, 128));
assertEq(tByteSize({ 0:0, 1:1, 2:2, 3:3 }),             s(112, 128));
assertEq(tByteSize({ 0:0, 1:1, 2:2, 3:3, 4:4 }),        s(144, 160));








assertEq(tByteSize({ w:1,                     0:0                     }),  s(96,  112));
assertEq(tByteSize({ w:1,                     0:0, 1:1, 2:2           }),  s(112, 128));
assertEq(tByteSize({ w:1,                     0:0, 1:1, 2:2, 3:3, 4:4 }),  s(144, 160));
assertEq(tByteSize({ w:1, x:2, y:3,           0:0                     }),  s(112, 128));
assertEq(tByteSize({ w:1, x:2, y:3,           0:0, 1:1, 2:2           }),  s(144, 160));
assertEq(tByteSize({ w:1, x:2, y:3,           0:0, 1:1, 2:2, 3:3, 4:4 }),  s(144, 160));
assertEq(tByteSize({ w:1, x:2, y:3, z:4, a:6, 0:0                     }),  s(144, 160));
assertEq(tByteSize({ w:1, x:2, y:3, z:4, a:6, 0:0, 1:1, 2:2           }),  s(144, 160));
assertEq(tByteSize({ w:1, x:2, y:3, z:4, a:6, 0:0, 1:1, 2:2, 3:3, 4:4 }),  s(176, 192));


assertEq(tByteSize([]),                                 s(80,  96));
assertEq(tByteSize([1]),                                s(48,  64));
assertEq(tByteSize([1, 2]),                             s(48,  64));
assertEq(tByteSize([1, 2, 3]),                          s(80,  96));
assertEq(tByteSize([1, 2, 3, 4]),                       s(80,  96));
assertEq(tByteSize([1, 2, 3, 4, 5]),                    s(80,  96));
assertEq(tByteSize([1, 2, 3, 4, 5, 6]),                 s(80,  96));
assertEq(tByteSize([1, 2, 3, 4, 5, 6, 7]),              s(112, 128));
assertEq(tByteSize([1, 2, 3, 4, 5, 6, 7, 8]),           s(112, 128));


assertEq(tByteSize(function () {}),                     s(32,  64));
assertEq(tByteSize(function () {}.bind()),              s(96,  128));
assertEq(tByteSize(() => 1),                            s(48,  96));
assertEq(tByteSize(Math.sin),                           s(32,  64));
