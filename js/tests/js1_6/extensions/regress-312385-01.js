




































var gTestfile = 'regress-312385-01.js';

var BUGNUMBER = 312385;
var summary = 'Generic methods with null or undefined |this|';
var actual = '';
var expect = true;
var voids = [null, undefined];

var generics = {
  String: [{ quote: [] },
{ substring: [] },
{ toLowerCase: [] },
{ toUpperCase: [] },
{ charAt: [] },
{ charCodeAt: [] },
{ indexOf: [] },
{ lastIndexOf: [] },
{ toLocaleLowerCase: [] },
{ toLocaleUpperCase: [] },
{ localeCompare: [] },
{ match: [/(?:)/] }, 
{ search: [] },
{ replace: [] },
{ split: [] },
{ substr: [] },
{ concat: [] },
{ slice: [] }],

  Array:  [{ join: [] },
{ reverse: [] },
{ sort: [] },
           
           
           
{ unshift: [] },
           
{ concat: [] },
{ indexOf: [] },
{ lastIndexOf: [] },
           
           
{ map: [noop] },      
{ filter: [noop] },   
{ some: [noop] },     
{ every: [noop] }     
    ]
};

printBugNumber(BUGNUMBER);
printStatus (summary);

for (var c in generics)
{
  var methods = generics[c];
  for (var i = 0; i < methods.length; i++)
  {
    var method = methods[i];

    for (var methodname in method)
    {
      for (var v = 0; v < voids.length; v++)
      {
	var lhs = c + '.' + methodname +
	  '(' + voids[v] + (method[methodname].length ?(', ' + method[methodname].toString()):'') + ')';

	var rhs = c + '.prototype.' + methodname +
	  '.apply(' + voids[v] + ', ' + method[methodname].toSource() + ')';

	var expr = lhs + ' == ' + rhs;
	printStatus('Testing ' + expr);

	try
	{
	  printStatus('lhs ' + lhs + ': ' + eval(lhs));
	}
	catch(ex)
	{
	  printStatus(ex + '');
	}

	try
	{
	  printStatus('rhs ' + rhs + ': ' + eval(rhs));
	}
	catch(ex)
	{
	  printStatus(ex + '');
	}

	try
	{
	  actual = comparelr(eval(lhs), eval(rhs));
	}
	catch(ex)
	{
	  actual = ex + '';
	}
	reportCompare(expect, actual, expr);
	printStatus('');
      }
    }
  }
}

function comparelr(lhs, rhs)
{
 
  if (lhs.constructor.name != 'Array')
  {
    return (lhs == rhs);
  }

  return (lhs.toSource() == rhs.toSource());
}

function noop()
{
}
