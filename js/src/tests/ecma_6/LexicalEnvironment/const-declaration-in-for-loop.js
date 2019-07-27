




var gTestfile = "const-declaration-in-for-loop.js";

var BUGNUMBER = 1146644;
var summary =
  "Don't crash compiling a non-body-level for-loop whose loop declaration is " +
  "a const";

print(BUGNUMBER + ": " + summary);










var executeGlobalScript = typeof evaluate === "function"
                        ? evaluate
                        : function(s) {};

for (const a1 = 3; false; )
  continue;

Function(`for (const a2 = 3; false; )
            continue;
         `)();

if (true)
{
  for (const a3 = 3; false; )
    continue;
}

Function(`if (true)
          {
            for (const a4 = 3; false; )
              continue;
          }`)();





try
{

executeGlobalScript(`for (const a5 of [])
                       continue;`);

Function(`for (const a6 of [])
            continue;`)();

executeGlobalScript(`if (true)
                     {
                       for (const a7 of [])
                         continue;
                     }`);

Function(`if (true)
          {
            for (const a8 of [])
              continue;
          }`)();

executeGlobalScript(`for (const a9 in {})
                       continue;`);

Function(`for (const a10 in {})
            continue;`)();

executeGlobalScript(`if (true)
                     {
                       for (const a11 in {})
                         continue;
                     }`);

Function(`if (true)
          {
            for (const a12 in {})
              continue;
          }`)();

throw new Error("Congratulations on making for (const … in/of …) work!  " +
                "Please remove the try/catch and this throw.");
}
catch (e)
{
  assertEq(e instanceof SyntaxError, true,
           "unexpected error: expected SyntaxError, got " + e);
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
