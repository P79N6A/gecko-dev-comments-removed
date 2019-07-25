


































var LogController = {}; 

LogController.counter = 0; 
LogController.listeners = [];
LogController.logLevel = {
    FATAL: 50,
    ERROR: 40,
    WARNING: 30,
    INFO: 20,
    DEBUG: 10
};


LogController.logLevelAtLeast = function(minLevel) {
    if (typeof(minLevel) == 'string') {
        minLevel = LogController.logLevel[minLevel];
    }
    return function (msg) {
        var msgLevel = msg.level;
        if (typeof(msgLevel) == 'string') {
            msgLevel = LogController.logLevel[msgLevel];
        }
        return msgLevel >= minLevel;
    };
};


LogController.createLogMessage = function(level, info) {
    var msg = {};
    msg.num = LogController.counter;
    msg.level = level;
    msg.info = info;
    msg.timestamp = new Date();
    return msg;
};


LogController.extend = function (args, skip) {
    var ret = [];
    for (var i = skip; i<args.length; i++) {
        ret.push(args[i]);
    }
    return ret;
};


LogController.logWithLevel = function(level, message) {
    var msg = LogController.createLogMessage(
        level,
        LogController.extend(arguments, 1)
    );
    LogController.dispatchListeners(msg);
    LogController.counter += 1;
};


LogController.log = function(message) {
    LogController.logWithLevel('INFO', message);
};


LogController.error = function(message) {
    LogController.logWithLevel('ERROR', message);
};


LogController.dispatchListeners = function(msg) {
    for (var k in LogController.listeners) {
        var pair = LogController.listeners[k];
        if (pair.ident != k || (pair[0] && !pair[0](msg))) {
            continue;
        }
        pair[1](msg);
    }
};


LogController.addListener = function(ident, filter, listener) {
    if (typeof(filter) == 'string') {
        filter = LogController.logLevelAtLeast(filter);
    } else if (filter !== null && typeof(filter) !== 'function') {
        throw new Error("Filter must be a string, a function, or null");
    }
    var entry = [filter, listener];
    entry.ident = ident;
    LogController.listeners[ident] = entry;
};


LogController.removeListener = function(ident) {
    delete LogController.listeners[ident];
};
