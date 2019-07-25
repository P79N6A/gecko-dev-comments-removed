





var BUGNUMBER = 497869;
var summary = "Implement FutureReservedWords per-spec";

print(BUGNUMBER + ": " + summary);





var futureReservedWords =
  [
   "class",
   
   "enum",
   "export",
   "extends",
   "import",
   "let",  
   "super",
  ];

var strictFutureReservedWords =
  [
   "implements",
   "interface",
   "package",
   "private",
   "protected",
   "public",
   "static",
   "yield", 
  ];

function testWord(word, wordKind, expectNormal, expectStrict)
{
  var actual, status;

  

  actual = "";
  status = summary + ": " + word + ": normal assignment";
  try
  {
    eval(word + " = 'foo';");
    actual = "no error";
  }
  catch(e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ": " + word + ": strict assignment";
  try
  {
    eval("'use strict'; " + word + " = 'foo';");
    actual = "no error";
  }
  catch(e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  

  actual = "";
  status = summary + ": " + word + ": normal var";
  try
  {
    eval("var " + word + ";");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ": " + word + ": strict var";
  try
  {
    eval("'use strict'; var " + word + ";");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  

  actual = "";
  status = summary + ": " + word + ": normal for-in var";
  try
  {
    eval("for (var " + word + " in {});");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ": " + word + ": strict for-in var";
  try
  {
    eval("'use strict'; for (var " + word + " in {});");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  

  actual = "";
  status = summary + ": " + word + ": normal var";
  try
  {
    eval("try { } catch (" + word + ") { }");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ": " + word + ": strict var";
  try
  {
    eval("'use strict'; try { } catch (" + word + ") { }");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  

  actual = "";
  status = summary + ": " + word + ": normal label";
  try
  {
    eval(word + ": while (false);");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ": " + word + ": strict label";
  try
  {
    eval("'use strict'; " + word + ": while (false);");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  

  actual = "";
  status = summary + ": " + word + ": normal function argument";
  try
  {
    eval("function foo(" + word + ") { }");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ": " + word + ": strict function argument";
  try
  {
    eval("'use strict'; function foo(" + word + ") { }");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  actual = "";
  status = summary + ": " + word + ": function argument retroactively strict";
  try
  {
    eval("function foo(" + word + ") { 'use strict'; }");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  

  actual = "";
  status = summary + ": " + word + ": normal function expression argument";
  try
  {
    eval("var s = (function foo(" + word + ") { });");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ": " + word + ": strict function expression argument";
  try
  {
    eval("'use strict'; var s = (function foo(" + word + ") { });");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  actual = "";
  status = summary + ": " + word + ": function expression argument retroactively strict";
  try
  {
    eval("var s = (function foo(" + word + ") { 'use strict'; });");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  

  actual = "";
  status = summary + ": " + word + ": argument with normal Function";
  try
  {
    Function(word, "return 17");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ": " + word + ": argument with strict Function";
  try
  {
    Function(word, "'use strict'; return 17");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  

  actual = "";
  status = summary + ": " + word + ": normal property setter argument";
  try
  {
    eval("var o = { set x(" + word + ") { } };");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ": " + word + ": strict property setter argument";
  try
  {
    eval("'use strict'; var o = { set x(" + word + ") { } };");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  actual = "";
  status = summary + ": " + word + ": property setter argument retroactively strict";
  try
  {
    eval("var o = { set x(" + word + ") { 'use strict'; } };");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  

  if (wordKind !== "reserved")
  {
    actual = "";
    status = summary + ": " + word + ": normal function name";
    try
    {
      eval("function " + word + "() { }");
      actual = "no error";
    }
    catch (e)
    {
      actual = e.name;
      status +=  ", " + e.name + ": " + e.message + " ";
    }
    reportCompare(expectNormal, actual, status);
  }

  actual = "";
  status = summary + ": " + word + ": strict function name";
  try
  {
    eval("'use strict'; function " + word + "() { }");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  actual = "";
  status = summary + ": " + word + ": function name retroactively strict";
  try
  {
    eval("function " + word + "() { 'use strict'; }");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  

  if (wordKind !== "reserved")
  {
    actual = "";
    status = summary + ": " + word + ": normal function expression name";
    try
    {
      eval("var s = (function " + word + "() { });");
      actual = "no error";
    }
    catch (e)
    {
      actual = e.name;
      status +=  ", " + e.name + ": " + e.message + " ";
    }
    reportCompare(expectNormal, actual, status);
  }

  actual = "";
  status = summary + ": " + word + ": strict function expression name";
  try
  {
    eval("'use strict'; var s = (function " + word + "() { });");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);

  actual = "";
  status = summary + ": " + word + ": function expression name retroactively strict";
  try
  {
    eval("var s = (function " + word + "() { 'use strict'; });");
    actual = "no error";
  }
  catch (e)
  {
    actual = e.name;
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);
}

function testFutureReservedWord(word)
{
  testWord(word, "reserved", "SyntaxError", "SyntaxError");
}

function testStrictFutureReservedWord(word)
{
  testWord(word, "strict reserved", "no error", "SyntaxError");
}

futureReservedWords.forEach(testFutureReservedWord);
strictFutureReservedWords.forEach(testStrictFutureReservedWord);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
