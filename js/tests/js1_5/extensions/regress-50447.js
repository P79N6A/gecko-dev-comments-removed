













































 

var bug = 50447;
var summary = 'Test (non-ECMA) Error object properties fileName, lineNumber';



test();



function test()
{
  enterFunc ('test');
  printBugNumber (bug);
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
        if (e.fileName.search (/-50447\.js$/i) == -1)
            reportFailure ("expected fileName to end with '-50447.js'");

        reportCompare (81, e.lineNumber,
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

    var e = new InternalError ("msg");
    reportCompare ("(new InternalError(\"msg\", \"\"))",
                   e.toSource(),
                   "toSource() returned unexpected result.");
    reportCompare ("", e.fileName,
                   "fileName property returned unexpected value.");
    reportCompare (0, e.lineNumber,
                   "lineNumber property returned unexpected value.");

    exitFunc ("test2");
}


function test3()
{
    
    enterFunc ("test3");

    var e = new InternalError ("msg");
    e.lineNumber = 10;
    reportCompare ("(new InternalError(\"msg\", \"\", 10))",
                   e.toSource(),
                   "toSource() returned unexpected result.");
    reportCompare ("", e.fileName,
                   "fileName property returned unexpected value.");
    reportCompare (10, e.lineNumber,
                   "lineNumber property returned unexpected value.");

    exitFunc ("test3");
}


function test4()
{
    
    enterFunc ("test4");

    var e = new InternalError ("msg", "file");
    reportCompare ("(new InternalError(\"msg\", \"file\"))",
                   e.toSource(),
                   "toSource() returned unexpected result.");
    reportCompare ("file", e.fileName,
                   "fileName property returned unexpected value.");
    reportCompare (0, e.lineNumber,
                   "lineNumber property returned unexpected value.");

    exitFunc ("test4");
}
