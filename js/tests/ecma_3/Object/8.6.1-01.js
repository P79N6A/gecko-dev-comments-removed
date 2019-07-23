





































var bug = 315436;
var summary = 'In strict mode, setting a read-only property should generate a warning';

printBugNumber (bug);
printStatus (summary);

enterFunc (String (bug));


var actual = '';
var expect = 's.length is read-only';
var status = summary + ': Throw if STRICT and WERROR is enabled';

options('strict');
options('werror');

try 
{ 
  var s = new String ('abc');
  s.length = 0;
} 
catch (e) 
{ 
  actual = e.message;
}

reportCompare(expect, actual, status);



actual = 'did not throw';
expect = 'did not throw';
var status = summary + ': Do not throw if STRICT is enabled and WERROR is disabled';


options('werror');

try 
{ 
  s.length = 0;
} 
catch (e) 
{ 
  actual = e.message;
}

reportCompare(expect, actual, status);



actual = 'did not throw';
expect = 'did not throw';
var status = summary + ': Do not throw if not in strict mode';


options('strict');

try 
{ 
  s.length = 0;
} 
catch (e) 
{ 
  actual = e.message;
}

reportCompare(expect, actual, status);
