



try {
new Int32Array(new Proxy(new Uint8ClampedArray, {}))
throw new Error("Hey! you made .length work on proxies! Congrats!" +
                "Remove the try catch for karma points. :)");
} catch (e) { }
