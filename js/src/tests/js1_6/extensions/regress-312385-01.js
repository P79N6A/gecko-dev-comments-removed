





































var BUGNUMBER = 312385;
var summary = 'Generic methods with null or undefined |this|';
var actual = '';
var expect = true;
var voids = [null, undefined];


function noop() { }

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

var global = this;

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
        var Constructor = global[c]

        var argsLen = method[methodname].length;
        assertEq(argsLen === 0 || argsLen === 1, true, "not all arities handled");

        var generic = Constructor[methodname];
        var prototypy = Constructor.prototype[methodname];

        assertEq(typeof generic, "function");
        assertEq(typeof prototypy, "function");

        

        try
        {
          switch (method[methodname].length)
          {
            case 0:
              generic(voids[v]);
              break;

            case 1:
              generic(voids[v], method[methodname][0]);
              break;
          }
          throw new Error(c + "." + methodname + " must throw for null or " +
                          "undefined first argument");
        }
        catch (e)
        {
          assertEq(e instanceof TypeError, true,
                   "Didn't get a TypeError for " + c + "." + methodname +
                   " called with null or undefined first argument");
        }


        

        try
        {
          prototypy.apply(voids[v], method[methodname][0]);
          throw new Error(c + ".prototype." + methodname + " must throw " +
                          "for null or undefined this");
        }
        catch (e)
        {
          assertEq(e instanceof TypeError, true,
                   c + ".prototype." + methodname + "didn't throw a " +
                   "TypeError when called with null or undefined this");
        }
      }
    }
  }
}

if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests finished.");
