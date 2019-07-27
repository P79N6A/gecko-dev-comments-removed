
new Function('(() => new.target)()');


assertThrowsInstanceOf(() => eval('() => new.target'), SyntaxError);

function assertNewTarget(expected) {
    assertEq((()=>new.target)(), expected);
    assertEq(eval('()=>new.target')(), expected);

    
    
    return (() => new.target);
}

const ITERATIONS = 550;
for (let i = 0; i < ITERATIONS; i++)
    assertEq(assertNewTarget(undefined)(), undefined);

for (let i = 0; i < ITERATIONS; i++)
    assertEq(new assertNewTarget(assertNewTarget)(), assertNewTarget);

if (typeof reportCompare === 'function')
    reportCompare(0,0,"OK");
