






























var EXPORTED_SYMBOLS = ['startsWith', 'endsWith'];

function startsWith(str, prefix, start, end) {
    if (arguments.length < 2) {
        throw new TypeError('startsWith() requires at least 2 arguments');
    }
        
    
    if ((start == null) || (isNaN(new Number(start)))) {
        start = 0;
    }
    if ((end == null) || (isNaN(new Number(end)))) {
        end = Number.MAX_VALUE;
    }
    
    
    if (typeof prefix == "object") {
        for (var i = 0, j = prefix.length; i < j; i++) {
            var res = _stringTailMatch(str, prefix[i], start, end, true);
            if (res) {
                return true;
            }
        }
        return false;
    }
    
    return _stringTailMatch(str, prefix, start, end, true);
}









function endsWith(str, suffix, start, end) {
    if (arguments.length < 2) {
        throw new TypeError('endsWith() requires at least 2 arguments');
    }
    
    
    if ((start == null) || (isNaN(new Number(start)))) {
        start = 0;
    }
    if ((end == null) || (isNaN(new Number(end)))) {
        end = Number.MAX_VALUE;
    }
    
    
    if (typeof suffix == "object") {
        for (var i = 0, j = suffix.length; i < j; i++) {
            var res = _stringTailMatch(str, suffix[i], start, end, false);
            if (res) {
                return true;
            }
        }
        return false;
    }
    
    return _stringTailMatch(str, suffix, start, end, false);
}






function _stringTailMatch(str, substr, start, end, fromStart) {
    var len = str.length;
    var slen = substr.length;
    
    var indices = _adjustIndices(start, end, len);
    start = indices[0]; end = indices[1]; len = indices[2];
    
    if (fromStart) {
        if (start + slen > len) {
            return false;
        }
    } else {
        if (end - start < slen || start > len) {
            return false;
        }
        if (end - slen > start) {
            start = end - slen;
        }
    }
    
    if (end - start >= slen) {
        return str.substr(start, slen) == substr;
    }
    return false;
}

function _adjustIndices(start, end, len)
{
	if (end > len) {
	    end = len;
	} else if (end < 0) {
	    end += len;
	}
    
    if (end < 0) {
        end = 0;
    }
	if (start < 0) {
	    start += len;   
	}
	if (start < 0) {
		start = 0;
	}
	
	return [start, end, len];
}
