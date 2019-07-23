





































gTestfile = 'regress-352097.js';


var BUGNUMBER     = "352097";
var summary = "Avoid adding unnecessary spaces to PIs";
var actual, expect;

printBugNumber(BUGNUMBER);
START(summary);





var failed = false;

function assertContains(func, str)
{
  if (func.toString().indexOf(str) < 0)
    throw func.toString() + " does not contain " + str + "!";
}

try
{
  var f = new Function("return <?f?>;");
  assertContains(f, "<?f?>");

  var g = new Function("return <?f ?>;");
  assertContains(g, "<?f?>");

  var h = new Function("return <?f       ?>;");
  assertContains(h, "<?f?>");

  var i = new Function("return <?f      i?>;");
  assertContains(i, "<?f i?>");

  var j = new Function("return <?f i ?>;");
  assertContains(j, "<?f i ?>");

  var k = new Function("return <?f i      ?>;");
  assertContains(k, "<?f i      ?>");

  var m = new Function("return <?f      i ?>;");
  assertContains(m, "<?f i ?>");
}
catch (ex)
{
  failed = ex;
}

expect = false;
actual = failed;

TEST(1, expect, actual);
