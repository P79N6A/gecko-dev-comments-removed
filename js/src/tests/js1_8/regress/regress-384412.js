




































var gTestfile = 'regress-384412.js';

var BUGNUMBER = 384412;
var summary = 'Exercise frame handling code';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 





  f = (function(n) { for (var i = 0; i != n; i++) yield i });
  g = f(3);
  expect(0, g.next());
  expect(1, g.next());
  expect(2, g.next());
  s = "no exception";
  try { g.next(); } catch (e) { s = e + ""; }
  expect("[object StopIteration]", s);


  f = (function(n) {
      try {
        for (var i = 0; i != n; i++) 
          yield i;
      } finally {
        yield "finally";
      }
    });

  g = f(3);
  expect(0, g.next());
  expect(1, g.next());
  expect(2, g.next());
  expect("finally", g.next());


  g = f(3);
  expect(0, g.next());
  s = "no exception";
  try { g.close(); } catch (e) { s = e + ""; };
  expect("TypeError: yield from closing generator " + f.toSource(), s);





  t = <xml><eins><name>ich</name></eins><eins><name>joki</name></eins></xml>;


  expect(<eins><name>joki</name></eins>, t.eins.(name == "joki"));
  expect(t.eins, t.eins.(t.eins.(true)));
  expect(t.(false), t.eins.(false).(true));


  f = (function() { t.eins.(yield true); });
  g = f();
  s = "no exception";
  try { g.next(); } catch (e) { s = e + ""; }
  expect("no exception", s);


  f = (function() { t.eins.(true); });
  expect(undefined, f());


  f = (function() {
      try {
        return "hallo";
      } finally {
        t.eins.(true);
      }
    });
  expect("hallo", f());





  f = (function() { return arguments[(arguments.length - 1) / 2]; });
  expect(2, f(1, 2, 3));
  expect(2, f.call(null, 1, 2, 3));
  expect(2, f.apply(null, [1, 2, 3]));
  expect("a1c", "abc".replace("b", f));
  s = "no exception";
  try {
    "abc".replace("b", (function() { throw "hello" }));
  } catch (e) {
    s = e + "";
  }
  expect("hello", s);
  expect(6, [1, 2, 3].reduce(function(a, b) { return a + b; }));
  s = "no exception";
  try {
    [1, 2, 3].reduce(function(a, b) { if (b == 2) throw "hello"; });
  } catch (e) {
    s = e + "";
  }
  expect("hello", s);




  o = {};
  s = "no exception";
  try {
    o.hello();
  } catch (e) {
    s = e + "";
  }
  expect("TypeError: o.hello is not a function", s);
  o.__noSuchMethod__ = (function() { return "world"; });
  expect("world", o.hello());
  o.__noSuchMethod__ = 1;
  s = "no exception";
  try {
    o.hello();
  } catch (e) {
    s = e + "";
  }
  expect("TypeError: o.hello is not a function", s);
  o.__noSuchMethod__ = {};
  s = "no exception";
  try {
    o.hello();
  } catch (e) {
    s = e + "";
  }
  expect("TypeError: o.hello() is not a function", s);
  s = "no exception";
  try {
    eval("o.hello()");
  } catch (e) {
    s = e + "";
  }
  expect("TypeError: o.hello() is not a function", s);
  s = "no exception";
  try { [2, 3, 0].sort({}); } catch (e) { s = e + ""; }
  expect("TypeError: [2, 3, 0].sort({}) is not a function", s);




  String.prototype.__iterator__ = (function () {
      




      for (let i = 0; i != 0 + this.length; i++)
        yield this[i];
    });
  expect(["a1", "a2", "a3", "b1", "b2", "b3", "c1", "c2", "c3"] + "",
         ([a + b for (a in 'abc') for (b in '123')]) + "");
  expect("", ([x for (x in <x/>)]) + "");




  if (typeof version == 'function')
  {
    var v = version(150);
    f = new Function("return version(arguments[0])");
    version(v);
    expect(150, f());
    expect(150, eval("f()"));
    expect(0, eval("f(0); f()"));
    version(v);
  }
  print("End of Tests");




  function expect(a, b) {
    print('expect: ' + a + ', actual: ' + b);
    reportCompare(a, b, summary);
  }


  exitFunc ('test');
}
