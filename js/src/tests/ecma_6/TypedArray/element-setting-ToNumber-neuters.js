



"use strict"; 

var gTestfile = 'element-setting-ToNumber-neuters.js';

var BUGNUMBER = 1001547;
var summary =
  "Don't assert assigning into memory neutered while converting the value to " +
  "assign into a number";

print(BUGNUMBER + ": " + summary);











var ab1 = new ArrayBuffer(64);
var ta1 = new Uint32Array(ab1);
ta1[4] = { valueOf: function() { neuter(ab1, "change-data"); return 5; } };

var ab2 = new ArrayBuffer(64);
var ta2 = new Uint32Array(ab2);
ta2[4] = { valueOf: function() { neuter(ab2, "same-data"); return 5; } };



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
