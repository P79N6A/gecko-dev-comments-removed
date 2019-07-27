


function logProxy(object = {}, handler = {}) {
    var log = [];
    var proxy = new Proxy(object, new Proxy(handler, {
        get(target, propertyKey, receiver) {
            log.push(propertyKey);
            return target[propertyKey];
        }
    }));
    return {proxy, log};
}





var {proxy, log} = logProxy();
Object.seal(proxy);
assertDeepEq(log, ["preventExtensions", "ownKeys"]);

var {proxy, log} = logProxy();
Object.seal(Object.seal(proxy));
assertDeepEq(log, ["preventExtensions", "ownKeys", "preventExtensions", "ownKeys"]);

reportCompare(0, 0);
