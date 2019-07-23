




































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

var qaTools = {
	
	
	
	
	loadJsonMenu : function(url, menulist, nameMethod, valueMethod, callback) {
		var d = loadJSONDoc(url);
		d.addErrback(function (err) { 
        if (err instanceof CancelledError) { 
          return; 
        }
        alert(err);
      });
	  d.addCallback(function(obj) {
	      if (obj instanceof Array) {
	          for (var i=0; i<obj.length; i++) {
	              var item = obj[i];
	              if (! item) { continue; }
	              var newitem = menulist.appendItem(item[nameMethod], 
	              									item[valueMethod]);
	          }
	      } else {
	          var newitem = menulist.appendItem(obj[nameMethod], obj[valueMethod]);
	      }
	      
	      
	      newitem.userData = item;
	      if (callback) {
	      	callback();
	      }
	  });
	},
	fetchFeed : function(url, callback) {
		var httpRequest = null;
		function FeedResultListener() {}
		FeedResultListener.prototype = {
			handleResult : function(result) {
				var feed = result.doc;
				feed.QueryInterface(Ci.nsIFeed);
				callback(feed);
			},
		};
		
		function infoReceived() {
			var data = httpRequest.responseText;
			var ioService = Cc['@mozilla.org/network/io-service;1']
							   .getService(Ci.nsIIOService);
			var uri = ioService.newURI(url, null, null);
			if (data.length) {
				var processor = Cc["@mozilla.org/feed-processor;1"]
								   .createInstance(Ci.nsIFeedProcessor);
				try {
					processor.listener = new FeedResultListener;
					processor.parseFromString(data, uri);
				} catch(e) {
					alert("Error parsing feed: " + e);
				}
			}
		}
		httpRequest = new XMLHttpRequest();
		httpRequest.open("GET", url, true);
		try {
			httpRequest.onload = infoReceived;
			httpRequest.send(null);
		} catch(e) {
			alert(e);
		}
	},
	httpPostRequest : function (url, data, callback, errback) {
		
		var req = getXMLHttpRequest();
		req.open("POST", url, true);
	    req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	    req.setRequestHeader("Content-length", data.length);
	    req.setRequestHeader("Connection", "close");
	    req = sendXMLHttpRequest(req, data);
	    req.addErrback(errback);
	    req.addCallback(callback);
	},
	showHideLoadingMessage : function(box, bool) {
		if (bool == true) { 
			var loading = document.createElementNS("http://www.w3.org/1999/xhtml", "p");
			loading.textContent = qaMain.bundle.getString("qa.extension.loading");
			loading.setAttributeNS("http://www.w3.org/1999/xhtml", "class", "loading_message");
			box.appendChild(loading);
		} else { 
			var elements = box.childNodes;
			for (var i=0; i<elements.length; i++) {
				if (elements[i] && elements[i].getAttributeNS && 
				  elements[i].getAttributeNS(
				  "http://www.w3.org/1999/xhtml", "class") == "loading_message") {
					box.removeChild(elements[i]);
					break;
				}
			}
		}
	},
    arrayify : function(obj) {
        if (obj instanceof Array) {
            return obj;
        }
        var newArray = new Array();
        newArray[0] = obj;
        return newArray;
    },
    writeSafeHTML : function(elementID, htmlstr) {
        document.getElementById(elementID).innerHTML = "";  
        var gUnescapeHTML = Components.classes["@mozilla.org/feed-unescapehtml;1"].getService(Components.interfaces.nsIScriptableUnescapeHTML);
        var context = document.getElementById(elementID);
        var fragment = gUnescapeHTML.parseFragment(htmlstr, false, null, context);
        context.appendChild(fragment);
        
    },
    
    linkTargetsToBlank : function(node) {
        var children = node.getElementsByTagName('a');
        for (var i = 0; i < children.length; i++)
            children[i].setAttribute("target", "_blank");
    }
};
