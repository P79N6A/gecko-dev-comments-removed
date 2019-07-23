




































var bug = 351448;
var summary = 'RegExp - throw InternalError on too complex regular expressions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var strings = [
    "/.X(.+)+X/.exec('bbbbXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')",
    "/.X(.+)+X/.exec('bbbbXcXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')",
    "/.X(.+)+XX/.exec('bbbbXXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.X(.+)+XX/.exec('bbbbXcXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')",
    "/.X(.+)+[X]/.exec('bbbbXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.X(.+)+[X]/.exec('bbbbXcXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.X(.+)+[X][X]/.exec('bbbbXXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.X(.+)+[X][X]/.exec('bbbbXcXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.XX(.+)+X/.exec('bbbbXXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.XX(.+)+X/.exec('bbbbXXcXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')",
    "/.XX(.+)+X/.exec('bbbbXXcXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')",
    "/.XX(.+)+[X]/.exec('bbbbXXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.XX(.+)+[X]/.exec('bbbbXXcXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.[X](.+)+[X]/.exec('bbbbXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.[X](.+)+[X]/.exec('bbbbXcXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.[X](.+)+[X][X]/.exec('bbbbXXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.[X](.+)+[X][X]/.exec('bbbbXcXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.[X][X](.+)+[X]/.exec('bbbbXXXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')", 
    "/.[X][X](.+)+[X]/.exec('bbbbXXcXaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')"
    ];

  expect = 'InternalError: regular expression too complex';
  var jsOptions = new JavaScriptOptions();

  for (var i = 0; i < strings.length; i++)
  {
    jsOptions.setOption('relimit', true);
    try
    {
    eval(strings[i]);
    }
    catch(ex)
    {
      actual = ex + '';
    }
    jsOptions.reset();
    reportCompare(expect, actual, summary + ': ' + strings[i]);
  }

  exitFunc ('test');
}
