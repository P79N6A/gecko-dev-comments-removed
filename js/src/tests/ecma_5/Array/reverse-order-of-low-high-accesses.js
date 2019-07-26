





var BUGNUMBER = 858677;
var summary =
  "[].reverse should swap elements low to high using accesses to low " +
  "elements, then accesses to high elements";

print(BUGNUMBER + ": " + summary);





var observed = [];












var props =
  {
    0: {
      configurable: true,
      get: function() { observed.push("index 0 get"); return "index 0 get"; },
      set: function(v) { observed.push("index 0 set: " + v); }
    },
    
    2: {
      configurable: true,
      get: function() { observed.push("index 2 get"); return "index 2 get"; },
      set: function(v) { observed.push("index 2 set: " + v); }
    },
    
    
    
    6: {
      configurable: true,
      get: function() { observed.push("index 6 get"); return "index 6 get"; },
      set: function(v) { observed.push("index 6 set: " + v); }
    },
    7: {
      configurable: true,
      get: function() { observed.push("index 7 get"); return "index 7 get"; },
      set: function(v) { observed.push("index 7 set: " + v); }
    },
  };

var arr = Object.defineProperties(new Array(8), props);

arr.reverse();

var expectedObserved =
  ["index 0 get", "index 7 get", "index 0 set: index 7 get", "index 7 set: index 0 get",
   "index 6 get",
   "index 2 get"
   ];
print(observed);

assertEq(observed.length, expectedObserved.length);
for (var i = 0; i < expectedObserved.length; i++)
  assertEq(observed[i], expectedObserved[i]);

assertEq(arr[0], "index 0 get"); 
assertEq(arr[1], "index 6 get"); 
assertEq(2 in arr, false); 
assertEq(3 in arr, false); 
assertEq(4 in arr, false); 
assertEq(arr[5], "index 2 get"); 
assertEq(6 in arr, false); 
assertEq(arr[7], "index 7 get"); 



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
