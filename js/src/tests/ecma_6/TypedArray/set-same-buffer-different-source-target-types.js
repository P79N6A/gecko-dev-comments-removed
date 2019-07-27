




var gTestfile = "set-same-buffer-different-source-target-types.js";

var BUGNUMBER = 896116;
var summary =
  "When setting a typed array from an overlapping typed array of different " +
  "element type, copy the source elements into properly-sized temporary " +
  "memory, and properly copy them into the target without overflow (of " +
  "either source *or* target) when finished";

print(BUGNUMBER + ": " + summary);








var srclen = 65536;

var ta = new Uint8Array(srclen * Float64Array.BYTES_PER_ELEMENT);
var ta2 = new Float64Array(ta.buffer, 0, srclen);
ta.set(ta2);




for (var i = 0, len = ta.length; i < len; i++)
  assertEq(ta[i], 0, "zero-bits double should convert to zero");



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
