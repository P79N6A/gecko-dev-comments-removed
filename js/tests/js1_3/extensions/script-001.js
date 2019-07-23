











































































































































var SECTION = "script-001";
var VERSION = "JS1_3";
var TITLE   = "NativeScript";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

if (typeof Script == 'undefined')
{
  print('Test skipped. Script not defined.');
}
else
{
  var s = new Script();
  s.getJSClass = Object.prototype.toString;

  new TestCase( SECTION,
                "var s = new Script(); typeof s",
                "function",
                typeof s );

  new TestCase( SECTION,
                "s.getJSClass()",
                "[object Script]",
                s.getJSClass() );
}

test();
