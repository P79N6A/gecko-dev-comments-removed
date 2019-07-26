
const ONE_PLUS_EPSILON = 1 + Math.pow(2, -52);  
const ONE_MINUS_EPSILON = 1 - Math.pow(2, -53);  

function assertNear(actual, expected) {
    var error = Math.abs(actual - expected);

    if (error > 1e-300 && error > Math.abs(actual) * 1e-12)
        throw 'Assertion failed: got "' + actual + '", expected "' + expected + '" (rel error = ' + (error / Math.abs(actual)) + ')';
}

