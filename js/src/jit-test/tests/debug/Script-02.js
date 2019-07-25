


load(libdir + 'asserts.js');

assertThrowsInstanceOf(function() { Debug.Script(); }, TypeError);
assertThrowsInstanceOf(function() { new Debug.Script(); }, TypeError);
