







































var gTestfile = 'regress-50447-1.js';













var BUGNUMBER = 50447;
var summary = 'Test (non-ECMA) Error object properties fileName, lineNumber';



test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  testRealError();
  test1();
  test2();
  test3();
  test4();

  exitFunc('test');
}


function testRealError()
{
  
  enterFunc ("testRealError");

  try
  {
    blabla;
  }
  catch (e)
  {
    if (e.fileName.search (/-50447-1\.js$/i) == -1)
      reportCompare('PASS', 'FAIL', "expected fileName to end with '-50447-1.js'");

    reportCompare(87, e.lineNumber,
		  "lineNumber property returned unexpected value.");
  }

  exitFunc ("testRealError");
}


function test1()
{
  
  enterFunc ("test1");

  var e = new InternalError ("msg", "file", 2);
  reportCompare ("(new InternalError(\"msg\", \"file\", 2))",
		 e.toSource(),
		 "toSource() returned unexpected result.");
  reportCompare ("file", e.fileName,
		 "fileName property returned unexpected value.");
  reportCompare (2, e.lineNumber,
		 "lineNumber property returned unexpected value.");

  exitFunc ("test1");
}


function test2()
{
  
  enterFunc ("test2");

  



  var expectedLine = 141;
  var expectedFileName = 'js1_5/extensions/regress-50447-1.js';
  if (typeof document == "undefined")
  {
    expectedFileName = './' + expectedFileName;
  }
  else
  {
    expectedFileName = document.location.href.
      replace(/[^\/]*(\?.*)$/, '') +
      expectedFileName;
  }
  var e = new InternalError ("msg");
  reportCompare ("(new InternalError(\"msg\", \"" +
		 expectedFileName + "\", " + expectedLine + "))",
		 e.toSource(),
		 "toSource() returned unexpected result.");
  reportCompare (expectedFileName, e.fileName,
		 "fileName property returned unexpected value.");
  reportCompare (expectedLine, e.lineNumber,
		 "lineNumber property returned unexpected value.");

  exitFunc ("test2");
}


function test3()
{
  

  




  enterFunc ("test3");

  var expectedFileName = 'js1_5/extensions/regress-50447-1.js';
  if (typeof document == "undefined")
  {
    expectedFileName = './' + expectedFileName;
  }
  else
  {
    expectedFileName = document.location.href.
      replace(/[^\/]*(\?.*)$/, '') +
      expectedFileName;
  }

  var e = new InternalError ("msg");
  e.lineNumber = 10;
  reportCompare ("(new InternalError(\"msg\", \"" +
		 expectedFileName + "\", 10))",
		 e.toSource(),
		 "toSource() returned unexpected result.");
  reportCompare (expectedFileName, e.fileName,
		 "fileName property returned unexpected value.");
  reportCompare (10, e.lineNumber,
		 "lineNumber property returned unexpected value.");

  exitFunc ("test3");
}


function test4()
{
  
  enterFunc ("test4");

  var expectedLine = 200;

  var e = new InternalError ("msg", "file");
  reportCompare ("(new InternalError(\"msg\", \"file\", " + expectedLine + "))",
		 e.toSource(),
		 "toSource() returned unexpected result.");
  reportCompare ("file", e.fileName,
		 "fileName property returned unexpected value.");
  reportCompare (expectedLine, e.lineNumber,
		 "lineNumber property returned unexpected value.");

  exitFunc ("test4");
}
