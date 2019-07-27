






function run_test() {
  let ns = {};
  Components.utils.import("resource://testing-common/Assert.jsm", ns);
  let assert = new ns.Assert();

  function makeBlock(f, ...args) {
    return function() {
      return f.apply(assert, args);
    };
  }

  function protoCtrChain(o) {
    let result = [];
    while (o = o.__proto__) {
      result.push(o.constructor);
    }
    return result.join();
  }

  function indirectInstanceOf(obj, cls) {
    if (obj instanceof cls) {
      return true;
    }
    let clsChain = protoCtrChain(cls.prototype);
    let objChain = protoCtrChain(obj);
    return objChain.slice(-clsChain.length) === clsChain;
  };

  assert.ok(indirectInstanceOf(ns.Assert.AssertionError.prototype, Error),
            "Assert.AssertionError instanceof Error");

  assert.throws(makeBlock(assert.ok, false),
                ns.Assert.AssertionError, "ok(false)");

  assert.ok(true, "ok(true)");

  assert.ok("test", "ok('test')");

  assert.throws(makeBlock(assert.equal, true, false), ns.Assert.AssertionError, "equal");

  assert.equal(null, null, "equal");

  assert.equal(undefined, undefined, "equal");

  assert.equal(null, undefined, "equal");

  assert.equal(true, true, "equal");

  assert.notEqual(true, false, "notEqual");

  assert.throws(makeBlock(assert.notEqual, true, true),
                ns.Assert.AssertionError, "notEqual");

  assert.throws(makeBlock(assert.strictEqual, 2, "2"),
                ns.Assert.AssertionError, "strictEqual");

  assert.throws(makeBlock(assert.strictEqual, null, undefined),
                ns.Assert.AssertionError, "strictEqual");

  assert.notStrictEqual(2, "2", "notStrictEqual");

  
  
  assert.deepEqual(new Date(2000, 3, 14), new Date(2000, 3, 14), "deepEqual date");
  assert.deepEqual(new Date(NaN), new Date(NaN), "deepEqual invalid dates");

  assert.throws(makeBlock(assert.deepEqual, new Date(), new Date(2000, 3, 14)),
                ns.Assert.AssertionError,
                "deepEqual date");

  
  assert.deepEqual(/a/, /a/);
  assert.deepEqual(/a/g, /a/g);
  assert.deepEqual(/a/i, /a/i);
  assert.deepEqual(/a/m, /a/m);
  assert.deepEqual(/a/igm, /a/igm);
  assert.throws(makeBlock(assert.deepEqual, /ab/, /a/));
  assert.throws(makeBlock(assert.deepEqual, /a/g, /a/));
  assert.throws(makeBlock(assert.deepEqual, /a/i, /a/));
  assert.throws(makeBlock(assert.deepEqual, /a/m, /a/));
  assert.throws(makeBlock(assert.deepEqual, /a/igm, /a/im));

  let re1 = /a/;
  re1.lastIndex = 3;
  assert.throws(makeBlock(assert.deepEqual, re1, /a/));

  
  assert.deepEqual(4, "4", "deepEqual == check");
  assert.deepEqual(true, 1, "deepEqual == check");
  assert.throws(makeBlock(assert.deepEqual, 4, "5"),
                ns.Assert.AssertionError,
                "deepEqual == check");

  
  
  assert.deepEqual({a: 4}, {a: 4});
  assert.deepEqual({a: 4, b: "2"}, {a: 4, b: "2"});
  assert.deepEqual([4], ["4"]);
  assert.throws(makeBlock(assert.deepEqual, {a: 4}, {a: 4, b: true}),
                ns.Assert.AssertionError);
  assert.deepEqual(["a"], {0: "a"});

  let a1 = [1, 2, 3];
  let a2 = [1, 2, 3];
  a1.a = "test";
  a1.b = true;
  a2.b = true;
  a2.a = "test";
  assert.throws(makeBlock(assert.deepEqual, Object.keys(a1), Object.keys(a2)),
                ns.Assert.AssertionError);
  assert.deepEqual(a1, a2);

  let nbRoot = {
    toString: function() { return this.first + " " + this.last; }
  };

  function nameBuilder(first, last) {
    this.first = first;
    this.last = last;
    return this;
  }
  nameBuilder.prototype = nbRoot;

  function nameBuilder2(first, last) {
    this.first = first;
    this.last = last;
    return this;
  }
  nameBuilder2.prototype = nbRoot;

  let nb1 = new nameBuilder("Ryan", "Dahl");
  let nb2 = new nameBuilder2("Ryan", "Dahl");

  assert.deepEqual(nb1, nb2);

  nameBuilder2.prototype = Object;
  nb2 = new nameBuilder2("Ryan", "Dahl");
  assert.throws(makeBlock(assert.deepEqual, nb1, nb2), ns.Assert.AssertionError);

  
  assert.throws(makeBlock(assert.deepEqual, "a", {}), ns.Assert.AssertionError);

  
  function thrower(errorConstructor) {
    throw new errorConstructor("test");
  }
  let aethrow = makeBlock(thrower, ns.Assert.AssertionError);
  aethrow = makeBlock(thrower, ns.Assert.AssertionError);

  
  assert.throws(makeBlock(thrower, ns.Assert.AssertionError),
                ns.Assert.AssertionError, "message");
  assert.throws(makeBlock(thrower, ns.Assert.AssertionError), ns.Assert.AssertionError);
  assert.throws(makeBlock(thrower, ns.Assert.AssertionError));

  
  assert.throws(makeBlock(thrower, TypeError));

  
  let threw = false;
  try {
    assert.throws(makeBlock(thrower, TypeError), ns.Assert.AssertionError);
  } catch (e) {
    threw = true;
    assert.ok(e instanceof TypeError, "type");
  }
  assert.equal(true, threw,
               "Assert.throws with an explicit error is eating extra errors",
               ns.Assert.AssertionError);
  threw = false;

  function ifError(err) {
    if (err) {
      throw err;
    }
  }
  assert.throws(function() {
    ifError(new Error("test error"));
  });

  
  threw = false;
  try {
    assert.throws(
      function() {
        throw ({});
      },
      Array
    );
  } catch (e) {
    threw = true;
  }
  assert.ok(threw, "wrong constructor validation");

  
  assert.throws(makeBlock(thrower, TypeError), /test/);

  
  assert.throws(makeBlock(thrower, TypeError), function(err) {
    if ((err instanceof TypeError) && /test/.test(err)) {
      return true;
    }
  });

  function testAssertionMessage(actual, expected) {
    try {
      assert.equal(actual, "");
    } catch (e) {
      assert.equal(e.toString(),
          ["AssertionError:", expected, "==", '""'].join(" "));
    }
  }
  testAssertionMessage(undefined, '"undefined"');
  testAssertionMessage(null, "null");
  testAssertionMessage(true, "true");
  testAssertionMessage(false, "false");
  testAssertionMessage(0, "0");
  testAssertionMessage(100, "100");
  testAssertionMessage(NaN, '"NaN"');
  testAssertionMessage(Infinity, '"Infinity"');
  testAssertionMessage(-Infinity, '"-Infinity"');
  testAssertionMessage("", '""');
  testAssertionMessage("foo", '"foo"');
  testAssertionMessage([], "[]");
  testAssertionMessage([1, 2, 3], "[1,2,3]");
  testAssertionMessage(/a/, '"/a/"');
  testAssertionMessage(/abc/gim, '"/abc/gim"');
  testAssertionMessage(function f() {}, '"function f() {}"');
  testAssertionMessage({}, "{}");
  testAssertionMessage({a: undefined, b: null}, '{"a":"undefined","b":null}');
  testAssertionMessage({a: NaN, b: Infinity, c: -Infinity},
      '{"a":"NaN","b":"Infinity","c":"-Infinity"}');

  
  try {
    assert.throws(function () {
      ifError(null);
    });
  } catch (e) {
    threw = true;
    assert.equal(e.message, "Missing expected exception..");
  }
  assert.ok(threw);

  
  try {
    assert.equal(1, 2);
  } catch (e) {
    assert.equal(e.toString().split("\n")[0], "AssertionError: 1 == 2")
  }

  try {
    assert.equal(1, 2, "oh no");
  } catch (e) {
    assert.equal(e.toString().split("\n")[0], "AssertionError: oh no - 1 == 2")
  }

  
  ok(true, "OK, this went well");
  deepEqual(/a/g, /a/g, "deep equal should work on RegExp");
  deepEqual(/a/igm, /a/igm, "deep equal should work on RegExp");
  deepEqual({a: 4, b: "1"}, {b: "1", a: 4}, "deep equal should work on regular Object");
  deepEqual(a1, a2, "deep equal should work on Array with Object properties");

  
  equal(new ns.Assert.AssertionError({
    actual: {
      toJSON: function() {
        throw "bam!";
      }
    },
    expected: "foo",
    operator: "="
  }).message, "[object Object] = \"foo\"");

  run_next_test();
}

add_task(function* test_rejects() {
  let ns = {};
  Components.utils.import("resource://testing-common/Assert.jsm", ns);
  let assert = new ns.Assert();

  
  function* checkRejectsFails(err, expected) {
    try {
      yield assert.rejects(Promise.reject(err), expected);
      ok(false, "should have thrown");
    } catch(ex) {
      deepEqual(ex, err, "Assert.rejects threw the original unexpected error");
    }
  }

  
  let SomeErrorLikeThing = function() {};

  
  
  yield assert.rejects(Promise.reject(new Error("oh no")));
  yield assert.rejects(Promise.reject("oh no"));

  
  
  yield assert.rejects(Promise.reject(new Error("oh no")), Error, "rejected");
  
  yield assert.rejects(Promise.reject(new Error("oh no")), /oh no/, "rejected");

  
  
  yield checkRejectsFails(new Error("something else"), SomeErrorLikeThing);
  
  yield checkRejectsFails(new Error("something else"), /oh no/);

  
  yield assert.rejects(Promise.reject("oh no"), /oh no/, "rejected");
  
  yield checkRejectsFails("something else", /oh no/);
});
