





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
   "super",
  ];

var strictFutureReservedWords =
  [
   "implements",
   "interface",
   "let", 
   "package",
   "private",
   "protected",
   "public",
   "static",
   "yield", 
  ];

function testWord(word, expectNormal, expectStrict)
{
  var actual, status;

  

  actual = "";
  status = summary + ", normal var: " + word;
  try
  {
    eval("var " + word + ";");
    actual = "no error";
  }
  catch (e)
  {
    actual = "error";
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ", strict var: " + word;
  try
  {
    eval("'use strict'; var " + word + ";");
    actual = "no error";
  }
  catch (e)
  {
    actual = "error";
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);


  

  actual = "";
  status = summary + ", normal assignment: " + word;
  try
  {
    eval(word + " = 'foo';");
    actual = "no error";
  }
  catch(e)
  {
    actual = "error";
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectNormal, actual, status);

  actual = "";
  status = summary + ", strict assignment: " + word;
  try
  {
    eval("'use strict'; " + word + " = 'foo';");
    actual = "no error";
  }
  catch(e)
  {
    actual = "error";
    status +=  ", " + e.name + ": " + e.message + " ";
  }
  reportCompare(expectStrict, actual, status);
}

function testFutureReservedWord(word)
{
  testWord(word, "error", "error");
}

function testStrictFutureReservedWord(word)
{
  testWord(word, "no error", "error");
}

futureReservedWords.forEach(testFutureReservedWord);
strictFutureReservedWords.forEach(testStrictFutureReservedWord);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
