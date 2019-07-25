



try {
    new {prototype: TypeError.prototype};
} catch (e) {}

reportCompare(0, 0, 'ok');
