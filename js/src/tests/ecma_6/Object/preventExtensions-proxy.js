


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
Object.preventExtensions(proxy);
assertDeepEq(log, ["preventExtensions"]);

var {proxy, log} = logProxy();
Object.preventExtensions(Object.preventExtensions(proxy));
assertDeepEq(log, ["preventExtensions", "preventExtensions"]);

reportCompare(0, 0);
