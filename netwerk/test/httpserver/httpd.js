















































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const CC = Components.Constructor;


var DEBUG = false; 







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



function dumpn(str)
{
  if (DEBUG)
    dump(str + "\n");
}







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
  
  this._port = undefined;

  
  this._socket = null;

  
  this._handler = new ServerHandler(this);

  


  this._doQuit = false;

  



  this._socketClosed = true;
}
nsHttpServer.prototype =
{
  

  






  onSocketAccepted: function(serverSocket, trans)
  {
    dumpn(">>> new connection on " + trans.host + ":" + trans.port);

    var input = trans.openInputStream(Ci.nsITransport.OPEN_BLOCKING, 0, 0);
    var output = trans.openOutputStream(Ci.nsITransport.OPEN_BLOCKING, 0, 0);

    this._processConnection(serverSocket.port, input, output);
  },

  




  onStopListening: function(serverSocket, status)
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

    var socket = Cc["@mozilla.org/network/server-socket;1"]
                   .createInstance(Ci.nsIServerSocket);
    socket.init(this._port,
                true,       
                -1);        

    dumpn(">>> listening on port " + socket.port);
    socket.asyncListen(this);
    this._socket = socket;
  },

  
  
  
  stop: function()
  {
    if (!this._socket)
      return;

    dumpn(">>> stopping listening on port " + this._socket.port);
    this._socket.close();
    this._socket = null;
    this._doQuit = false;

    
    
    
    
    if ("@mozilla.org/thread-manager;1" in Cc)
    {
      var thr = Cc["@mozilla.org/thread-manager;1"]
                  .getService()
                  .currentThread;
      while (!this._socketClosed || this._handler.hasPendingRequests())
        thr.processNextEvent(true);
    }
  },

  
  
  
  registerFile: function(path, file)
  {
    if (!file.exists() || lf.isDirectory())
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
  
  

  










  _processConnection: function(port, inStream, outStream)
  {
    try
    {
      var metadata = new RequestMetadata(port);
      metadata.init(inStream);

      inStream.close();

      this._handler.handleRequest(outStream, metadata);
    }
    catch (e)
    {
      dumpn(">>> internal error, shutting down server: " + e);
      dumpn("*** stack trace: " + e.stack);

      inStream.close(); 

      this._doQuit = true;
      this._endConnection(this._handler, outStream);
    }

    
    
  },

  








  _endConnection: function(handler, outStream)
  {
    
    
    
    
    
    
    

    outStream.close();  
    handler._pendingRequests--;

    
    
    
    
    
    if (this._doQuit)
      this.stop();
  },

  


  _requestQuit: function()
  {
    dumpn(">>> requesting a quit");
    this._doQuit = true;
  }

};










function getTypeFromFile(file)
{
  try
  {
    var type = Cc["@mozilla.org/uriloader/external-helper-app-service;1"]
                 .getService(Ci.nsIMIMEService)
                 .getTypeFromFile(file);
  }
  catch (e)
  {
    type = "application/octet-stream";
  }
  return type;
}






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












function ServerHandler(srv)
{
  

  


  this._server = srv;

  




  this._pendingRequests = 0;

  








  this._pathDirectoryMap = new FileMap();

  





  this._overridePaths = {};

  






  this._overrideErrors = {};

  



  this._indexHandler = defaultIndexHandler;
}
ServerHandler.prototype =
{
  

  









  handleRequest: function(outStream, metadata)
  {
    var response = new Response();

    
    if (metadata.errorCode)
    {
      dumpn("*** errorCode == " + metadata.errorCode);
      this._handleError(metadata, response);
      return this._end(response, outStream);
    }

    var path = metadata.path;
    dumpn("*** path == " + path);

    try
    {
      try
      {
        
        
        if (path in this._overridePaths)
          this._overridePaths[path](metadata, response);
        else
          this._handleDefault(metadata, response);
      }
      catch (e)
      {
        response.recycle();

        if (!(e instanceof HttpError) || e.code != 404)
          throw HTTP_500;

        if (path in this._defaultPaths)
          this._defaultPaths[path](metadata, response);
        else
          throw HTTP_404;
      }
    }
    catch (e2)
    {
      if (!(e2 instanceof HttpError))
      {
        dumpn("*** internal error: e2 == " + e2);
        throw e2;
      }

      var errorCode = e2.code;
      dumpn("*** errorCode == " + errorCode);

      response.recycle();

      metadata.errorCode = errorCode;
      this._handleError(metadata, response);
    }

    return this._end(response, outStream);
  },

  









  registerFile: function(path, file)
  {
    dumpn("*** registering '" + path + "' as mapping to " + file.path);
    file = file.clone();

    this._overridePaths[path] =
      function(metadata, response)
      {
        if (!file.exists())
          throw HTTP_404;

        response.setStatusLine(metadata.httpVersion, 200, "OK");

        try
        {
          this._writeFileResponse(file, response);
        }
        catch (e)
        {
          
          
          throw e;
        }

        maybeAddHeaders(file, metadata, response);
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

  










  _handlerToField: function(handler, dict, key)
  {
    
    if (typeof(handler) == "function")
      dict[key] = handler;
    else if (handler)
      dict[key] = createHandlerFunc(handler);
    else
      delete dict[key];
  },

  




  hasPendingRequests: function()
  {
    return this._pendingRequests > 0;
  },

  

  













  _handleDefault: function(metadata, response)
  {
    try
    {
      response.setStatusLine(metadata.httpVersion, 200, "OK");

      var path = metadata.path;
      NS_ASSERT(path.charAt(0) == "/");

      
      
      var file = this._getFileForPath(path);

      
      
      if (file.exists() && file.isDirectory())
      {
        file.append("index.html"); 
        if (!file.exists() || file.isDirectory())
        {
          metadata._bag.setPropertyAsInterface("directory", file.parent);
          this._indexHandler(metadata, response);
          return;
        }
      }

      
      if (!file.exists())
        throw HTTP_404;

      
      dumpn("*** handling '" + path + "' as mapping to " + file.path);
      this._writeFileResponse(file, response);

      maybeAddHeaders(file, metadata, response);
    }
    catch (e)
    {
      
      throw e;
    }
  },

  








  _writeFileResponse: function(file, response)
  {
    try
    {
      response.setHeader("Last-Modified",
                         toDateString(file.lastModifiedTime),
                         false);
    }
    catch (e) {  }

    response.setHeader("Content-Type", getTypeFromFile(file), false);

    const PR_RDONLY = 0x01;
    var fis = new FileInputStream(file, PR_RDONLY, 0444,
                                  Ci.nsIFileInputStream.CLOSE_ON_EOF);
    response.bodyOutputStream.writeFrom(fis, file.fileSize);
    fis.close();
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

  














  _handleError: function(metadata, response)
  {
    if (!metadata)
      throw Cr.NS_ERROR_NULL_POINTER;

    var errorCode = metadata.errorCode;
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

  













  _end:  function(response, outStream)
  {
    try
    {
      
      response.setHeader("Connection", "close", false);
      response.setHeader("Server", "MozJSHTTP", false);
      response.setHeader("Date", toDateString(Date.now()), false);

      var bodyStream = response.bodyInputStream
                               .QueryInterface(Ci.nsIInputStream);

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
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
      outStream.write(preamble, preamble.length);


      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      this._pendingRequests++;

      
      
      
      
      if (available != 0)
      {
        
        var server = this._server;
        var handler = this;

        





        var copyObserver =
          {
            onStartRequest: function(request, context) {  },

            





            onStopRequest: function(request, context, statusCode)
            {
              
              if (!Components.isSuccessCode(statusCode))
                server._requestQuit();

              
              response.destroy();

              server._endConnection(handler, outStream);
            },

            QueryInterface: function(aIID)
            {
              if (aIID.equals(Ci.nsIRequestObserver) ||
                  aIID.equals(Ci.nsISupports))
                return this;

              throw Cr.NS_ERROR_NO_INTERFACE;
            }
          };


        
        
        var copier = new StreamCopier(bodyStream, outStream, null,
                                      true, true, 8192);
        copier.asyncCopy(copyObserver, null);
      }
      else
      {
        
        response.destroy();
        this._server._endConnection(this, outStream);
      }
    }
    catch (e)
    {
      
      
      throw e;
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

      var body = "Request (semantically equivalent, slightly reformatted):\n\n";
      body += metadata.method + " " + metadata.path;

      if (metadata.queryString)
        body +=  "?" + metadata.queryString;
        
      body += " HTTP/" + metadata.httpVersion + "\n";

      var headEnum = metadata.headers;
      while (headEnum.hasMoreElements())
      {
        var fieldName = headEnum.getNext()
                                .QueryInterface(Ci.nsISupportsString)
                                .data;
        body += fieldName + ": " + metadata.getHeader(fieldName) + "\n";
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
  


  major: undefined,

  


  minor: undefined,

  



  toString: function ()
  {
    return this.major + "." + this.minor;
  },

  



  equals: function (otherVersion)
  {
    return this.major == otherVersion.major &&
           this.minor == otherVersion.minor;
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












function RequestMetadata(port)
{
  this._method = "";
  this._path = "";
  this._queryString = "";
  this._host = "";
  this._port = port;

  


  this._headers = new nsHttpHeaders();

  



  this._bag = new WritablePropertyBag();

  





  this.errorCode = 0;
}
RequestMetadata.prototype =
{
  

  
  
  
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
    return this._bag.enumerator;
  },

  
  
  
  getProperty: function(name) 
  {
    return this._bag.getProperty(name);
  },

  

  


  get bodyStream()
  {
    
    
    return null;
  },

  

  



  errorCode: 0,

  
  init: function(input)
  {
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    var lis = new ConverterInputStream(input, "ISO-8859-1", 1024, 0xFFFD);
    lis.QueryInterface(Ci.nsIUnicharLineInputStream);


    this._parseRequestLine(lis);
    if (this.errorCode)
      return;

    this._parseHeaders(lis);
    if (this.errorCode)
      return;

    

    
    if (!this._headers.hasHeader("Host") &&
        this._httpVersion.equals(nsHttpVersion.HTTP_1_1))
    {
      this.errorCode = 400;
      return;
    }

    
    this._host = "localhost";
  },

  

  







  _parseRequestLine: function(lis)
  {
    
    
    var line = {};
    while (lis.readLine(line) && line.value == "")
      dumpn("*** ignoring beginning blank line...");

    
    
    var request = line.value.split(/[ \t]+/);
    if (!request || request.length != 3)
    {
      this.errorCode = 400;
      return;
    }

    this._method = request[0];

    
    var ver = request[2];
    var match = ver.match(/^HTTP\/(\d+\.\d+)$/);
    if (!match)
    {
      this.errorCode = 400;
      return;
    }

    
    if (request[0] != "GET" && request[0] != "POST")
    {
      this.errorCode = 501;
      return;
    }

    
    try
    {
      this._httpVersion = new nsHttpVersion(match[1]);
      if (!this._httpVersion.equals(nsHttpVersion.HTTP_1_0) &&
          !this._httpVersion.equals(nsHttpVersion.HTTP_1_1))
        throw "unsupported HTTP version";
    }
    catch (e)
    {
      
      this.errorCode = 501;
      return;
    }

    var fullPath = request[1];

    if (fullPath.charAt(0) != "/")
    {
      
      
      try
      {
        var uri = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService)
                    .newURI(fullPath, null, null);
        fullPath = uri.path;
      }
      catch (e) {  }
      if (fullPath.charAt(0) != "/")
      {
        this.errorCode = 400;
        return;
      }
    }

    var splitter = fullPath.indexOf("?");
    if (splitter < 0)
    {
      
      this._path = fullPath;
    }
    else
    {
      this._path = fullPath.substring(0, splitter);
      this._queryString = fullPath.substring(splitter + 1);
    }
  },

  







  _parseHeaders: function(lis)
  {
    var headers = this._headers;
    var lastName, lastVal;

    var line = {};
    while (true)
    {
      NS_ASSERT((lastVal === undefined && lastName === undefined) ||
                (lastVal !== undefined && lastName !== undefined),
                lastName === undefined ?
                  "lastVal without lastName?  lastVal: '" + lastVal + "'" :
                  "lastName without lastVal?  lastName: '" + lastName + "'");

      lis.readLine(line);
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
            this.errorCode = 400;
            return;
          }
        }
        else
        {
          
        }

        
        break;
      }
      else if (firstChar == " " || firstChar == "\t")
      {
        
        if (!lastName)
        {
          
          this.errorCode = 400;
          return;
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
            this.errorCode = 400;
            return;
          }
        }

        var colon = lineText.indexOf(":"); 
        if (colon < 1)
        {
          
          this.errorCode = 400;
          return;
        }

        
        lastName = lineText.substring(0, colon);
        lastVal = lineText.substring(colon + 1);
      } 
    } 
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
  srv.start(port);

  var thread = Cc["@mozilla.org/thread-manager;1"]
                 .getService()
                 .currentThread;
  while (!srv.isStopped())
    thread.processNextEvent(true);

  
  while (thread.hasPendingEvents())
    thread.processNextEvent(true);

  DEBUG = false;
}
