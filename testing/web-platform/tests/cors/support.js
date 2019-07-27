












function dirname(path) {
    return path.replace(/\/[^\/]*$/, '/')
}


var SUBDOMAIN = 'www1'
var SUBDOMAIN2 = 'www2'
var PORT = {{ports[http][1]}}




var CROSSDOMAIN     = dirname(location.href)
                        .replace('://', '://' + SUBDOMAIN + '.')
var REMOTE_HOST     = SUBDOMAIN + '.' + location.host
var REMOTE_PROTOCOL = location.protocol
var REMOTE_ORIGIN   = REMOTE_PROTOCOL + '//' + REMOTE_HOST
