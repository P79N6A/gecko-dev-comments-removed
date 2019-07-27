



var test = `

// |undefined|, writable: false
Object.defineProperty(Object.prototype, "prop", { writable: false });

class strictAssignmentTest {
    constructor() { 
        // Strict mode. Throws.
        super.prop = 14;
    }
}

assertThrowsInstanceOf(()=>new strictAssignmentTest(), TypeError);

// Non-strict. Silent failure.
({ test() { super.prop = 14; } }).test();

assertEq(Object.prototype.prop, undefined);

`;

if (classesEnabled())
    eval(test);

if (typeof reportCompare === 'function')
    reportCompare(0,0,"OK");
