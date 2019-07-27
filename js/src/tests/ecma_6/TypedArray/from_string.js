
var from = Int8Array.from.bind(Array);


assertDeepEq(from("test string"),
             ['t', 'e', 's', 't', ' ', 's', 't', 'r', 'i', 'n', 'g']);


var gclef = "\uD834\uDD1E"; 
assertDeepEq(from(gclef), [gclef]);
assertDeepEq(from(gclef + " G"), [gclef, " ", "G"]);


String.prototype[Symbol.iterator] = function* () { yield 1; yield 2; };
assertDeepEq(from("anything"), [1, 2]);


delete String.prototype[Symbol.iterator];
assertDeepEq(from("works"), ['w', 'o', 'r', 'k', 's']);
assertDeepEq(from(gclef), ['\uD834', '\uDD1E']);

if (typeof reportCompare === "function")
    reportCompare(true, true);
