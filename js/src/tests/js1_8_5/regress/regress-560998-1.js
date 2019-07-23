



for (let j = 0; j < 4; ++j) {
  function g() { j; }
  g();
}

reportCompare(0, 0, "ok");
