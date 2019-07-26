






Object.preventExtensions(this);
delete Function;

try {
    
    Object.getOwnPropertyNames(this);
} catch(e) {
    reportCompare(true, false, "this shouldn't have thrown");
}
reportCompare(0, 0, "ok");

