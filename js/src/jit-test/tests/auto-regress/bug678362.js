




var replacer = [0, 1, 2, 3];
Object.defineProperty(replacer, 3.e7, {});
JSON.stringify({ 0: 0, 1: 1, 2: 2, 3: 3 }, replacer)
