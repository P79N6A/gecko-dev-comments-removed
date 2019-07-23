





































var bug = 315436;
var summary = 'In strict mode, setting a read-only property should generate a warning';

printBugNumber (bug);
printStatus (summary);

enterFunc (String (bug));


var actual = '';
var expect = 's.length is read-only';
var status = summary + ': Throw if STRICT and WERROR is enabled';

var jsOptions = new JavaScriptOptions();

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);
try 
{ 
  var s = new String ('abc');
  s.length = 0;
} 
catch (e) 
{ 
  actual = e.message;
}
jsOptions.reset();

reportCompare(expect, actual, status);



actual = 'did not throw';
expect = 'did not throw';
var status = summary + ': Do not throw if STRICT is enabled and WERROR is disabled';

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', false);
try 
{ 
  s.length = 0;
} 
catch (e) 
{ 
  actual = e.message;
}
jsOptions.reset();
reportCompare(expect, actual, status);



actual = 'did not throw';
expect = 'did not throw';
var status = summary + ': Do not throw if not in strict mode';

jsOptions.setOption('strict', false);
jsOptions.setOption('werror', false);
try 
{ 
  s.length = 0;
} 
catch (e) 
{ 
  actual = e.message;
}
jsOptions.reset();
reportCompare(expect, actual, status);

exitFunc (String (bug));

