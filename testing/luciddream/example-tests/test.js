ok(true, "Assertion from a JS script!");

setTimeout(function() {
  ok(true, "Assertion from setTimeout!");
  finish();
}, 15);
