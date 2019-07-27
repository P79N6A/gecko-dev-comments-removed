
try {
    eval('new.target');
    assertEq(false, true);
} catch (e if e instanceof SyntaxError) { }


assertThrowsInstanceOf(() => eval('new.target'), SyntaxError);


let ieval = eval;
try {
    (function () ieval('new.target'))();
    assertEq(false, true);
} catch (e if e instanceof SyntaxError) { }

function assertNewTarget(expected) {
    assertEq(eval('new.target'), expected);
    assertEq((()=>eval('new.target'))(), expected);

    
    assertEq(eval('eval("new.target")'), expected);
    assertEq(eval("eval('eval(`new.target`)')"), expected);
}

const ITERATIONS = 550;
for (let i = 0; i < ITERATIONS; i++)
    assertNewTarget(undefined);

for (let i = 0; i < ITERATIONS; i++)
    new assertNewTarget(assertNewTarget);

if (typeof reportCompare === "function")
    reportCompare(0,0,"OK");
