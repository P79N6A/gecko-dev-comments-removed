



assertDeepEq(
    Reflect.getOwnPropertyDescriptor({x: "hello"}, "x"),
    {value: "hello", writable: true, enumerable: true, configurable: true});
assertEq(
    Reflect.getOwnPropertyDescriptor({x: "hello"}, "y"),
    undefined);
assertDeepEq(
    Reflect.getOwnPropertyDescriptor([], "length"),
    {value: 0, writable: true, enumerable: false, configurable: false});







reportCompare(0, 0);
