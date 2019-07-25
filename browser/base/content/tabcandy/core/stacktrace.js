























































function printStackTrace(options) {
    var ex = (options && options.e) ? options.e : null;
    var guess = (options && options.guess) ? options.guess : false;
    
    var p = new printStackTrace.implementation();
    var result = p.run(ex);
    return (guess) ? p.guessFunctions(result) : result;
}

printStackTrace.implementation = function() {};

printStackTrace.implementation.prototype = {
    run: function(ex) {
        
        var mode = this._mode || this.mode();
        if (mode === 'other') {
            return this.other(arguments.callee);
        }
        else {
            ex = ex ||
                (function() {
                    try {
                        (0)();
                    } catch (e) {
                        return e;
                    }
                })();
            return this[mode](ex);
        }
    },
    
    mode: function() {
        try {
            (0)();
        } catch (e) {
            if (e.arguments) {
                return (this._mode = 'chrome');
            }
            if (e.stack) {
                return (this._mode = 'firefox');
            }
            if (window.opera && !('stacktrace' in e)) { 
                return (this._mode = 'opera');
            }
        }
        return (this._mode = 'other');
    },
    
    chrome: function(e) {
        return e.stack.replace(/^.*?\n/, '').
                replace(/^.*?\n/, '').
                replace(/^.*?\n/, '').
                replace(/^[^\(]+?[\n$]/gm, '').
                replace(/^\s+at\s+/gm, '').
                replace(/^Object.<anonymous>\s*\(/gm, '{anonymous}()@').
                split("\n");
    },
    
    firefox: function(e) {
        return e.stack.replace(/^.*?\n/, '').
                replace(/(?:\n@:0)?\s+$/m, '').
                replace(/^\(/gm, '{anonymous}(').
                split("\n");
    },
    
    
    opera: function(e) {
        var lines = e.message.split("\n"), ANON = '{anonymous}', 
            lineRE = /Line\s+(\d+).*?script\s+(http\S+)(?:.*?in\s+function\s+(\S+))?/i, i, j, len;
        
        for (i = 4, j = 0, len = lines.length; i < len; i += 2) {
            if (lineRE.test(lines[i])) {
                lines[j++] = (RegExp.$3 ? RegExp.$3 + '()@' + RegExp.$2 + RegExp.$1 : ANON + '()@' + RegExp.$2 + ':' + RegExp.$1) +
                ' -- ' +
                lines[i + 1].replace(/^\s+/, '');
            }
        }
        
        lines.splice(j, lines.length - j);
        return lines;
    },
    
    
    other: function(curr) {
        var ANON = "{anonymous}", fnRE = /function\s*([\w\-$]+)?\s*\(/i, stack = [], j = 0, fn, args;
        
        var maxStackSize = 10;
        while (curr && stack.length < maxStackSize) {
            fn = fnRE.test(curr.toString()) ? RegExp.$1 || ANON : ANON;
            args = Array.prototype.slice.call(curr['arguments']);
            stack[j++] = fn + '(' + printStackTrace.implementation.prototype.stringifyArguments(args) + ')';
            
            
            if (curr === curr.caller && window.opera) {
	            
                break;
            }
            curr = curr.caller;
        }
        return stack;
    },
    
    stringifyArguments: function(args) {
        for (var i = 0; i < args.length; ++i) {
            var argument = args[i];
            if (typeof argument == 'object') {
                args[i] = '#object';
            } else if (typeof argument == 'function') {
                args[i] = '#function';
            } else if (typeof argument == 'string') {
                args[i] = '"' + argument + '"';
            }
        }
        return args.join(',');
    },
    
    sourceCache: {},
    
    ajax: function(url) {
        var req = this.createXMLHTTPObject();
        if (!req) {
            return;
        }
        req.open('GET', url, false);
        req.setRequestHeader("User-Agent", "XMLHTTP/1.0");
        req.send('');
        return req.responseText;
    },
    
    createXMLHTTPObject: function() {
	    
        var xmlhttp, XMLHttpFactories = [
            function() {
                return new XMLHttpRequest();
            }, function() {
                return new ActiveXObject("Msxml2.XMLHTTP");
            }, function() {
                return new ActiveXObject("Msxml3.XMLHTTP");
            }, function() {
                return new ActiveXObject("Microsoft.XMLHTTP");
            }
        ];
        for (var i = 0; i < XMLHttpFactories.length; i++) {
            try {
                xmlhttp = XMLHttpFactories[i]();
                
                this.createXMLHTTPObject = XMLHttpFactories[i];
                return xmlhttp;
            } catch (e) {}
        }
    },
    
    getSource: function(url) {
        if (!(url in this.sourceCache)) {
            this.sourceCache[url] = this.ajax(url).split("\n");
        }
        return this.sourceCache[url];
    },
    
    guessFunctions: function(stack) {
        for (var i = 0; i < stack.length; ++i) {
            var reStack = /{anonymous}\(.*\)@(\w+:\/\/([-\w\.]+)+(:\d+)?[^:]+):(\d+):?(\d+)?/;
            var frame = stack[i], m = reStack.exec(frame);
            if (m) {
                var file = m[1], lineno = m[4]; 
                if (file && lineno) {
                    var functionName = this.guessFunctionName(file, lineno);
                    stack[i] = frame.replace('{anonymous}', functionName);
                }
            }
        }
        return stack;
    },
    
    guessFunctionName: function(url, lineNo) {
        try {
            return this.guessFunctionNameFromLines(lineNo, this.getSource(url));
        } catch (e) {
            return 'getSource failed with url: ' + url + ', exception: ' + e.toString();
        }
    },
    
    guessFunctionNameFromLines: function(lineNo, source) {
        var reFunctionArgNames = /function ([^(]*)\(([^)]*)\)/;
        var reGuessFunction = /['"]?([0-9A-Za-z_]+)['"]?\s*[:=]\s*(function|eval|new Function)/;
        
        
        var line = "", maxLines = 10;
        for (var i = 0; i < maxLines; ++i) {
            line = source[lineNo - i] + line;
            if (line !== undefined) {
                var m = reGuessFunction.exec(line);
                if (m) {
                    return m[1];
                }
                else {
                    m = reFunctionArgNames.exec(line);
                }
                if (m && m[1]) {
                    return m[1];
                }
            }
        }
        return "(?)";
    }
};
