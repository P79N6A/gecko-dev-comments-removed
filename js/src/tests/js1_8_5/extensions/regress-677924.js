


try {
    version(4096);  
} catch (exc) {
}

try {
    version(-1);  
} catch (exc) {
}

reportCompare(0, 0, 'ok');
