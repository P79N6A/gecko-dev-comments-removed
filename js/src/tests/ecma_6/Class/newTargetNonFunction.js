


assertThrowsInstanceOf(() => new ""(...Array()), TypeError);

assertThrowsInstanceOf(() => new ""(), TypeError);
assertThrowsInstanceOf(() => new ""(1), TypeError);

if (typeof reportCompare === 'function')
    reportCompare(0,0,"OK");
