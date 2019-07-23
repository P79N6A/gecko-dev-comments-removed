





































gTestfile = 'regress-354151-02.js';

var BUGNUMBER = 354151;
var summary = 'Bad assumptions about Array elements';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var xml = <tag xmlns:n="uri:1" xmlns:n2="uri:2" n:a="1" n2:a="1"/>;

function getter() { }

function setter(v)
{
  delete xml.@*::a;
  xml.removeNamespace("uri:2");
  gc();
}

Array.prototype.__defineGetter__(0, getter);
Array.prototype.__defineSetter__(0, setter);

xml.namespaceDeclarations();

delete Array.prototype[0];

TEST(1, expect, actual);

END();
