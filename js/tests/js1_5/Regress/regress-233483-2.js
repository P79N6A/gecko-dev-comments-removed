






































var gTestfile = 'regress-233483-2.js';

var BUGNUMBER = 233483;
var summary = 'Don\'t crash with null properties - Browser only';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof document == 'undefined')
{
  reportCompare(expect, actual, summary);
}
else
{ 
  
  gDelayTestDriverEnd = true;

  actual = 'Crash';
  window.onload = onLoad;
}

function onLoad()
{
  var a = new Array();
  var pe;
  var x;
  var s;

  setform();

  for (pe=document.getElementById("test"); pe; pe=pe.parentNode)
  {
    a[a.length] = pe;
  }

  
  s = a.toString();

  actual = 'No Crash';

  reportCompare(expect, actual, summary);

  gDelayTestDriverEnd = false;
  jsTestDriverEnd();
}

function setform()
{
  var form  = document.body.appendChild(document.createElement('form'));
  var table = form.appendChild(document.createElement('table'));
  var tbody = table.appendChild(document.createElement('tbody'));
  var tr    = tbody.appendChild(document.createElement('tr'));
  var td    = tr.appendChild(document.createElement('td'))
    var input = td.appendChild(document.createElement('input'));

  input.setAttribute('id', 'test');
  input.setAttribute('value', '1232');

}
