


var gTestfile = 'stringify-replacer-array-hijinks.js';

var BUGNUMBER = 648471;
var summary =
  "Better/more correct handling for replacer arrays with getter array index " +
  "properties";

print(BUGNUMBER + ": " + summary);





var replacer = [0, 1, 2, 3];
Object.prototype[3] = 3;
Object.defineProperty(replacer, 1, {
  get: function()
  {
    Object.defineProperty(replacer, 4, { value: 4 });
    delete replacer[2];
    delete replacer[3];
    replacer[5] = 5;
    return 1;
  }
});

var s =
  JSON.stringify({0: { 1: { 3: { 4: { 5: { 2: "omitted" } } } } } }, replacer);




assertEq('{"0":{"1":{"3":{"3":3}},"3":3},"3":3}', s);


var replacer = [0, 1, 2, 3];
Object.defineProperty(replacer, 0, {
  get: function()
  {
    replacer.length = 0;
    return {};
  }
});




assertEq(JSON.stringify({ 0: 0, 1: 1, 2: 2, 3: 3 }, replacer),
         '{"3":3}');



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
