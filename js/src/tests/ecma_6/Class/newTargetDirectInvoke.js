
var func = new Function("new.target");



function assertNewTarget(expected, unused) { assertEq(new.target, expected); }



for (let i = 0; i < 100; i++)
    assertNewTarget(undefined, null);

for (let i = 0; i < 100; i++)
    assertNewTarget(undefined);

for (let i = 0; i < 100; i++)
    assertNewTarget(undefined, null, 1);


for (let i = 0; i < 100; i++)
    assertNewTarget(...[undefined]);

for (let i = 0; i < 100; i++)
    assertNewTarget(...[undefined, null]);

for (let i = 0; i < 100; i++)
    assertNewTarget(...[undefined, null, 1]);


for (let i = 0; i < 100; i++)
    new assertNewTarget(assertNewTarget, null);

for (let i = 0; i < 100; i++)
    new assertNewTarget(assertNewTarget);

for (let i = 0; i < 100; i++)
    new assertNewTarget(assertNewTarget, null, 1);


for (let i = 0; i < 100; i++)
    new assertNewTarget(...[assertNewTarget]);

for (let i = 0; i < 100; i++)
    new assertNewTarget(...[assertNewTarget, null]);

for (let i = 0; i < 100; i++)
    new assertNewTarget(...[assertNewTarget, null, 1]);

if (typeof reportCompare === "function")
    reportCompare(0,0,"OK");
