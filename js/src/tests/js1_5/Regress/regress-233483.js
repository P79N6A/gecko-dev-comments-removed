






































var gTestfile = 'regress-233483.js';

var BUGNUMBER = 233483;
var summary = 'Don\'t crash with null properties - Browser only';
var actual = 'Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof document != 'undefined')
{ 
  
  gDelayTestDriverEnd = true;
  window.onload = onLoad;
}
else
{
  actual = 'No Crash';
  reportCompare(expect, actual, summary);
}

function onLoad() {
  setform();
  var a=new Array();
  var forms = document.getElementsByTagName('form');
  a[a.length]=forms[0];
  var s=a.toString();

  actual = 'No Crash';

  reportCompare(expect, actual, summary);
  gDelayTestDriverEnd = false;
  jsTestDriverEnd();

}

function setform()
{
  var form = document.body.appendChild(document.createElement('form'));
  var input = form.appendChild(document.createElement('input'));
  input.setAttribute('id', 'test');
  input.setAttribute('value', '1232');

  var result = form.toString();

}
