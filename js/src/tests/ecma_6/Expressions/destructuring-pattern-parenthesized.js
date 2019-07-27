



var BUGNUMBER = 1146136;
var summary =
  'Parenthesized "destructuring patterns" are not usable as destructuring ' +
  'patterns';

print(BUGNUMBER + ": " + summary);






var savedEval = this[String.fromCharCode(101, 118, 97, 108)];

function checkError(code, nonstrictErr, strictErr)
{
  function helper(exec, prefix, err)
  {
    var fullCode = prefix + code;
    try
    {
      var f = exec(fullCode);

      var error =
        "no early error, parsed code <" + fullCode + "> using " + exec.name;
      if (typeof f === "function")
      {
        try
        {
          f();
          error += ", and the function can be called without error";
        }
        catch (e)
        {
          error +=", and calling the function throws " + e;
        }
      }

      throw new Error(error);
    }
    catch (e)
    {
      assertEq(e instanceof err, true,
               "expected " + err.name + ", got " + e + " " +
               "for code <" + fullCode + "> when parsed using " + exec.name);
    }
  }

  helper(Function, "", nonstrictErr);
  helper(Function, "'use strict'; ", strictErr);
  helper(savedEval, "", nonstrictErr);
  helper(savedEval, "'use strict'; ", strictErr);
}




checkError("var a, b; ([a, b]) = [1, 2];", ReferenceError, ReferenceError);
checkError("var a, b; ({a, b}) = { a: 1, b: 2 };", ReferenceError, ReferenceError);









checkError("var a, b; ({ a: ({ b: b }) } = { a: { b: 42 } });", SyntaxError, SyntaxError);
checkError("var a, b; ({ a: { b: (b = 7) } } = { a: {} });", SyntaxError, SyntaxError);
checkError("var a, b; ({ a: ([b]) } = { a: [42] });", SyntaxError, SyntaxError);
checkError("var a, b; [(a = 5)] = [1];", SyntaxError, SyntaxError);
checkError("var a, b; ({ a: (b = 7)} = { b: 1 });", SyntaxError, SyntaxError);

Function("var a, b; [(a), b] = [1, 2];")();
Function("var a, b; [(a) = 5, b] = [1, 2];")();
Function("var a, b; [(arguments), b] = [1, 2];")();
Function("var a, b; [(arguments) = 5, b] = [1, 2];")();
Function("var a, b; [(eval), b] = [1, 2];")();
Function("var a, b; [(eval) = 5, b] = [1, 2];")();

var repair = {}, demolition = {};

Function("var a, b; [(repair.man), b] = [1, 2];")();
Function("var a, b; [(demolition['man']) = 'motel', b] = [1, 2];")();
Function("var a, b; [(demolition['man' + {}]) = 'motel', b] = [1, 2];")(); 

function classesEnabled()
{
  try
  {
    new Function("class B { constructor() { } }; class D extends B { constructor() { super(); } }");
    return true;
  }
  catch (e if e instanceof SyntaxError)
  {
    return false;
  }
}

if (classesEnabled())
{
  Function("var a, b; var obj = { x() { [(super.man), b] = [1, 2]; } };")();
  Function("var a, b; var obj = { x() { [(super[8]) = 'motel', b] = [1, 2]; } };")();
  Function("var a, b; var obj = { x() { [(super[8 + {}]) = 'motel', b] = [1, 2]; } };")(); 
}



checkError("var a, b; [(repair.man = 17)] = [1];", SyntaxError, SyntaxError);
checkError("var a, b; [(demolition['man'] = 'motel')] = [1, 2];", SyntaxError, SyntaxError);
checkError("var a, b; [(demolition['man' + {}] = 'motel')] = [1];", SyntaxError, SyntaxError); 
if (classesEnabled())
{
  checkError("var a, b; var obj = { x() { [(super.man = 5)] = [1]; } };", SyntaxError, SyntaxError);
  checkError("var a, b; var obj = { x() { [(super[8] = 'motel')] = [1]; } };", SyntaxError, SyntaxError);
  checkError("var a, b; var obj = { x() { [(super[8 + {}] = 'motel')] = [1]; } };", SyntaxError, SyntaxError); 
}





checkError("var a, b; [f() = 'ohai', b] = [1, 2];", SyntaxError, ReferenceError);
checkError("var a, b; [(f()) = 'kthxbai', b] = [1, 2];", SyntaxError, ReferenceError);

Function("var a, b; ({ a: (a), b} = { a: 1, b: 2 });")();
Function("var a, b; ({ a: (a) = 5, b} = { a: 1, b: 2 });")();




if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
