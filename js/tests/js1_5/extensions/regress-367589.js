




































var bug = 367589;
var summary = 'Do not assert !SPROP_HAS_STUB_SETTER(sprop) || (sprop->attrs & JSPROP_GETTER)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  if (typeof window != 'undefined')
  {
    gDelayTestDriverEnd = true;
    document.write('<button id="button" onclick="document.getElementsByTagName(\'button\')[0] = \'wtf\';">Crash</button>');
    window.addEventListener('load', crash, false); 
  }
  else
  {
    reportCompare(expect, actual, summary);
  }

  exitFunc ('test');
}

function crash()
{
  document.getElementById('button').click();
  gDelayTestDriverEnd = false;
  jsTestDriverEnd();
  reportCompare(expect, actual, summary);
}
