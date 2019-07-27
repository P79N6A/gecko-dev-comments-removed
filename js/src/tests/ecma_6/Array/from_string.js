



assertDeepEq(Array.from("test string"),
             ['t', 'e', 's', 't', ' ', 's', 't', 'r', 'i', 'n', 'g']);


var gclef = "\uD834\uDD1E"; 
assertDeepEq(Array.from(gclef), [gclef]);
assertDeepEq(Array.from(gclef + " G"), [gclef, " ", "G"]);


String.prototype[Symbol.iterator] = function* () { yield 1; yield 2; };
assertDeepEq(Array.from("anything"), [1, 2]);


delete String.prototype[Symbol.iterator];
assertDeepEq(Array.from("works"), ['w', 'o', 'r', 'k', 's']);
assertDeepEq(Array.from(gclef), ['\uD834', '\uDD1E']);

if (typeof reportCompare === 'function')
    reportCompare(0, 0);
