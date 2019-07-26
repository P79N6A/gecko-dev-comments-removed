Object.prototype[0] = "foo";
assertEq([, 5].concat().hasOwnProperty("0"), true);
