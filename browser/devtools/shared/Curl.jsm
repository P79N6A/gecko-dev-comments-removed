




































"use strict";

this.EXPORTED_SYMBOLS = ["Curl", "CurlUtils"];

Components.utils.import("resource://gre/modules/Services.jsm");

const DEFAULT_HTTP_VERSION = "HTTP/1.1";

this.Curl = {
  














  generateCommand: function(aData) {
    let utils = CurlUtils;

    let command = ["curl"];
    let ignoredHeaders = new Set();

    
    
    let escapeString = Services.appinfo.OS == "WINNT" ?
                       utils.escapeStringWin : utils.escapeStringPosix;

    
    command.push(escapeString(aData.url));

    let postDataText = null;
    let multipartRequest = utils.isMultipartRequest(aData);

    
    let data = [];
    if (utils.isUrlEncodedRequest(aData) || aData.method == "PUT") {
      postDataText = aData.postDataText;
      data.push("--data");
      data.push(escapeString(utils.writePostDataTextParams(postDataText)));
      ignoredHeaders.add("Content-Length");
    } else if (multipartRequest) {
      postDataText = aData.postDataText;
      data.push("--data-binary");
      let boundary = utils.getMultipartBoundary(aData);
      let text = utils.removeBinaryDataFromMultipartText(postDataText, boundary);
      data.push(escapeString(text));
      ignoredHeaders.add("Content-Length");
    }

    
    
    
    if (!(aData.method == "GET" || aData.method == "POST")) {
      command.push("-X");
      command.push(aData.method);
    }

    
    
    
    if (aData.method == "HEAD") {
      command.push("-I");
    }

    
    if (aData.httpVersion && aData.httpVersion != DEFAULT_HTTP_VERSION) {
      command.push("--" + aData.httpVersion.split("/")[1]);
    }

    
    let headers = aData.headers;
    if (multipartRequest) {
      let multipartHeaders = utils.getHeadersFromMultipartText(postDataText);
      headers = headers.concat(multipartHeaders);
    }
    for (let i = 0; i < headers.length; i++) {
      let header = headers[i];
      if (header.name === "Accept-Encoding"){
        command.push("--compressed");
        continue;
      }
      if (ignoredHeaders.has(header.name)) {
        continue;
      }
      command.push("-H");
      command.push(escapeString(header.name + ": " + header.value));
    }

    
    command = command.concat(data);

    return command.join(" ");
  }
};




this.CurlUtils = {
  







  isUrlEncodedRequest: function(aData) {
    let postDataText = aData.postDataText;
    if (!postDataText) {
      return false;
    }

    postDataText = postDataText.toLowerCase();
    if (postDataText.includes("content-type: application/x-www-form-urlencoded")) {
      return true;
    }

    let contentType = this.findHeader(aData.headers, "content-type");

    return (contentType &&
      contentType.toLowerCase().includes("application/x-www-form-urlencoded"));
  },

  







  isMultipartRequest: function(aData) {
    let postDataText = aData.postDataText;
    if (!postDataText) {
      return false;
    }

    postDataText = postDataText.toLowerCase();
    if (postDataText.includes("content-type: multipart/form-data")) {
      return true;
    }

    let contentType = this.findHeader(aData.headers, "content-type");

    return (contentType &&
      contentType.toLowerCase().includes("multipart/form-data;"));
  },

  







  writePostDataTextParams: function(aPostDataText) {
    let lines = aPostDataText.split("\r\n");
    return lines[lines.length - 1];
  },

  









  findHeader: function(aHeaders, aName) {
    if (!aHeaders) {
      return null;
    }

    let name = aName.toLowerCase();
    for (let header of aHeaders) {
      if (name == header.name.toLowerCase()) {
        return header.value;
      }
    }

    return null;
  },

  







  getMultipartBoundary: function(aData) {
    let boundaryRe = /\bboundary=(-{3,}\w+)/i;

    
    let contentType = this.findHeader(aData.headers, "Content-Type");
    if (boundaryRe.test(contentType)) {
      return contentType.match(boundaryRe)[1];
    }
    
    
    
    let boundaryString = aData.postDataText.match(boundaryRe)[1];
    if (boundaryString) {
      return boundaryString;
    }

    return null;
  },

  









  removeBinaryDataFromMultipartText: function(aMultipartText, aBoundary) {
    let result = "";
    let boundary = "--" + aBoundary;
    let parts = aMultipartText.split(boundary);
    for (let part of parts) {
      
      let contentDispositionLine = part.trimLeft().split("\r\n")[0];
      if (!contentDispositionLine) {
        continue;
      }
      contentDispositionLine = contentDispositionLine.toLowerCase();
      if (contentDispositionLine.includes("content-disposition: form-data")) {
        if (contentDispositionLine.includes("filename=")) {
          
          
          let headers = part.split("\r\n\r\n")[0];
          result += boundary + "\r\n" + headers + "\r\n\r\n";
        }
        else {
          result += boundary + "\r\n" + part;
        }
      }
    }
    result += aBoundary + "--\r\n";

    return result;
  },

  







  getHeadersFromMultipartText: function(aMultipartText) {
    let headers = [];
    if (!aMultipartText || aMultipartText.startsWith("---")) {
      return headers;
    }

    
    let index = aMultipartText.indexOf("\r\n\r\n");
    if (index == -1) {
      return headers;
    }

    
    let headersText = aMultipartText.substring(0, index);
    let headerLines = headersText.split("\r\n");
    let lastHeaderName = null;

    for (let line of headerLines) {
      
      
      
      if (lastHeaderName && /^\s+/.test(line)) {
        headers.push({ name: lastHeaderName, value: line.trim() });
        continue;
      }

      let indexOfColon = line.indexOf(":");
      if (indexOfColon == -1) {
        continue;
      }

      let header = [line.slice(0, indexOfColon), line.slice(indexOfColon + 1)];
      if (header.length != 2) {
        continue;
      }
      lastHeaderName = header[0].trim();
      headers.push({ name: lastHeaderName, value: header[1].trim() });
    }

    return headers;
  },

  



  escapeStringPosix: function(str) {
    function escapeCharacter(x) {
      let code = x.charCodeAt(0);
      if (code < 256) {
        
        return code < 16 ? "\\x0" + code.toString(16) : "\\x" + code.toString(16);
      }
      code = code.toString(16);
      return "\\u" + ("0000" + code).substr(code.length, 4);
    }

    if (/[^\x20-\x7E]|\'/.test(str)) {
      
      return "$\'" + str.replace(/\\/g, "\\\\")
                        .replace(/\'/g, "\\\'")
                        .replace(/\n/g, "\\n")
                        .replace(/\r/g, "\\r")
                        .replace(/[^\x20-\x7E]/g, escapeCharacter) + "'";
    } else {
      
      return "'" + str + "'";
    }
  },

  



  escapeStringWin: function(str) {
    













    return "\"" + str.replace(/"/g, "\"\"")
                     .replace(/%/g, "\"%\"")
                     .replace(/\\/g, "\\\\")
                     .replace(/[\r\n]+/g, "\"^$&\"") + "\"";
  }
};
