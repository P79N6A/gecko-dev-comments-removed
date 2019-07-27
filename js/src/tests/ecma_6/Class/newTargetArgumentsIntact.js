


var argsContent;

function argsWithNewTarget(foo) {
    assertEq(arguments.length, argsContent.length);
    for (let i = 0; i < arguments.length; i++)
        assertEq(arguments[i], argsContent[i]);
    let nt = new.target;

    
    arguments[arguments.length] = 42;
    assertEq(new.target, nt);
}


argsContent = [];
for (let i = 0; i < 100; i++)
    new argsWithNewTarget();

argsContent = [1];
for (let i = 0; i < 100; i++)
    new argsWithNewTarget(1);

argsContent = [1,2,3];
for (let i = 0; i < 100; i++)
    new argsWithNewTarget(1, 2, 3);


argsContent = [];
for (let i = 0; i < 100; i++)
    new argsWithNewTarget(...[]);

argsContent = [1];
for (let i = 0; i < 100; i++)
    new argsWithNewTarget(...[1]);

argsContent = [1,2,3];
for (let i = 0; i < 100; i++)
    new argsWithNewTarget(...[1,2,3]);

if (typeof reportCompare === "function")
    reportCompare(0,0,"OK");
