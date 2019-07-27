if (!("startgc" in this &&
      "offThreadCompileScript" in this &&
      "runOffThreadScript" in this))
{
    quit();
}

if (helperThreadCount() == 0)
    quit();

if ("gczeal" in this)
   gczeal(0);


startgc(0);
var g = newGlobal();


if ("gcstate" in this)
   assertEq(gcstate(), "mark");
g.offThreadCompileScript('23;', {});


assertEq(23, g.runOffThreadScript());
if ("gcstate" in this)
   assertEq(gcstate(), "none");

print("done");
