




































var bug = 325269;
var summary = 'GC hazard in js_ConstructObject';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);



  
var SavedArray = Array;

function Redirector() { }

Redirector.prototype = 1;
Redirector.__defineGetter__('prototype', function() {

        gc();
        return SavedArray.prototype;
});


Array = Function('');
Array.prototype = 1;
Array.__defineGetter__('prototype', function() { 

        Array = Redirector;
        gc();
        new Object();
        new Object();
        return undefined;
});

new Object();

try
{
  var y = "test".split('');
}
catch(ex)
{
  printStatus(ex + '');
}


reportCompare(expect, actual, summary);
