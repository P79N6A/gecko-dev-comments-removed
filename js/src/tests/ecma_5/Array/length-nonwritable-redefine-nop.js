







var BUGNUMBER = 858381;
var summary = "No-op array length redefinition";

print(BUGNUMBER + ": " + summary);





var arr;



arr = Object.defineProperty([0, 1, 2, 3, 4, 5], "length", { writable: false });
Object.defineProperty(arr, "length", { value: 6 });
Object.defineProperty(arr, "length", { writable: false });
Object.defineProperty(arr, "length", { configurable: false });
Object.defineProperty(arr, "length", { writable: false, configurable: false });
Object.defineProperty(arr, "length", { writable: false, value: 6 });
Object.defineProperty(arr, "length", { configurable: false, value: 6 });
Object.defineProperty(arr, "length", { writable: false, configurable: false, value: 6 });



arr = Object.defineProperty([0, 1, 2, 3, 4, 5], "length", { value: 8, writable: false });
Object.defineProperty(arr, "length", { value: 8 });
Object.defineProperty(arr, "length", { writable: false });
Object.defineProperty(arr, "length", { configurable: false });
Object.defineProperty(arr, "length", { writable: false, configurable: false });
Object.defineProperty(arr, "length", { writable: false, value: 8 });
Object.defineProperty(arr, "length", { configurable: false, value: 8 });
Object.defineProperty(arr, "length", { writable: false, configurable: false, value: 8 });



arr = Object.defineProperty([0, 1, 2, 3, 4, 5, 6, , ], "length",
                            { value: 8, writable: false });
Object.defineProperty(arr, "length", { value: 8 });
Object.defineProperty(arr, "length", { writable: false });
Object.defineProperty(arr, "length", { configurable: false });
Object.defineProperty(arr, "length", { writable: false, configurable: false });
Object.defineProperty(arr, "length", { writable: false, value: 8 });
Object.defineProperty(arr, "length", { configurable: false, value: 8 });
Object.defineProperty(arr, "length", { writable: false, configurable: false, value: 8 });



arr = Object.defineProperty([0, 1, 2], "length", { value: 8, writable: false });
Object.defineProperty(arr, "length", { value: 8 });
Object.defineProperty(arr, "length", { writable: false });
Object.defineProperty(arr, "length", { configurable: false });
Object.defineProperty(arr, "length", { writable: false, configurable: false });
Object.defineProperty(arr, "length", { writable: false, value: 8 });
Object.defineProperty(arr, "length", { configurable: false, value: 8 });
Object.defineProperty(arr, "length", { writable: false, configurable: false, value: 8 });



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
