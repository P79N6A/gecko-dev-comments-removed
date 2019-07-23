




var obj = {};
obj.__defineSetter__("x", function() {});
obj.watch("x", function() {});
obj.__defineSetter__("x", /a/);
