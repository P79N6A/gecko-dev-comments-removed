




a = evalcx("lazy");
a.watch("x", function() {});
({}).watch("x", function() {});
a.__defineGetter__("y", {});
