









if (typeof(dojo) != 'undefined') {
    dojo.provide("MochiKit.Async");
    dojo.require("MochiKit.Base");
}
if (typeof(JSAN) != 'undefined') {
    JSAN.use("MochiKit.Base", []);
}

try {
    if (typeof(MochiKit.Base) == 'undefined') {
        throw "";
    }
} catch (e) {
    throw "MochiKit.Async depends on MochiKit.Base!";
}

if (typeof(MochiKit.Async) == 'undefined') {
    MochiKit.Async = {};
}

MochiKit.Async.NAME = "MochiKit.Async";
MochiKit.Async.VERSION = "1.4";
MochiKit.Async.__repr__ = function () {
    return "[" + this.NAME + " " + this.VERSION + "]";
};
MochiKit.Async.toString = function () {
    return this.__repr__();
};


MochiKit.Async.Deferred = function ( canceller) {
    this.chain = [];
    this.id = this._nextId();
    this.fired = -1;
    this.paused = 0;
    this.results = [null, null];
    this.canceller = canceller;
    this.silentlyCancelled = false;
    this.chained = false;
};

MochiKit.Async.Deferred.prototype = {
    
    repr: function () {
        var state;
        if (this.fired == -1) {
            state = 'unfired';
        } else if (this.fired === 0) {
            state = 'success';
        } else {
            state = 'error';
        }
        return 'Deferred(' + this.id + ', ' + state + ')';
    },

    toString: MochiKit.Base.forwardCall("repr"),

    _nextId: MochiKit.Base.counter(),

    
    cancel: function () {
        var self = MochiKit.Async;
        if (this.fired == -1) {
            if (this.canceller) {
                this.canceller(this);
            } else {
                this.silentlyCancelled = true;
            }
            if (this.fired == -1) {
                this.errback(new self.CancelledError(this));
            }
        } else if ((this.fired === 0) && (this.results[0] instanceof self.Deferred)) {
            this.results[0].cancel();
        }
    },
            
    _resback: function (res) {
        




        this.fired = ((res instanceof Error) ? 1 : 0);
        this.results[this.fired] = res;
        this._fire();
    },

    _check: function () {
        if (this.fired != -1) {
            if (!this.silentlyCancelled) {
                throw new MochiKit.Async.AlreadyCalledError(this);
            }
            this.silentlyCancelled = false;
            return;
        }
    },

    
    callback: function (res) {
        this._check();
        if (res instanceof MochiKit.Async.Deferred) {
            throw new Error("Deferred instances can only be chained if they are the result of a callback");
        }
        this._resback(res);
    },

    
    errback: function (res) {
        this._check();
        var self = MochiKit.Async;
        if (res instanceof self.Deferred) {
            throw new Error("Deferred instances can only be chained if they are the result of a callback");
        }
        if (!(res instanceof Error)) {
            res = new self.GenericError(res);
        }
        this._resback(res);
    },

    
    addBoth: function (fn) {
        if (arguments.length > 1) {
            fn = MochiKit.Base.partial.apply(null, arguments);
        }
        return this.addCallbacks(fn, fn);
    },

    
    addCallback: function (fn) {
        if (arguments.length > 1) {
            fn = MochiKit.Base.partial.apply(null, arguments);
        }
        return this.addCallbacks(fn, null);
    },

    
    addErrback: function (fn) {
        if (arguments.length > 1) {
            fn = MochiKit.Base.partial.apply(null, arguments);
        }
        return this.addCallbacks(null, fn);
    },

    
    addCallbacks: function (cb, eb) {
        if (this.chained) {
            throw new Error("Chained Deferreds can not be re-used");
        }
        this.chain.push([cb, eb]);
        if (this.fired >= 0) {
            this._fire();
        }
        return this;
    },

    _fire: function () {
        





        var chain = this.chain;
        var fired = this.fired;
        var res = this.results[fired];
        var self = this;
        var cb = null;
        while (chain.length > 0 && this.paused === 0) {
            
            var pair = chain.shift();
            var f = pair[fired];
            if (f === null) {
                continue;
            }
            try {
                res = f(res);
                fired = ((res instanceof Error) ? 1 : 0);
                if (res instanceof MochiKit.Async.Deferred) {
                    cb = function (res) {
                        self._resback(res);
                        self.paused--;
                        if ((self.paused === 0) && (self.fired >= 0)) {
                            self._fire();
                        }
                    };
                    this.paused++;
                }
            } catch (err) {
                fired = 1;
                if (!(err instanceof Error)) {
                    err = new MochiKit.Async.GenericError(err);
                }
                res = err;
            }
        }
        this.fired = fired;
        this.results[fired] = res;
        if (cb && this.paused) {
            
            
            res.addBoth(cb);
            res.chained = true;
        }
    }
};

MochiKit.Base.update(MochiKit.Async, {
    
    evalJSONRequest: function () {
        return eval('(' + arguments[0].responseText + ')');
    },

    
    succeed: function (result) {
        var d = new MochiKit.Async.Deferred();
        d.callback.apply(d, arguments);
        return d;
    },

    
    fail: function (result) {
        var d = new MochiKit.Async.Deferred();
        d.errback.apply(d, arguments);
        return d;
    },

    
    getXMLHttpRequest: function () {
        var self = arguments.callee;
        if (!self.XMLHttpRequest) {
            var tryThese = [
                function () { return new XMLHttpRequest(); },
                function () { return new ActiveXObject('Msxml2.XMLHTTP'); },
                function () { return new ActiveXObject('Microsoft.XMLHTTP'); },
                function () { return new ActiveXObject('Msxml2.XMLHTTP.4.0'); },
                function () {
                    throw new MochiKit.Async.BrowserComplianceError("Browser does not support XMLHttpRequest");
                }
            ];
            for (var i = 0; i < tryThese.length; i++) {
                var func = tryThese[i];
                try {
                    self.XMLHttpRequest = func;
                    return func();
                } catch (e) {
                    
                }
            }
        }
        return self.XMLHttpRequest();
    },

    _xhr_onreadystatechange: function (d) {
        
        var m = MochiKit.Base;
        if (this.readyState == 4) {
            
            try {
                this.onreadystatechange = null;
            } catch (e) {
                try {
                    this.onreadystatechange = m.noop;
                } catch (e) {
                }
            }
            var status = null;
            try {
                status = this.status;
                if (!status && m.isNotEmpty(this.responseText)) {
                    
                    status = 304;
                }
            } catch (e) {
                
                
            }
            
            if (status == 200 || status == 304) { 
                d.callback(this);
            } else {
                var err = new MochiKit.Async.XMLHttpRequestError(this, "Request failed");
                if (err.number) {
                    
                    d.errback(err);
                } else {
                    
                    d.errback(err);
                }
            }
        }
    },

    _xhr_canceller: function (req) {
        
        try {
            req.onreadystatechange = null;
        } catch (e) {
            try {
                req.onreadystatechange = MochiKit.Base.noop;
            } catch (e) {
            }
        }
        req.abort();
    },

    
    
    sendXMLHttpRequest: function (req,  sendContent) {
        if (typeof(sendContent) == "undefined" || sendContent === null) {
            sendContent = "";
        }

        var m = MochiKit.Base;
        var self = MochiKit.Async;
        var d = new self.Deferred(m.partial(self._xhr_canceller, req));
        
        try {
            req.onreadystatechange = m.bind(self._xhr_onreadystatechange,
                req, d);
            req.send(sendContent);
        } catch (e) {
            try {
                req.onreadystatechange = null;
            } catch (ignore) {
                
            }
            d.errback(e);
        }

        return d;

    },

    
    doXHR: function (url, opts) {
        var m = MochiKit.Base;
        opts = m.update({
            method: 'GET',
            sendContent: ''
            






        }, opts);
        var self = MochiKit.Async;
        var req = self.getXMLHttpRequest();
        if (opts.queryString) {
            var qs = m.queryString(opts.queryString);
            if (qs) {
                url += "?" + qs;
            }
        }
        req.open(opts.method, url, true, opts.username, opts.password);
        if (req.overrideMimeType && opts.mimeType) {
            req.overrideMimeType(opts.mimeType);
        }
        if (opts.headers) {
            var headers = opts.headers;
            if (!m.isArrayLike(headers)) {
                headers = m.items(headers);
            }
            for (var i = 0; i < headers.length; i++) {
                var header = headers[i];
                var name = header[0];
                var value = header[1];
                req.setRequestHeader(name, value);
            }
        }
        return self.sendXMLHttpRequest(req, opts.sendContent);
    },
            
    _buildURL: function (url) {
        if (arguments.length > 1) {
            var m = MochiKit.Base;
            var qs = m.queryString.apply(null, m.extend(null, arguments, 1));
            if (qs) {
                return url + "?" + qs;
            }
        }
        return url;
    },
    
    
    doSimpleXMLHttpRequest: function (url) {
        var self = MochiKit.Async;
        url = self._buildURL.apply(self, arguments);
        return self.doXHR(url);
    },

    
    loadJSONDoc: function (url) {
        var self = MochiKit.Async;
        url = self._buildURL.apply(self, arguments);
        var d = self.doXHR(url, {
            'mimeType': 'text/plain',
            'headers': [['Accept', 'application/json']]
        });
        d = d.addCallback(self.evalJSONRequest);
        return d;
    },

    
    wait: function (seconds, value) {
        var d = new MochiKit.Async.Deferred();
        var m = MochiKit.Base;
        if (typeof(value) != 'undefined') {
            d.addCallback(function () { return value; });
        }
        var timeout = setTimeout(
            m.bind("callback", d),
            Math.floor(seconds * 1000));
        d.canceller = function () {
            try {
                clearTimeout(timeout);
            } catch (e) {
                
            }
        };
        return d;
    },

    
    callLater: function (seconds, func) {
        var m = MochiKit.Base;
        var pfunc = m.partial.apply(m, m.extend(null, arguments, 1));
        return MochiKit.Async.wait(seconds).addCallback(
            function (res) { return pfunc(); }
        );
    }
});



MochiKit.Async.DeferredLock = function () {
    this.waiting = [];
    this.locked = false;
    this.id = this._nextId();
};

MochiKit.Async.DeferredLock.prototype = {
    __class__: MochiKit.Async.DeferredLock,
    
    acquire: function () {
        var d = new MochiKit.Async.Deferred();
        if (this.locked) {
            this.waiting.push(d);
        } else {
            this.locked = true;
            d.callback(this);
        }
        return d;
    },
    
    release: function () {
        if (!this.locked) {
            throw TypeError("Tried to release an unlocked DeferredLock");
        }
        this.locked = false;
        if (this.waiting.length > 0) {
            this.locked = true;
            this.waiting.shift().callback(this);
        }
    },
    _nextId: MochiKit.Base.counter(),
    repr: function () {
        var state;
        if (this.locked) {
            state = 'locked, ' + this.waiting.length + ' waiting';
        } else {
            state = 'unlocked';
        }
        return 'DeferredLock(' + this.id + ', ' + state + ')';
    },
    toString: MochiKit.Base.forwardCall("repr")

};


MochiKit.Async.DeferredList = function (list, fireOnOneCallback, fireOnOneErrback, consumeErrors, canceller) {

    
    MochiKit.Async.Deferred.apply(this, [canceller]);
    
    this.list = list;
    var resultList = [];
    this.resultList = resultList;

    this.finishedCount = 0;
    this.fireOnOneCallback = fireOnOneCallback;
    this.fireOnOneErrback = fireOnOneErrback;
    this.consumeErrors = consumeErrors;

    var cb = MochiKit.Base.bind(this._cbDeferred, this);
    for (var i = 0; i < list.length; i++) {
        var d = list[i];
        resultList.push(undefined);
        d.addCallback(cb, i, true);
        d.addErrback(cb, i, false);
    }

    if (list.length === 0 && !fireOnOneCallback) {
        this.callback(this.resultList);
    }
    
};

MochiKit.Async.DeferredList.prototype = new MochiKit.Async.Deferred();

MochiKit.Async.DeferredList.prototype._cbDeferred = function (index, succeeded, result) {
    this.resultList[index] = [succeeded, result];
    this.finishedCount += 1;
    if (this.fired == -1) {
        if (succeeded && this.fireOnOneCallback) {
            this.callback([index, result]);
        } else if (!succeeded && this.fireOnOneErrback) {
            this.errback(result);
        } else if (this.finishedCount == this.list.length) {
            this.callback(this.resultList);
        }
    }
    if (!succeeded && this.consumeErrors) {
        result = null;
    }
    return result;
};


MochiKit.Async.gatherResults = function (deferredList) {
    var d = new MochiKit.Async.DeferredList(deferredList, false, true, false);
    d.addCallback(function (results) {
        var ret = [];
        for (var i = 0; i < results.length; i++) {
            ret.push(results[i][1]);
        }
        return ret;
    });
    return d;
};


MochiKit.Async.maybeDeferred = function (func) {
    var self = MochiKit.Async;
    var result;
    try {
        var r = func.apply(null, MochiKit.Base.extend([], arguments, 1));
        if (r instanceof self.Deferred) {
            result = r;
        } else if (r instanceof Error) {
            result = self.fail(r);
        } else {
            result = self.succeed(r);
        }
    } catch (e) {
        result = self.fail(e);
    }
    return result;
};


MochiKit.Async.EXPORT = [
    "AlreadyCalledError",
    "CancelledError",
    "BrowserComplianceError",
    "GenericError",
    "XMLHttpRequestError",
    "Deferred",
    "succeed",
    "fail",
    "getXMLHttpRequest",
    "doSimpleXMLHttpRequest",
    "loadJSONDoc",
    "wait",
    "callLater",
    "sendXMLHttpRequest",
    "DeferredLock",
    "DeferredList",
    "gatherResults",
    "maybeDeferred",
    "doXHR"
];
    
MochiKit.Async.EXPORT_OK = [
    "evalJSONRequest"
];

MochiKit.Async.__new__ = function () {
    var m = MochiKit.Base;
    var ne = m.partial(m._newNamedError, this);
    
    ne("AlreadyCalledError", 
        
        function (deferred) {
            





            this.deferred = deferred;
        }
    );

    ne("CancelledError",
        
        function (deferred) {
            




            this.deferred = deferred;
        }
    );

    ne("BrowserComplianceError",
        
        function (msg) {
            







            this.message = msg;
        }
    );

    ne("GenericError", 
        
        function (msg) {
            this.message = msg;
        }
    );

    ne("XMLHttpRequestError",
        
        function (req, msg) {
            




            this.req = req;
            this.message = msg;
            try {
                
                this.number = req.status;
            } catch (e) {
                
            }
        }
    );


    this.EXPORT_TAGS = {
        ":common": this.EXPORT,
        ":all": m.concat(this.EXPORT, this.EXPORT_OK)
    };

    m.nameFunctions(this);

};

MochiKit.Async.__new__();

MochiKit.Base._exportSymbols(this, MochiKit.Async);
