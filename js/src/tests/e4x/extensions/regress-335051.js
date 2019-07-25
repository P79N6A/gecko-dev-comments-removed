








var BUGNUMBER = 335051;
var summary = '';
var actual = '';
var expect = '';



test();

function testSyntax(syntax, isValid) {
  var result;
  try {
    eval(syntax);
    result = true;
  } catch(exception) {
    if (SyntaxError.prototype.isPrototypeOf(exception)) {
      result = false;
    }
  }
  reportCompare(isValid, result, "test " + (isValid?"":"in") + "valid syntax: " + syntax);
}

function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
  var oldVersion = false;

  testSyntax("#1={}", true);
  testSyntax("#1=[]", true);
  testSyntax("#1=([])", true);

  testSyntax("#1=[1, 2, 3]", true);
  testSyntax("#1={a:1, b:2}", true);

  testSyntax("#1=function() { }", true);
  testSyntax("#1=(new Date())", true);

  testSyntax("#1=<a/>", true);
  testSyntax("#1=<!-- -->", true);
  testSyntax("#1=<a>b</a>", true);
  testSyntax("[#1=<a>b</a>, #1#]", true);
  testSyntax("#1=(<a/>)", true);
  testSyntax("#1=(<![CDATA[foo]]>)", true);

  testSyntax("#1=123", false);
  testSyntax("#1='foo'", false);
  testSyntax("#1=1+2", false);
  testSyntax("#1=-1", false);
  testSyntax("#1=(123)", false);
  testSyntax("#1=true", false);

  exitFunc ('test');
}
