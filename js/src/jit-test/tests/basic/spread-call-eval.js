load(libdir + "asserts.js");

assertEq(eval(...[]), undefined);
assertEq(eval(...["1 + 2"]), 3);

let a = 10, b = 1;
assertEq(eval(...["a + b"]), 11);

(function() {
  let a = 20;
  assertEq(eval(...["a + b"]), 21);
})();

with ({ a: 30 }) {
  assertEq(eval(...["a + b"]), 31);
}

let line0 = Error().lineNumber;
try {             
  eval(...["("]); 
} catch (e) {
  assertEq(e.lineNumber, line0 + 2);
}


assertEq(eval(...["a + b"].iterator()), 11);
assertEq(eval(...Set(["a + b"])), 11);
let itr = {
  iterator: function() {
    return {
      i: 0,
      next: function() {
        this.i++;
        if (this.i == 1)
          return "a + b";
        else
          throw StopIteration;
      }
    };
  }
};
assertEq(eval(...itr), 11);
let gen = {
  iterator: function() {
    yield "a + b";
  }
};
assertEq(eval(...gen), 11);

let c = ["C"], d = "D";
assertEq(eval(...c=["c[0] + d"]), "c[0] + dD");






assertThrowsInstanceOf(() => eval("a + b", ...null), TypeError);
assertThrowsInstanceOf(() => eval("a + b", ...undefined), TypeError);
