















































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
const CC = Components.Constructor;


var DEBUG = false; 

var gGlobalObject = this;







function NS_ASSERT(cond, msg)
{
  if (DEBUG && !cond)
  {
    dumpn("###!!!");
    dumpn("###!!! ASSERTION" + (msg ? ": " + msg : "!"));
    dumpn("###!!! Stack follows:");

    var stack = new Error().stack.split(/\n/);
    dumpn(stack.map(function(val) { return "###!!!   " + val; }).join("\n"));
    
    throw Cr.NS_ERROR_ABORT;
  }
}


function HttpError(code, description)
{
  this.code = code;
  this.description = description;
}
HttpError.prototype =
{
  toString: function()
  {
    return this.code + " " + this.description;
  }
};




const HTTP_400 = new HttpError(400, "Bad Request");
const HTTP_401 = new HttpError(401, "Unauthorized");
const HTTP_402 = new HttpError(402, "Payment Required");
const HTTP_403 = new HttpError(403, "Forbidden");
const HTTP_404 = new HttpError(404, "Not Found");
const HTTP_405 = new HttpError(405, "Method Not Allowed");
const HTTP_406 = new HttpError(406, "Not Acceptable");
const HTTP_407 = new HttpError(407, "Proxy Authentication Required");
const HTTP_408 = new HttpError(408, "Request Timeout");
const HTTP_409 = new HttpError(409, "Conflict");
const HTTP_410 = new HttpError(410, "Gone");
const HTTP_411 = new HttpError(411, "Length Required");
const HTTP_412 = new HttpError(412, "Precondition Failed");
const HTTP_413 = new HttpError(413, "Request Entity Too Large");
const HTTP_414 = new HttpError(414, "Request-URI Too Long");
const HTTP_415 = new HttpError(415, "Unsupported Media Type");
const HTTP_416 = new HttpError(416, "Requested Range Not Satisfiable");
const HTTP_417 = new HttpError(417, "Expectation Failed");

const HTTP_500 = new HttpError(500, "Internal Server Error");
const HTTP_501 = new HttpError(501, "Not Implemented");
const HTTP_502 = new HttpError(502, "Bad Gateway");
const HTTP_503 = new HttpError(503, "Service Unavailable");
const HTTP_504 = new HttpError(504, "Gateway Timeout");
const HTTP_505 = new HttpError(505, "HTTP Version Not Supported");


function array2obj(arr)
{
  var obj = {};
  for (var i = 0; i < arr.length; i++)
    obj[arr[i]] = arr[i];
  return obj;
}


function range(x, y)
{
  var arr = [];
  for (var i = x; i <= y; i++)
    arr.push(i);
  return arr;
}


const HTTP_ERROR_CODES = array2obj(range(400, 417).concat(range(500, 505)));











const HIDDEN_CHAR = "^";





const HEADERS_SUFFIX = HIDDEN_CHAR + "headers" + HIDDEN_CHAR;


const SJS_TYPE = "sjs";



function dumpn(str)
{
  if (DEBUG)
    dump(str + "\n");
}


function dumpStack()
{
  
  var stack = new Error().stack.split(/\n/).slice(2);
  stack.forEach(dumpn);
}



var gThreadManager = null;







const ServerSocket = CC("@mozilla.org/network/server-socket;1",
                        "nsIServerSocket",
                        "init");
const BinaryInputStream = CC("@mozilla.org/binaryinputstream;1",
                             "nsIBinaryInputStream",
                             "setInputStream");
const ScriptableInputStream = CC("@mozilla.org/scriptableinputstream;1",
                                 "nsIScriptableInputStream",
                                 "init");
const Pipe = CC("@mozilla.org/pipe;1",
                "nsIPipe",
                "init");
const FileInputStream = CC("@mozilla.org/network/file-input-stream;1",
                           "nsIFileInputStream",
                           "init");
const StreamCopier = CC("@mozilla.org/network/async-stream-copier;1",
                        "nsIAsyncStreamCopier",
                        "init");
const ConverterInputStream = CC("@mozilla.org/intl/converter-input-stream;1",
                                "nsIConverterInputStream",
                                "init");
const WritablePropertyBag = CC("@mozilla.org/hash-property-bag;1",
                               "nsIWritablePropertyBag2");
const SupportsString = CC("@mozilla.org/supports-string;1",
                          "nsISupportsString");










function toDateString(date)
{
  
  
  
  
  
  
  
  
  
  
  
  

  const wkdayStrings = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
  const monthStrings = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];

  








  function toTime(date)
  {
    var hrs = date.getUTCHours();
    var rv  = (hrs < 10) ? "0" + hrs : hrs;
    
    var mins = date.getUTCMinutes();
    rv += ":";
    rv += (mins < 10) ? "0" + mins : mins;

    var secs = date.getUTCSeconds();
    rv += ":";
    rv += (secs < 10) ? "0" + secs : secs;

    return rv;
  }

  








  function toDate1(date)
  {
    var day = date.getUTCDate();
    var month = date.getUTCMonth();
    var year = date.getUTCFullYear();

    var rv = (day < 10) ? "0" + day : day;
    rv += " " + monthStrings[month];
    rv += " " + year;

    return rv;
  }

  date = new Date(date);

  const fmtString = "%wkday%, %date1% %time% GMT";
  var rv = fmtString.replace("%wkday%", wkdayStrings[date.getUTCDay()]);
  rv = rv.replace("%time%", toTime(date));
  return rv.replace("%date1%", toDate1(date));
}






function printObj(o, showMembers)
{
  var s = "******************************\n";
  s +=    "o = {\n";
  for (var i in o)
  {
    if (typeof(i) != "string" ||
        (showMembers || (i.length > 0 && i[0] != "_")))
      s+= "      " + i + ": " + o[i] + ",\n";
  }
  s +=    "    };\n";
  s +=    "******************************";
  dumpn(s);
}




function nsHttpServer()
{
  if (!gThreadManager)
    gThreadManager = Cc["@mozilla.org/thread-manager;1"].getService();

  
  this._port = undefined;

  
  this._socket = null;

  
  this._handler = new ServerHandler(this);

  
  this._identity = new ServerIdentity();

  


  this._doQuit = false;

  



  this._socketClosed = true;
}
nsHttpServer.prototype =
{
  

  









  onSocketAccepted: function(socket, trans)
  {
    dumpn("*** onSocketAccepted(socket=" + socket + ", trans=" + trans + ") " +
          "on thread " + gThreadManager.currentThread +
          " (main is " + gThreadManager.mainThread + ")");

    dumpn(">>> new connection on " + trans.host + ":" + trans.port);

    const SEGMENT_SIZE = 8192;
    const SEGMENT_COUNT = 1024;
    var input = trans.openInputStream(0, SEGMENT_SIZE, SEGMENT_COUNT)
                     .QueryInterface(Ci.nsIAsyncInputStream);
    var output = trans.openOutputStream(Ci.nsITransport.OPEN_BLOCKING, 0, 0);

    var conn = new Connection(input, output, this, socket.port);
    var reader = new RequestReader(conn);

    

    
    
    
    input.asyncWait(reader, 0, 0, gThreadManager.mainThread);
  },

  









  onStopListening: function(socket, status)
  {
    dumpn(">>> shutting down server");
    this._socketClosed = true;
  },

  

  
  
  
  start: function(port)
  {
    if (this._socket)
      throw Cr.NS_ERROR_ALREADY_INITIALIZED;

    this._port = port;
    this._doQuit = this._socketClosed = false;

    var socket = new ServerSocket(this._port,
                                  true, 
                                  -1);  

    dumpn(">>> listening on port " + socket.port);
    socket.asyncListen(this);
    this._identity._initialize(port, true);
    this._socket = socket;
  },

  
  
  
  stop: function()
  {
    if (!this._socket)
      return;

    dumpn(">>> stopping listening on port " + this._socket.port);
    this._socket.close();
    this._socket = null;

    
    
    this._identity._teardown();

    this._doQuit = false;

    
    var thr = gThreadManager.currentThread;
    while (!this._socketClosed || this._handler.hasPendingRequests())
      thr.processNextEvent(true);
  },

  
  
  
  registerFile: function(path, file)
  {
    if (file && (!file.exists() || file.isDirectory()))
      throw Cr.NS_ERROR_INVALID_ARG;

    this._handler.registerFile(path, file);
  },

  
  
  
  registerDirectory: function(path, directory)
  {
    
    if (path.charAt(0) != "/" ||
        path.charAt(path.length - 1) != "/" ||
        (directory &&
         (!directory.exists() || !directory.isDirectory())))
      throw Cr.NS_ERROR_INVALID_ARG;

    
    

    this._handler.registerDirectory(path, directory);
  },

  
  
  
  registerPathHandler: function(path, handler)
  {
    this._handler.registerPathHandler(path, handler);
  },

  
  
  
  registerErrorHandler: function(code, handler)
  {
    this._handler.registerErrorHandler(code, handler);
  },

  
  
  
  setIndexHandler: function(handler)
  {
    this._handler.setIndexHandler(handler);
  },

  
  
  
  registerContentType: function(ext, type)
  {
    this._handler.registerContentType(ext, type);
  },

  
  
  
  get identity()
  {
    return this._identity;
  },

  

  
  
  
  QueryInterface: function(iid)
  {
    if (iid.equals(Ci.nsIHttpServer) ||
        iid.equals(Ci.nsIServerSocketListener) ||
        iid.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },


  

  




  isStopped: function()
  {
    return this._socketClosed && !this._handler.hasPendingRequests();
  },

  
  

  





  _endConnection: function(connection)
  {
    
    
    
    
    
    
    

    connection.close();

    NS_ASSERT(this == connection.server);

    this._handler._pendingRequests--;

    connection.destroy();
  },

  


  _requestQuit: function()
  {
    dumpn(">>> requesting a quit");
    dumpStack();
    this._doQuit = true;
  }

};












const HOST_REGEX =
  new RegExp("^(?:" +
               
               "(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\\.)*" +
               
               "[a-z](?:[a-z0-9-]*[a-z0-9])?" +
             "|" +
               
               "\\d+\\.\\d+\\.\\d+\\.\\d+" +
             ")$",
             "i");















function ServerIdentity()
{
  
  this._primaryScheme = "http";

  
  this._primaryHost = "127.0.0.1"

  
  this._primaryPort = -1;

  



  this._defaultPort = -1;

  











  this._locations = { "xlocalhost": {} };
}
ServerIdentity.prototype =
{
  



  _initialize: function(port, addSecondaryDefault)
  {
    if (this._primaryPort !== -1)
      this.add("http", "localhost", port);
    else
      this.setPrimary("http", "localhost", port);
    this._defaultPort = port;

    
    if (addSecondaryDefault)
      this.add("http", "127.0.0.1", port);
  },

  




  _teardown: function()
  {
    
    this.remove("http", "127.0.0.1", this._defaultPort);

    
    
    if (this._primaryScheme == "http" &&
        this._primaryHost == "localhost" &&
        this._primaryPort == this._defaultPort)
    {
      
      
      var port = this._defaultPort;
      this._defaultPort = -1;
      this.remove("http", "localhost", port);

      
      this._primaryPort = -1;
    }
    else
    {
      
      this.remove("http", "localhost", this._defaultPort);
    }
  },

  
  
  
  get primaryScheme()
  {
    if (this._primaryPort === -1)
      throw Cr.NS_ERROR_NOT_INITIALIZED;
    return this._primaryScheme;
  },

  
  
  
  get primaryHost()
  {
    if (this._primaryPort === -1)
      throw Cr.NS_ERROR_NOT_INITIALIZED;
    return this._primaryHost;
  },

  
  
  
  get primaryPort()
  {
    if (this._primaryPort === -1)
      throw Cr.NS_ERROR_NOT_INITIALIZED;
    return this._primaryPort;
  },

  
  
  
  add: function(scheme, host, port)
  {
    this._validate(scheme, host, port);
    
    var entry = this._locations["x" + host];
    if (!entry)
      this._locations["x" + host] = entry = {};

    entry[port] = scheme;
  },

  
  
  
  remove: function(scheme, host, port)
  {
    this._validate(scheme, host, port);

    var entry = this._locations["x" + host];
    if (!entry)
      return false;

    var present = port in entry;
    delete entry[port];

    if (this._primaryScheme == scheme &&
        this._primaryHost == host &&
        this._primaryPort == port &&
        this._defaultPort !== -1)
    {
      
      
      this._primaryPort = -1;
      this._initialize(this._defaultPort, false);
    }

    return present;
  },

  
  
  
  has: function(scheme, host, port)
  {
    this._validate(scheme, host, port);

    return "x" + host in this._locations &&
           scheme === this._locations["x" + host][port];
  },
  
  
  
  
  getScheme: function(host, port)
  {
    this._validate("http", host, port);

    var entry = this._locations["x" + host];
    if (!entry)
      return "";

    return entry[port] || "";
  },
  
  
  
  
  setPrimary: function(scheme, host, port)
  {
    this._validate(scheme, host, port);

    this.add(scheme, host, port);

    this._primaryScheme = scheme;
    this._primaryHost = host;
    this._primaryPort = port;
  },

  





  _validate: function(scheme, host, port)
  {
    if (scheme !== "http" && scheme !== "https")
    {
      dumpn("*** server only supports http/https schemes: '" + scheme + "'");
      dumpStack();
      throw Cr.NS_ERROR_ILLEGAL_VALUE;
    }
    if (!HOST_REGEX.test(host))
    {
      dumpn("*** unexpected host: '" + host + "'");
      throw Cr.NS_ERROR_ILLEGAL_VALUE;
    }
    if (port < 0 || port > 65535)
    {
      dumpn("*** unexpected port: '" + port + "'");
      throw Cr.NS_ERROR_ILLEGAL_VALUE;
    }
  }
};















function Connection(input, output, server, port)
{
  
  this.input = input;

  
  this.output = output;

  
  this.server = server;

  
  this.port = port;
  
  
  this._closed = this._processed = false;
}
Connection.prototype =
{
  
  close: function()
  {
    this.input.close();
    this.output.close();
    this._closed = true;
  },

  






  process: function(request)
  {
    NS_ASSERT(!this._closed && !this._processed);

    this._processed = true;

    this.server._handler.handleResponse(this, request);
  },

  









  processError: function(code, metadata)
  {
    NS_ASSERT(!this._closed && !this._processed);

    this._processed = true;

    this.server._handler.handleError(code, this, metadata);
  },

  
  end: function()
  {
    this.server._endConnection(this);
  },

  
  destroy: function()
  {
    if (!this._closed)
      this.close();

    
    var server = this.server;
    if (server._doQuit)
      server.stop();
  }
};




function readBytes(inputStream, count)
{
  return new BinaryInputStream(inputStream).readByteArray(count);
}




const READER_INITIAL    = 0;
const READER_IN_HEADERS = 1;
const READER_IN_BODY    = 2;





















function RequestReader(connection)
{
  
  this._connection = connection;

  





  this._data = new LineData();

  this._contentLength = 0;

  
  this._state = READER_INITIAL;

  
  this._metadata = new Request(connection.port);

  






  this._lastHeaderName = this._lastHeaderValue = undefined;
}
RequestReader.prototype =
{
  

  







  onInputStreamReady: function(input)
  {
    dumpn("*** onInputStreamReady(input=" + input + ") on thread " +
          gThreadManager.currentThread + " (main is " +
          gThreadManager.mainThread + ")");
    dumpn("*** this._state == " + this._state);

    var count = input.available();

    
    
    if (!this._data)
      return;

    var moreAvailable = false;

    switch (this._state)
    {
      case READER_INITIAL:
        moreAvailable = this._processRequestLine(input, count);
        break;

      case READER_IN_HEADERS:
        moreAvailable = this._processHeaders(input, count);
        break;

      default:
        NS_ASSERT(false);
    }

    if (this._state == READER_IN_BODY && moreAvailable)
      moreAvailable = this._processBody(input, count);

    if (moreAvailable)
      input.asyncWait(this, 0, 0, gThreadManager.currentThread);
  },

  
  
  
  QueryInterface: function(aIID)
  {
    if (aIID.equals(Ci.nsIInputStreamCallback) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },


  

  










  _processRequestLine: function(input, count)
  {
    NS_ASSERT(this._state == READER_INITIAL);

    var data = this._data;
    data.appendBytes(readBytes(input, count));


    
    
    var line = {};
    var readSuccess;
    while ((readSuccess = data.readLine(line)) && line.value == "")
      dumpn("*** ignoring beginning blank line...");

    
    if (!readSuccess)
      return true;

    
    try
    {
      this._parseRequestLine(line.value);

      
      if (!this._parseHeaders())
        return true;

      dumpn("_processRequestLine, Content-length="+this._contentLength);
      if (this._contentLength > 0)
        return true;

      
      this._validateRequest();
      return this._handleResponse();
    }
    catch (e)
    {
      this._handleError(e);
      return false;
    }
  },

  










  _processHeaders: function(input, count)
  {
    NS_ASSERT(this._state == READER_IN_HEADERS);

    
    
    

    this._data.appendBytes(readBytes(input, count));

    try
    {
      
      if (!this._parseHeaders())
        return true;

      dumpn("_processHeaders, Content-length="+this._contentLength);
      if (this._contentLength > 0)
        return true;

      
      this._validateRequest();
      return this._handleResponse();
    }
    catch (e)
    {
      this._handleError(e);
      return false;
    }
  },

  _processBody: function(input, count)
  {
    NS_ASSERT(this._state == READER_IN_BODY);

    try
    {
      if (this._contentLength > 0)
      {
        var bodyData = this._data.purge();
        if (bodyData.length == 0)
        {
          if (count > this._contentLength)
            count = this._contentLength;

          bodyData = readBytes(input, count);
        }
        dumpn("*** loading data="+bodyData+" len="+bodyData.length);

        this._metadata._body.appendBytes(bodyData);
        this._contentLength -= bodyData.length;
      }

      dumpn("*** remainig body data len="+this._contentLength);
      if (this._contentLength > 0)
        return true;

      this._validateRequest();
      return this._handleResponse();
    }
    catch (e)
    {
      this._handleError(e);
      return false;
    }
  },

  





  _validateRequest: function()
  {
    NS_ASSERT(this._state == READER_IN_BODY);

    dumpn("*** _validateRequest");

    var metadata = this._metadata;
    var headers = metadata._headers;

    
    var identity = this._connection.server.identity;
    if (metadata._httpVersion.atLeast(nsHttpVersion.HTTP_1_1))
    {
      if (!headers.hasHeader("Host"))
      {
        dumpn("*** malformed HTTP/1.1 or greater request with no Host header!");
        throw HTTP_400;
      }

      
      
      
      
      if (!metadata._host)
      {
        var host, port;
        var hostPort = headers.getHeader("Host");
        var colon = hostPort.indexOf(":");
        if (colon < 0)
        {
          host = hostPort;
          port = "";
        }
        else
        {
          host = hostPort.substring(0, colon);
          port = hostPort.substring(colon + 1);
        }

        
        
        
        if (!HOST_REGEX.test(host) || !/^\d*$/.test(port))
        {
          dumpn("*** malformed hostname (" + hostPort + ") in Host " +
                "header, 400 time");
          throw HTTP_400;
        }

        
        
        
        
        
        port = +port || 80;

        var scheme = identity.getScheme(host, port);
        if (!scheme)
        {
          dumpn("*** unrecognized hostname (" + hostPort + ") in Host " +
                "header, 400 time");
          throw HTTP_400;
        }

        metadata._scheme = scheme;
        metadata._host = host;
        metadata._port = port;
      }
    }
    else
    {
      NS_ASSERT(metadata._host === undefined,
                "HTTP/1.0 doesn't allow absolute paths in the request line!");

      metadata._scheme = identity.primaryScheme;
      metadata._host = identity.primaryHost;
      metadata._port = identity.primaryPort;
    }

    NS_ASSERT(identity.has(metadata._scheme, metadata._host, metadata._port),
              "must have a location we recognize by now!");
  },

  








  _handleError: function(e)
  {
    var server = this._connection.server;
    if (e instanceof HttpError)
    {
      var code = e.code;
    }
    else
    {
      
      code = 500;
      server._requestQuit();
    }

    
    this._data = null;

    this._connection.processError(code, this._metadata);
  },

  









  _handleResponse: function()
  {
    NS_ASSERT(this._state == READER_IN_BODY);

    
    
    this._data = null;

    this._connection.process(this._metadata);

    return false;
  },


  

  





  _parseRequestLine: function(line)
  {
    NS_ASSERT(this._state == READER_INITIAL);

    dumpn("*** _parseRequestLine('" + line + "')");

    var metadata = this._metadata;

    
    
    var request = line.split(/[ \t]+/);
    if (!request || request.length != 3)
      throw HTTP_400;

    metadata._method = request[0];

    
    var ver = request[2];
    var match = ver.match(/^HTTP\/(\d+\.\d+)$/);
    if (!match)
      throw HTTP_400;

    
    try
    {
      metadata._httpVersion = new nsHttpVersion(match[1]);
      if (!metadata._httpVersion.atLeast(nsHttpVersion.HTTP_1_0))
        throw "unsupported HTTP version";
    }
    catch (e)
    {
      
      throw HTTP_501;
    }


    var fullPath = request[1];
    var serverIdentity = this._connection.server.identity;

    var scheme, host, port;

    if (fullPath.charAt(0) != "/")
    {
      
      if (!metadata._httpVersion.atLeast(nsHttpVersion.HTTP_1_1))
        throw HTTP_400;

      try
      {
        var uri = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService)
                    .newURI(fullPath, null, null);
        fullPath = uri.path;
        scheme = uri.scheme;
        host = metadata._host = uri.asciiHost;
        port = uri.port;
        if (port === -1)
        {
          if (scheme === "http")
            port = 80;
          else if (scheme === "https")
            port = 443;
          else
            throw HTTP_400;
        }
      }
      catch (e)
      {
        
        
        
        throw HTTP_400;
      }

      if (!serverIdentity.has(scheme, host, port) || fullPath.charAt(0) != "/")
        throw HTTP_400;
    }

    var splitter = fullPath.indexOf("?");
    if (splitter < 0)
    {
      
      metadata._path = fullPath;
    }
    else
    {
      metadata._path = fullPath.substring(0, splitter);
      metadata._queryString = fullPath.substring(splitter + 1);
    }

    metadata._scheme = scheme;
    metadata._host = host;
    metadata._port = port;

    
    this._state = READER_IN_HEADERS;
  },

  








  _parseHeaders: function()
  {
    NS_ASSERT(this._state == READER_IN_HEADERS);

    dumpn("*** _parseHeaders");

    var data = this._data;

    var headers = this._metadata._headers;
    var lastName = this._lastHeaderName;
    var lastVal = this._lastHeaderValue;

    var line = {};
    while (true)
    {
      NS_ASSERT(!((lastVal === undefined) ^ (lastName === undefined)),
                lastName === undefined ?
                  "lastVal without lastName?  lastVal: '" + lastVal + "'" :
                  "lastName without lastVal?  lastName: '" + lastName + "'");

      if (!data.readLine(line))
      {
        
        this._lastHeaderName = lastName;
        this._lastHeaderValue = lastVal;
        return false;
      }

      var lineText = line.value;
      var firstChar = lineText.charAt(0);

      
      if (lineText == "")
      {
        
        if (lastName)
        {
          try
          {
            headers.setHeader(lastName, lastVal, true);
          }
          catch (e)
          {
            dumpn("*** e == " + e);
            throw HTTP_400;
          }
        }
        else
        {
          
        }

        
        this._state = READER_IN_BODY;
        try
        {
          this._contentLength = parseInt(headers.getHeader("Content-Length"));
          dumpn("Content-Length="+this._contentLength);
        }
        catch (e) {}
        return true;
      }
      else if (firstChar == " " || firstChar == "\t")
      {
        
        if (!lastName)
        {
          
          throw HTTP_400;
        }

        
        
        lastVal += lineText;
      }
      else
      {
        
        if (lastName)
        {
          try
          {
            headers.setHeader(lastName, lastVal, true);
          }
          catch (e)
          {
            dumpn("*** e == " + e);
            throw HTTP_400;
          }
        }

        var colon = lineText.indexOf(":"); 
        if (colon < 1)
        {
          
          throw HTTP_400;
        }

        
        lastName = lineText.substring(0, colon);
        lastVal = lineText.substring(colon + 1);
      } 
    } 
  }
};



const CR = 0x0D, LF = 0x0A;













function findCRLF(array)
{
  for (var i = array.indexOf(CR); i >= 0; i = array.indexOf(CR, i + 1))
  {
    if (array[i + 1] == LF)
      return i;
  }
  return -1;
}






function LineData()
{
  
  this._data = [];
}
LineData.prototype =
{
  



  appendBytes: function(bytes)
  {
    Array.prototype.push.apply(this._data, bytes);
  },

  











  readLine: function(out)
  {
    var data = this._data;
    var length = findCRLF(data);
    if (length < 0)
      return false;

    
    
    
    
    
    
    
    
    
    
    var line = String.fromCharCode.apply(null, data.splice(0, length + 2));
    out.value = line.substring(0, length);

    return true;
  },

  






  purge: function()
  {
    var data = this._data;
    this._data = null;
    return data;
  }
};






function createHandlerFunc(handler)
{
  return function(metadata, response) { handler.handle(metadata, response); };
}






function defaultIndexHandler(metadata, response)
{
  response.setHeader("Content-Type", "text/html", false);

  var path = htmlEscape(decodeURI(metadata.path));

  
  
  
  
  

  var body = '<html>\
                <head>\
                  <title>' + path + '</title>\
                </head>\
                <body>\
                  <h1>' + path + '</h1>\
                  <ol style="list-style-type: none">';

  var directory = metadata.getProperty("directory");
  NS_ASSERT(directory && directory.isDirectory());

  var fileList = [];
  var files = directory.directoryEntries;
  while (files.hasMoreElements())
  {
    var f = files.getNext().QueryInterface(Ci.nsIFile);
    var name = f.leafName;
    if (!f.isHidden() &&
        (name.charAt(name.length - 1) != HIDDEN_CHAR ||
         name.charAt(name.length - 2) == HIDDEN_CHAR))
      fileList.push(f);
  }

  fileList.sort(fileSort);

  for (var i = 0; i < fileList.length; i++)
  {
    var file = fileList[i];
    try
    {
      var name = file.leafName;
      if (name.charAt(name.length - 1) == HIDDEN_CHAR)
        name = name.substring(0, name.length - 1);
      var sep = file.isDirectory() ? "/" : "";

      
      
      var item = '<li><a href="' + encodeURIComponent(name) + sep + '">' +
                   htmlEscape(name) + sep +
                 '</a></li>';

      body += item;
    }
    catch (e) {  }
  }

  body    += '    </ol>\
                </body>\
              </html>';

  response.bodyOutputStream.write(body, body.length);
}




function fileSort(a, b)
{
  var dira = a.isDirectory(), dirb = b.isDirectory();

  if (dira && !dirb)
    return -1;
  if (dirb && !dira)
    return 1;

  var namea = a.leafName.toLowerCase(), nameb = b.leafName.toLowerCase();
  return nameb > namea ? -1 : 1;
}














function toInternalPath(path, encoded)
{
  if (encoded)
    path = decodeURI(path);

  var comps = path.split("/");
  for (var i = 0, sz = comps.length; i < sz; i++)
  {
    var comp = comps[i];
    if (comp.charAt(comp.length - 1) == HIDDEN_CHAR)
      comps[i] = comp + HIDDEN_CHAR;
  }
  return comps.join("/");
}















function maybeAddHeaders(file, metadata, response)
{
  var name = file.leafName;
  if (name.charAt(name.length - 1) == HIDDEN_CHAR)
    name = name.substring(0, name.length - 1);

  var headerFile = file.parent;
  headerFile.append(name + HEADERS_SUFFIX);

  if (!headerFile.exists())
    return;

  const PR_RDONLY = 0x01;
  var fis = new FileInputStream(headerFile, PR_RDONLY, 0444,
                                Ci.nsIFileInputStream.CLOSE_ON_EOF);

  var lis = new ConverterInputStream(fis, "UTF-8", 1024, 0x0);
  lis.QueryInterface(Ci.nsIUnicharLineInputStream);

  try
  {
    var line = {value: ""};
    var more = lis.readLine(line);

    if (!more && line.value == "")
      return;


    

    var status = line.value;
    if (status.indexOf("HTTP ") == 0)
    {
      status = status.substring(5);
      var space = status.indexOf(" ");
      var code, description;
      if (space < 0)
      {
        code = status;
        description = "";
      }
      else
      {
        code = status.substring(0, space);
        description = status.substring(space + 1, status.length);
      }
    
      response.setStatusLine(metadata.httpVersion, parseInt(code, 10), description);

      line.value = "";
      more = lis.readLine(line);
    }

    
    while (more || line.value != "")
    {
      var header = line.value;
      var colon = header.indexOf(":");

      response.setHeader(header.substring(0, colon),
                         header.substring(colon + 1, header.length),
                         false); 

      line.value = "";
      more = lis.readLine(line);
    }
  }
  catch (e)
  {
    dumpn("WARNING: error in headers for " + metadata.path + ": " + e);
    throw HTTP_500;
  }
}












function ServerHandler(server)
{
  

  


  this._server = server;

  




  this._pendingRequests = 0;

  








  this._pathDirectoryMap = new FileMap();

  





  this._overridePaths = {};
  
  


  this._putDataOverrides = {};

  






  this._overrideErrors = {};

  



  this._mimeMappings = {};

  



  this._indexHandler = defaultIndexHandler;
}
ServerHandler.prototype =
{
  

  










  handleResponse: function(connection, metadata)
  {
    var response = new Response();

    var path = metadata.path;
    dumpn("*** path == " + path);

    try
    {
      try
      {
        if (metadata.method == "PUT")
        {
          
          var data = metadata.body.purge();
          data = String.fromCharCode.apply(null, data.splice(0, data.length + 2));
          var contentType;
          try
          {
            contentType = metadata.getHeader("Content-Type");
          }
          catch (ex)
          {
            contentType = "application/octet-stream";
          }

          if (data.length)
          {
            dumpn("PUT data \'"+data+"\' for "+path);
            this._putDataOverrides[path] =
              function(ametadata, aresponse)
              {
                aresponse.setStatusLine(metadata.httpVersion, 200, "OK");
                aresponse.setHeader("Content-Type", contentType, false);
                dumpn("*** writting PUT data=\'"+data+"\'");
                aresponse.bodyOutputStream.write(data, data.length);
              };
          }
          else
          {
            delete this._putDataOverrides[path];
            dumpn("clearing PUT data for "+path);
          }

          response.setStatusLine(metadata.httpVersion, 200, "OK");
        }
        else if (path in this._putDataOverrides)
        {
          
          
          dumpn("calling PUT data override for "+path);
          this._putDataOverrides[path](metadata, response);
        }
        else if (path in this._overridePaths)
        {
          
          
          dumpn("calling override for "+path);
          this._overridePaths[path](metadata, response);
        }
        else
          this._handleDefault(metadata, response);
      }
      catch (e)
      {
        response.recycle();

        if (!(e instanceof HttpError))
        {
          dumpn("*** unexpected error: e == " + e);
          throw HTTP_500;
        }
        if (e.code != 404)
          throw e;

        dumpn("*** default: " + (path in this._defaultPaths));

        if (path in this._defaultPaths)
          this._defaultPaths[path](metadata, response);
        else
          throw HTTP_404;
      }
    }
    catch (e)
    {
      var errorCode = "internal";

      try
      {
        if (!(e instanceof HttpError))
          throw e;

        errorCode = e.code;
        dumpn("*** errorCode == " + errorCode);

        response.recycle();

        this._handleError(errorCode, metadata, response);
      }
      catch (e2)
      {
        dumpn("*** error handling " + errorCode + " error: " +
              "e2 == " + e2 + ", shutting down server");

        response.destroy();
        connection.close();
        connection.server.stop();
        return;
      }
    }

    this._end(response, connection);
  },

  
  
  
  registerFile: function(path, file)
  {
    if (!file)
    {
      dumpn("*** unregistering '" + path + "' mapping");
      delete this._overridePaths[path];
      return;
    }

    dumpn("*** registering '" + path + "' as mapping to " + file.path);
    file = file.clone();

    var self = this;
    this._overridePaths[path] =
      function(metadata, response)
      {
        if (!file.exists())
          throw HTTP_404;

        response.setStatusLine(metadata.httpVersion, 200, "OK");
        self._writeFileResponse(metadata, file, response);
      };
  },

  
  
  
  registerPathHandler: function(path, handler)
  {
    
    if (path.charAt(0) != "/")
      throw Cr.NS_ERROR_INVALID_ARG;

    this._handlerToField(handler, this._overridePaths, path);
  },

  
  
  
  registerDirectory: function(path, directory)
  {
    
    
    
    
    var key = path.length == 1 ? "" : path.substring(1, path.length - 1);

    
    
    if (key.charAt(0) == "/")
      throw Cr.NS_ERROR_INVALID_ARG;

    key = toInternalPath(key, false);

    if (directory)
    {
      dumpn("*** mapping '" + path + "' to the location " + directory.path);
      this._pathDirectoryMap.put(key, directory);
    }
    else
    {
      dumpn("*** removing mapping for '" + path + "'");
      this._pathDirectoryMap.put(key, null);
    }
  },

  
  
  
  registerErrorHandler: function(err, handler)
  {
    if (!(err in HTTP_ERROR_CODES))
      dumpn("*** WARNING: registering non-HTTP/1.1 error code " +
            "(" + err + ") handler -- was this intentional?");

    this._handlerToField(handler, this._overrideErrors, err);
  },

  
  
  
  setIndexHandler: function(handler)
  {
    if (!handler)
      handler = defaultIndexHandler;
    else if (typeof(handler) != "function")
      handler = createHandlerFunc(handler);

    this._indexHandler = handler;
  },

  
  
  
  registerContentType: function(ext, type)
  {
    if (!type)
      delete this._mimeMappings[ext];
    else
      this._mimeMappings[ext] = headerUtils.normalizeFieldValue(type);
  },

  

  




  hasPendingRequests: function()
  {
    return this._pendingRequests > 0;
  },


  

  









  _handlerToField: function(handler, dict, key)
  {
    
    if (typeof(handler) == "function")
      dict[key] = handler;
    else if (handler)
      dict[key] = createHandlerFunc(handler);
    else
      delete dict[key];
  },

  













  _handleDefault: function(metadata, response)
  {
    response.setStatusLine(metadata.httpVersion, 200, "OK");

    var path = metadata.path;
    NS_ASSERT(path.charAt(0) == "/", "invalid path: <" + path + ">");

    
    
    var file = this._getFileForPath(path);

    
    
    if (file.exists() && file.isDirectory())
    {
      file.append("index.html"); 
      if (!file.exists() || file.isDirectory())
      {
        metadata._ensurePropertyBag();
        metadata._bag.setPropertyAsInterface("directory", file.parent);
        this._indexHandler(metadata, response);
        return;
      }
    }

    
    if (!file.exists())
      throw HTTP_404;

    
    dumpn("*** handling '" + path + "' as mapping to " + file.path);
    this._writeFileResponse(metadata, file, response);
  },

  










  _writeFileResponse: function(metadata, file, response)
  {
    const PR_RDONLY = 0x01;

    var type = this._getTypeFromFile(file);
    if (type == SJS_TYPE)
    {
      try
      {
        var fis = new FileInputStream(file, PR_RDONLY, 0444,
                                      Ci.nsIFileInputStream.CLOSE_ON_EOF);
        var sis = new ScriptableInputStream(fis);
        var s = Cu.Sandbox(gGlobalObject);
        Cu.evalInSandbox(sis.read(file.fileSize), s);
        s.handleRequest(metadata, response);
      }
      catch (e)
      {
        dumpn("*** error running SJS: " + e);
        throw HTTP_500;
      }
    }
    else
    {
      try
      {
        response.setHeader("Last-Modified",
                           toDateString(file.lastModifiedTime),
                           false);
      }
      catch (e) {  }

      response.setHeader("Content-Type", type, false);
  
      var fis = new FileInputStream(file, PR_RDONLY, 0444,
                                    Ci.nsIFileInputStream.CLOSE_ON_EOF);
      response.bodyOutputStream.writeFrom(fis, file.fileSize);
      fis.close();
      
      maybeAddHeaders(file, metadata, response);
    }
  },

  










  _getTypeFromFile: function(file)
  {
    try
    {
      var name = file.leafName;
      var dot = name.lastIndexOf(".");
      if (dot > 0)
      {
        var ext = name.slice(dot + 1);
        if (ext in this._mimeMappings)
          return this._mimeMappings[ext];
      }
      return Cc["@mozilla.org/uriloader/external-helper-app-service;1"]
               .getService(Ci.nsIMIMEService)
               .getTypeFromFile(file);
    }
    catch (e)
    {
      return "application/octet-stream";
    }
  },

  













  _getFileForPath: function(path)
  {
    
    try
    {
      path = toInternalPath(path, true);
    }
    catch (e)
    {
      throw HTTP_400; 
    }

    
    var pathMap = this._pathDirectoryMap;

    
    
    var tmp = path.substring(1);
    while (true)
    {
      
      var file = pathMap.get(tmp);
      if (file)
      {
        
        
        
        
        if (tmp == path.substring(1) &&
            tmp.length != 0 &&
            tmp.charAt(tmp.length - 1) != "/")
          file = null;
        else
          break;
      }

      
      if (tmp == "")
        break;

      tmp = tmp.substring(0, tmp.lastIndexOf("/"));
    }

    
    if (!file)
      throw HTTP_404;


    
    var parentFolder = file.parent;
    var dirIsRoot = (parentFolder == null);

    
    
    
    
    
    var leafPath = path.substring(tmp.length + 1);
    var comps = leafPath.split("/");
    for (var i = 0, sz = comps.length; i < sz; i++)
    {
      var comp = comps[i];

      if (comp == "..")
        file = file.parent;
      else if (comp == "." || comp == "")
        continue;
      else
        file.append(comp);

      if (!dirIsRoot && file.equals(parentFolder))
        throw HTTP_403;
    }

    return file;
  },

  








  handleError: function(errorCode, connection)
  {
    var response = new Response();

    dumpn("*** error in request: " + errorCode);

    try
    {
      this._handleError(errorCode, new Request(connection.port), response);
      this._end(response, connection);
    }
    catch (e)
    {
      connection.close();
      connection.server.stop();
    }
  }, 

  
















  _handleError: function(errorCode, metadata, response)
  {
    if (!metadata)
      throw Cr.NS_ERROR_NULL_POINTER;

    var errorX00 = errorCode - (errorCode % 100);

    try
    {
      if (!(errorCode in HTTP_ERROR_CODES))
        dumpn("*** WARNING: requested invalid error: " + errorCode);

      
      
      
      

      
      try
      {
        if (errorCode in this._overrideErrors)
          this._overrideErrors[errorCode](metadata, response);
        else if (errorCode in this._defaultErrors)
          this._defaultErrors[errorCode](metadata, response);
      }
      catch (e)
      {
        
        if (errorX00 == errorCode)
          throw HTTP_500;

        dumpn("*** error in handling for error code " + errorCode + ", " +
              "falling back to " + errorX00 + "...");
        if (errorX00 in this._overrideErrors)
          this._overrideErrors[errorX00](metadata, response);
        else if (errorX00 in this._defaultErrors)
          this._defaultErrors[errorX00](metadata, response);
        else
          throw HTTP_500;
      }
    }
    catch (e)
    {
      response.recycle();

      
      dumpn("*** error in handling for error code " + errorX00 + ", falling " +
            "back to 500...");

      try
      {
        if (500 in this._overrideErrors)
          this._overrideErrors[500](metadata, response);
        else
          this._defaultErrors[500](metadata, response);
      }
      catch (e2)
      {
        dumpn("*** multiple errors in default error handlers!");
        dumpn("*** e == " + e + ", e2 == " + e2);
        throw e2;
      }
    }
  },

  















  _end:  function(response, connection)
  {
    
    response.setHeader("Connection", "close", false);
    response.setHeader("Server", "httpd.js", false);
    response.setHeader("Date", toDateString(Date.now()), false);

    var bodyStream = response.bodyInputStream;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    try
    {
      var available = bodyStream.available();
    }
    catch (e)
    {
      available = 0;
    }

    response.setHeader("Content-Length", available.toString(), false);


    

    
    var preamble = "HTTP/" + response.httpVersion + " " +
                   response.httpCode + " " +
                   response.httpDescription + "\r\n";

    
    var head = response.headers;
    var headEnum = head.enumerator;
    while (headEnum.hasMoreElements())
    {
      var fieldName = headEnum.getNext()
                              .QueryInterface(Ci.nsISupportsString)
                              .data;
      preamble += fieldName + ": " + head.getHeader(fieldName) + "\r\n";
    }

    
    preamble += "\r\n";

    var outStream = connection.output;
    try
    {
      outStream.write(preamble, preamble.length);
    }
    catch (e)
    {
      
      
      
      response.destroy();
      connection.close();
      return;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    this._pendingRequests++;

    var server = this._server;

    
    
    
    
    if (available != 0)
    {
      





      var copyObserver =
        {
          onStartRequest: function(request, context) {  },

          





          onStopRequest: function(request, cx, statusCode)
          {
            
            
            
            
            
            
            
            if (!Components.isSuccessCode(statusCode))
            {
              dumpn("*** WARNING: non-success statusCode in onStopRequest: " +
                    statusCode);
            }

            
            response.destroy();

            connection.end();
          },

          QueryInterface: function(aIID)
          {
            if (aIID.equals(Ci.nsIRequestObserver) ||
                aIID.equals(Ci.nsISupports))
              return this;

            throw Cr.NS_ERROR_NO_INTERFACE;
          }
        };


      
      
      
      
      var copier = new StreamCopier(bodyStream, outStream,
                                    null,
                                    true, true, 8192);
      copier.asyncCopy(copyObserver, null);
    }
    else
    {
      
      response.destroy();
      this._server._endConnection(connection);
    }
  },

  

  


  _defaultErrors:
  {
    400: function(metadata, response)
    {
      
      response.setStatusLine("1.1", 400, "Bad Request");
      response.setHeader("Content-Type", "text/plain", false);

      var body = "Bad request\n";
      response.bodyOutputStream.write(body, body.length);
    },
    403: function(metadata, response)
    {
      response.setStatusLine(metadata.httpVersion, 403, "Forbidden");
      response.setHeader("Content-Type", "text/html", false);

      var body = "<html>\
                    <head><title>403 Forbidden</title></head>\
                    <body>\
                      <h1>403 Forbidden</h1>\
                    </body>\
                  </html>";
      response.bodyOutputStream.write(body, body.length);
    },
    404: function(metadata, response)
    {
      response.setStatusLine(metadata.httpVersion, 404, "Not Found");
      response.setHeader("Content-Type", "text/html", false);

      var body = "<html>\
                    <head><title>404 Not Found</title></head>\
                    <body>\
                      <h1>404 Not Found</h1>\
                      <p>\
                        <span style='font-family: monospace;'>" +
                          htmlEscape(metadata.path) +
                       "</span> was not found.\
                      </p>\
                    </body>\
                  </html>";
      response.bodyOutputStream.write(body, body.length);
    },
    500: function(metadata, response)
    {
      response.setStatusLine(metadata.httpVersion,
                             500,
                             "Internal Server Error");
      response.setHeader("Content-Type", "text/html", false);

      var body = "<html>\
                    <head><title>500 Internal Server Error</title></head>\
                    <body>\
                      <h1>500 Internal Server Error</h1>\
                      <p>Something's broken in this server and\
                        needs to be fixed.</p>\
                    </body>\
                  </html>";
      response.bodyOutputStream.write(body, body.length);
    },
    501: function(metadata, response)
    {
      response.setStatusLine(metadata.httpVersion, 501, "Not Implemented");
      response.setHeader("Content-Type", "text/html", false);

      var body = "<html>\
                    <head><title>501 Not Implemented</title></head>\
                    <body>\
                      <h1>501 Not Implemented</h1>\
                      <p>This server is not (yet) Apache.</p>\
                    </body>\
                  </html>";
      response.bodyOutputStream.write(body, body.length);
    },
    505: function(metadata, response)
    {
      response.setStatusLine("1.1", 505, "HTTP Version Not Supported");
      response.setHeader("Content-Type", "text/html", false);

      var body = "<html>\
                    <head><title>505 HTTP Version Not Supported</title></head>\
                    <body>\
                      <h1>505 HTTP Version Not Supported</h1>\
                      <p>This server only supports HTTP/1.0 and HTTP/1.1\
                        connections.</p>\
                    </body>\
                  </html>";
      response.bodyOutputStream.write(body, body.length);
    }
  },

  


  _defaultPaths:
  {
    "/": function(metadata, response)
    {
      response.setStatusLine(metadata.httpVersion, 200, "OK");
      response.setHeader("Content-Type", "text/html", false);

      var body = "<html>\
                    <head><title>MozJSHTTP</title></head>\
                    <body>\
                      <h1>MozJSHTTP</h1>\
                      <p>If you're seeing this page, MozJSHTTP is up and\
                        serving requests!  Now set a base path and serve some\
                        files!</p>\
                    </body>\
                  </html>";

      response.bodyOutputStream.write(body, body.length);
    },

    "/trace": function(metadata, response)
    {
      response.setStatusLine(metadata.httpVersion, 200, "OK");
      response.setHeader("Content-Type", "text/plain", false);

      var body = "Request-URI: " +
                 metadata.scheme + "://" + metadata.host + ":" + metadata.port +
                 metadata.path + "\n\n";
      body += "Request (semantically equivalent, slightly reformatted):\n\n";
      body += metadata.method + " " + metadata.path;

      if (metadata.queryString)
        body +=  "?" + metadata.queryString;
        
      body += " HTTP/" + metadata.httpVersion + "\r\n";

      var headEnum = metadata.headers;
      while (headEnum.hasMoreElements())
      {
        var fieldName = headEnum.getNext()
                                .QueryInterface(Ci.nsISupportsString)
                                .data;
        body += fieldName + ": " + metadata.getHeader(fieldName) + "\r\n";
      }

      response.bodyOutputStream.write(body, body.length);
    }
  }
};





function FileMap()
{
  
  this._map = {};
}
FileMap.prototype =
{
  

  








  put: function(key, value)
  {
    if (value)
      this._map[key] = value.clone();
    else
      delete this._map[key];
  },

  








  get: function(key)
  {
    var val = this._map[key];
    return val ? val.clone() : null;
  }
};











const IS_TOKEN_ARRAY =
  [0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 

   0, 1, 0, 1, 1, 1, 1, 1, 
   0, 0, 1, 1, 0, 1, 1, 0, 
   1, 1, 1, 1, 1, 1, 1, 1, 
   1, 1, 0, 0, 0, 0, 0, 0, 

   0, 1, 1, 1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 1, 1, 1, 
   1, 1, 1, 0, 0, 0, 1, 1, 

   1, 1, 1, 1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 1, 1, 1, 
   1, 1, 1, 0, 1, 0, 1];   










function isCTL(code)
{
  return (code >= 0 && code <= 31) || (code == 127);
}






function Response()
{
  
  
  this.recycle();
}
Response.prototype =
{
  

  
  
  
  get bodyOutputStream()
  {
    this._ensureAlive();

    if (!this._bodyOutputStream && !this._outputProcessed)
    {
      const PR_UINT32_MAX = Math.pow(2, 32) - 1;
      var pipe = new Pipe(false, false, 0, PR_UINT32_MAX, null);
      this._bodyOutputStream = pipe.outputStream;
      this._bodyInputStream = pipe.inputStream;
    }

    return this._bodyOutputStream;
  },

  
  
  
  write: function(data)
  {
    var dataAsString = String(data);
    this.bodyOutputStream.write(dataAsString, dataAsString.length);
  },

  
  
  
  setStatusLine: function(httpVersion, code, description)
  {
    this._ensureAlive();

    if (!(code >= 0 && code < 1000))
      throw Cr.NS_ERROR_INVALID_ARG;

    try
    {
      var httpVer;
      
      if (!httpVersion || httpVersion == "1.1")
        httpVer = nsHttpVersion.HTTP_1_1;
      else if (httpVersion == "1.0")
        httpVer = nsHttpVersion.HTTP_1_0;
      else
        httpVer = new nsHttpVersion(httpVersion);
    }
    catch (e)
    {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    
    
    
    
    
    if (!description)
      description = "";
    for (var i = 0; i < description.length; i++)
      if (isCTL(description.charCodeAt(i)) && description.charAt(i) != "\t")
        throw Cr.NS_ERROR_INVALID_ARG;

    
    this._httpDescription = description;
    this._httpCode = code;
    this._httpVersion = httpVer;
  },

  
  
  
  setHeader: function(name, value, merge)
  {
    this._ensureAlive();

    this._headers.setHeader(name, value, merge);
  },

  

  


  get httpVersion()
  {
    this._ensureAlive();
    return this._httpVersion.toString();
  },

  



  get httpCode()
  {
    this._ensureAlive();

    var codeString = (this._httpCode < 10 ? "0" : "") +
                     (this._httpCode < 100 ? "0" : "") +
                     this._httpCode;
    return codeString;
  },

  



  get httpDescription()
  {
    this._ensureAlive();

    return this._httpDescription;
  },

  


  get headers()
  {
    this._ensureAlive();

    return this._headers;
  },

  
  
  
  getHeader: function(name)
  {
    this._ensureAlive();

    return this._headers.getHeader(name);
  },

  






  get bodyInputStream()
  {
    this._ensureAlive();

    if (!this._outputProcessed)
    {
      
      
      if (!this._bodyOutputStream)
        this.bodyOutputStream.write("", 0);

      this._outputProcessed = true;
    }
    if (this._bodyOutputStream)
    {
      this._bodyOutputStream.close(); 
      this._bodyOutputStream = null;  
    }
    return this._bodyInputStream;
  },

  








  recycle: function()
  {
    if (this._bodyOutputStream)
    {
      this._bodyOutputStream.close();
      this._bodyOutputStream = null;
    }
    if (this._bodyInputStream)
    {
      this._bodyInputStream.close();
      this._bodyInputStream = null;
    }

    



    this._httpVersion = nsHttpVersion.HTTP_1_1;

    


    this._httpCode = 200;

    


    this._httpDescription = "OK";

    



    this._headers = new nsHttpHeaders();

    



    this._destroyed = false;

    



    this._outputProcessed = false;
  },

  









  destroy: function()
  {
    if (this._destroyed)
      return;

    if (this._bodyOutputStream)
    {
      this._bodyOutputStream.close();
      this._bodyOutputStream = null;
    }
    if (this._bodyInputStream)
    {
      this._bodyInputStream.close();
      this._bodyInputStream = null;
    }

    this._destroyed = true;
  },

  

  
  _ensureAlive: function()
  {
    if (this._destroyed)
      throw Cr.NS_ERROR_FAILURE;
  }
};





const headerUtils =
{
  










  normalizeFieldName: function(fieldName)
  {
    if (fieldName == "")
      throw Cr.NS_ERROR_INVALID_ARG;

    for (var i = 0, sz = fieldName.length; i < sz; i++)
    {
      if (!IS_TOKEN_ARRAY[fieldName.charCodeAt(i)])
      {
        dumpn(fieldName + " is not a valid header field name!");
        throw Cr.NS_ERROR_INVALID_ARG;
      }
    }

    return fieldName.toLowerCase();
  },

  












  normalizeFieldValue: function(fieldValue)
  {
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    var val = fieldValue.replace(/(?:(?:\r\n)?[ \t]+)+/g, " ");

    
    val = val.replace(/^ +/, "").replace(/ +$/, "");

    
    for (var i = 0, len = val.length; i < len; i++)
      if (isCTL(val.charCodeAt(i)))
        throw Cr.NS_ERROR_INVALID_ARG;

    
    
    
    return val;
  }
};












function htmlEscape(str)
{
  
  var s = "";
  for (var i = 0; i < str.length; i++)
    s += "&#" + str.charCodeAt(i) + ";";
  return s;
}











function nsHttpVersion(versionString)
{
  var matches = /^(\d+)\.(\d+)$/.exec(versionString);
  if (!matches)
    throw "Not a valid HTTP version!";

  
  this.major = parseInt(matches[1], 10);

  
  this.minor = parseInt(matches[2], 10);

  if (isNaN(this.major) || isNaN(this.minor) ||
      this.major < 0    || this.minor < 0)
    throw "Not a valid HTTP version!";
}
nsHttpVersion.prototype =
{
  



  toString: function ()
  {
    return this.major + "." + this.minor;
  },

  






  equals: function (otherVersion)
  {
    return this.major == otherVersion.major &&
           this.minor == otherVersion.minor;
  },

  
  atLeast: function(otherVersion)
  {
    return this.major > otherVersion.major ||
           (this.major == otherVersion.major &&
            this.minor >= otherVersion.minor);
  }
};

nsHttpVersion.HTTP_1_0 = new nsHttpVersion("1.0");
nsHttpVersion.HTTP_1_1 = new nsHttpVersion("1.1");











function nsHttpHeaders()
{
  











  this._headers = {};
}
nsHttpHeaders.prototype =
{
  









  setHeader: function(fieldName, fieldValue, merge)
  {
    var name = headerUtils.normalizeFieldName(fieldName);
    var value = headerUtils.normalizeFieldValue(fieldValue);

    if (merge && name in this._headers)
      this._headers[name] = this._headers[name] + "," + value;
    else
      this._headers[name] = value;
  },

  











  getHeader: function(fieldName)
  {
    var name = headerUtils.normalizeFieldName(fieldName);

    if (name in this._headers)
      return this._headers[name];
    else
      throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  










  hasHeader: function(fieldName)
  {
    var name = headerUtils.normalizeFieldName(fieldName);
    return (name in this._headers);
  },

  





  get enumerator()
  {
    var headers = [];
    for (var i in this._headers)
    {
      var supports = new SupportsString();
      supports.data = i;
      headers.push(supports);
    }

    return new nsSimpleEnumerator(headers);
  }
};








function nsSimpleEnumerator(items)
{
  this._items = items;
  this._nextIndex = 0;
}
nsSimpleEnumerator.prototype =
{
  hasMoreElements: function()
  {
    return this._nextIndex < this._items.length;
  },
  getNext: function()
  {
    if (!this.hasMoreElements())
      throw Cr.NS_ERROR_NOT_AVAILABLE;

    return this._items[this._nextIndex++];
  },
  QueryInterface: function(aIID)
  {
    if (Ci.nsISimpleEnumerator.equals(aIID) ||
        Ci.nsISupports.equals(aIID))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};








function Request(port)
{
  
  this._method = "";

  
  this._path = "";

  
  this._queryString = "";

  
  this._scheme = "http";

  
  this._host = undefined;

  
  this._port = port;

  
  this._body = new LineData();

  


  this._headers = new nsHttpHeaders();

  




  this._bag = null;
}
Request.prototype =
{
  

  
  
  
  get scheme()
  {
    return this._scheme;
  },

  
  
  
  get host()
  {
    return this._host;
  },

  
  
  
  get port()
  {
    return this._port;
  },

  

  
  
  
  get method()
  {
    return this._method;
  },

  
  
  
  get httpVersion()
  {
    return this._httpVersion.toString();
  },

  
  
  
  get path()
  {
    return this._path;
  },

  
  
  
  get queryString()
  {
    return this._queryString;
  },

  

  
  
  
  getHeader: function(name)
  {
    return this._headers.getHeader(name);
  },

  
  
  
  hasHeader: function(name)
  {
    return this._headers.hasHeader(name);
  },

  
  
  
  get headers()
  {
    return this._headers.enumerator;
  },

  
  
  
  get enumerator()
  {
    this._ensurePropertyBag();
    return this._bag.enumerator;
  },

  
  
  
  getProperty: function(name) 
  {
    this._ensurePropertyBag();
    return this._bag.getProperty(name);
  },
  
  
  _ensurePropertyBag: function()
  {
    if (!this._bag)
      this._bag = new WritablePropertyBag();
  },

  get body()
  {
    return this._body;
  }
};








function makeFactory(ctor)
{
  function ci(outer, iid)
  {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return (new ctor()).QueryInterface(iid);
  } 

  return {
           createInstance: ci,
           lockFactory: function(lock) { },
           QueryInterface: function(aIID)
           {
             if (Ci.nsIFactory.equals(aIID) ||
                 Ci.nsISupports.equals(aIID))
               return this;
             throw Cr.NS_ERROR_NO_INTERFACE;
           }
         };
}


const module =
{
  
  QueryInterface: function(aIID)
  {
    if (Ci.nsIModule.equals(aIID) ||
        Ci.nsISupports.equals(aIID))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  
  registerSelf: function(compMgr, fileSpec, location, type)
  {
    compMgr = compMgr.QueryInterface(Ci.nsIComponentRegistrar);
    
    for (var key in this._objects)
    {
      var obj = this._objects[key];
      compMgr.registerFactoryLocation(obj.CID, obj.className, obj.contractID,
                                               fileSpec, location, type);
    }
  },
  unregisterSelf: function (compMgr, location, type)
  {
    compMgr = compMgr.QueryInterface(Ci.nsIComponentRegistrar);

    for (var key in this._objects)
    {
      var obj = this._objects[key];
      compMgr.unregisterFactoryLocation(obj.CID, location);
    }
  },
  getClassObject: function(compMgr, cid, iid)
  {
    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    for (var key in this._objects)
    {
      if (cid.equals(this._objects[key].CID))
        return this._objects[key].factory;
    }
    
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  canUnload: function(compMgr)
  {
    return true;
  },

  
  _objects:
  {
    server:
    {
      CID:         Components.ID("{54ef6f81-30af-4b1d-ac55-8ba811293e41}"),
      contractID:  "@mozilla.org/server/jshttp;1",
      className:   "MozJSHTTP server",
      factory:     makeFactory(nsHttpServer)
    }
  }
};



function NSGetModule(compMgr, fileSpec)
{
  return module;
}





























function server(port, basePath)
{
  if (basePath)
  {
    var lp = Cc["@mozilla.org/file/local;1"]
               .createInstance(Ci.nsILocalFile);
    lp.initWithPath(basePath);
  }

  
  DEBUG = true;

  var srv = new nsHttpServer();
  if (lp)
    srv.registerDirectory("/", lp);
  srv.registerContentType("sjs", SJS_TYPE);
  srv.identity.setPrimary("http", "localhost", port);
  srv.start(port);

  var thread = gThreadManager.currentThread;
  while (!srv.isStopped())
    thread.processNextEvent(true);

  
  while (thread.hasPendingEvents())
    thread.processNextEvent(true);

  DEBUG = false;
}
