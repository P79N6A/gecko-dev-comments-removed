




function c(a) {
    this.f = function () { a; };
}
c(0);  
Object.defineProperty(this, "f", {configurable: true, enumerable: true, value: 3});

reportCompare(0, 0, "");
