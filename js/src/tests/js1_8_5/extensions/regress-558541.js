






options("strict");
for (var i = 0; i < 5; i++)
  Boolean.prototype = 42;

reportCompare(true, true);
