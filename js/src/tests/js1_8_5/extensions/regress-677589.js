



try {
    clone(null);  
} catch (exc if exc instanceof TypeError) {
}

reportCompare(0, 0, 'ok');
