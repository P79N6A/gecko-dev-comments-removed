(function(){

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;



var homeWindow = Cc["@mozilla.org/embedcomp/window-watcher;1"]
    .getService(Ci.nsIWindowWatcher)
    .activeWindow;

var consoleService = Cc["@mozilla.org/consoleservice;1"]
    .getService(Components.interfaces.nsIConsoleService);

var extensionManager = Cc["@mozilla.org/extensions/manager;1"]  
    .getService(Ci.nsIExtensionManager);  


window.Point = function(x, y) {
  this.x = (typeof(x) == 'undefined' ? 0 : x);
  this.y = (typeof(y) == 'undefined' ? 0 : y);
}


window.Rect = function(a, top, width, height) {
  if(typeof(a.left) != 'undefined' && typeof(a.top) != 'undefined'
      && typeof(a.right) != 'undefined' && typeof(a.bottom) != 'undefined') {
    this.left = a.left;
    this.top = a.top;
    this.width = a.width;
    this.height = a.height;
  } else {
    this.left = a;
    this.top = top;
    this.width = width;
    this.height = height;
  }
}

window.Rect.prototype = {
  
  get right() {
    return this.left + this.width;
  },
  
  
  set right(value) {
      this.width = value - this.left;
  },

  
  get bottom() {
    return this.top + this.height;
  },
  
  
  set bottom(value) {
      this.height = value - this.top;
  },
  
  
  intersects: function(rect) {
    return (rect.right > this.left
        && rect.left < this.right
        && rect.bottom > this.top
        && rect.top < this.bottom);      
  },
  
  
  center: function() {
    return new Point(this.left + (this.width / 2), this.top + (this.height / 2));
  },
  
  
  inset: function(a, b) {
    if(typeof(a.x) != 'undefined' && typeof(a.y) != 'undefined') {
      b = a.y; 
      a = a.x;
    }
    
    this.left += a;
    this.width -= a * 2;
    this.top += b;
    this.height -= b * 2;
  },
  
  
  offset: function(a, b) {
    if(typeof(a.x) != 'undefined' && typeof(a.y) != 'undefined') {
      this.left += a.x;
      this.top += a.y;
    } else {
      this.left += a;
      this.top += b;
    }
  },
  
  
  equals: function(a) {
    return (a.left == this.left
        && a.top == this.top
        && a.right == this.right
        && a.bottom == this.bottom);
  },
  
  union: function(a){
    var newLeft = Math.min(a.left, this.left);
    var newTop = Math.min(a.top, this.top);
    var newWidth = Math.max(a.right, this.right) - newLeft;
    var newHeight = Math.max(a.bottom, this.bottom) - newTop;
    var newRect = new Rect(newLeft, newTop, newWidth, newHeight); 
  
    return newRect;
  }
};



window.Subscribable = function() {
  this.onCloseSubscribers = [];
};

window.Subscribable.prototype = {
  
  addOnClose: function(referenceElement, callback) {
    var existing = jQuery.grep(this.onCloseSubscribers, function(element) {
      return element.referenceElement == referenceElement;
    });
    
    if(existing.size) {
      Utils.assert('should only ever be one', existing.size == 1);
      existing[0].callback = callback;
    } else {  
      this.onCloseSubscribers.push({
        referenceElement: referenceElement, 
        callback: callback
      });
    }
  },
  
  
  removeOnClose: function(referenceElement) {
    this.onCloseSubscribers = jQuery.grep(this.onCloseSubscribers, function(element) {
      return element.referenceElement == referenceElement;
    }, true);
  },
  
  
  _sendOnClose: function() {
    jQuery.each(this.onCloseSubscribers, function(index, object) { 
      object.callback(this);
    });
  }
};


var Utils = {
  
  get activeWindow(){
    var win = Cc["@mozilla.org/embedcomp/window-watcher;1"]
               .getService(Ci.nsIWindowWatcher)
               .activeWindow;
               
    if( win != null ) return win;  
    else return homeWindow;
  },
  
  get activeTab(){
    var tabBrowser = this.activeWindow.gBrowser;
    return tabBrowser.selectedTab;
  },
  
  get homeTab(){
    for( var i=0; i<Tabs.length; i++){
      if(Tabs[i].contentWindow.location.host == "tabcandy"){
        return Tabs[i];
      }
    }
    
    return null;
  },
  
  
  getInstallDirectory: function(id) { 
    var file = extensionManager.getInstallLocation(id).getItemFile(id, "install.rdf");  
    return file.parent;  
  }, 
  
  getFiles: function(dir) {
    var files = [];
    if(dir.isReadable() && dir.isDirectory) {
      var entries = dir.directoryEntries;
      while(entries.hasMoreElements()) {
        var entry = entries.getNext();
        entry.QueryInterface(Ci.nsIFile);
        files.push(entry);
      }
    }
    
    return files;
  },

  getVisualizationNames: function() {
    var names = [];
    var dir = this.getInstallDirectory('tabcandy@aza.raskin');  
    dir.append('content');
    dir.append('candies');
    var files = this.getFiles(dir);
    var count = files.length;
    var a;
    for(a = 0; a < count; a++) {
      var file = files[a];
      if(file.isDirectory()) 
        names.push(file.leafName);
    }

    return names;
  },
    
  
  
  
  ilog: function(){ 
    
    if( window.firebug ){
      window.firebug.d.console.cmd.log.apply(null, arguments);
      return;
    }
    
    
    $('<link rel="stylesheet" href="../../js/firebuglite/firebug-lite.css"/>')
      .appendTo("head");
    
    $('<script src="../../js/firebuglite/firebug-lite.js"></script>')
      .appendTo("body");
    
    var args = arguments;
    
    (function(){
      var fb = window.firebug;
      if(fb && fb.version){
        fb.init();
        fb.win.setHeight(100);
        fb.d.console.cmd.log.apply(null, args);
        }
      else{setTimeout(arguments.callee);}
    })();
  },
  
  log: function() { 
    var text = this.expandArgumentsForLog(arguments);
    consoleService.logStringMessage(text);
  }, 
  
  error: function() { 
    var text = this.expandArgumentsForLog(arguments);
    Cu.reportError('tabcandy error: ' + text);
  }, 
  
  trace: function() { 
    var text = this.expandArgumentsForLog(arguments);
    if(typeof(printStackTrace) != 'function')
      this.log(text + ' trace: you need to include stacktrace.js');
    else {
      var calls = printStackTrace();
      calls.splice(0, 3); 
      this.log('trace: ' + text + '\n' + calls.join('\n'));
    }
  }, 
  
  assert: function(label, condition) {
    if(!condition) {
      var text = 'tabcandy assert: ' + label;        
      if(typeof(printStackTrace) == 'function') {
        var calls = printStackTrace();
        text += '\n' + calls[3];
      }
      
      Cu.reportError(text);
    }
  },
  
  expandObject: function(obj) {
      var s = obj + ' = {';
      for(prop in obj) {
        var value = obj[prop]; 
        s += prop + ': ';
        if(typeof(value) == 'string')
          s += '\'' + value + '\'';
        else if(typeof(value) == 'function')
          s += 'function';
        else
          s += value;

        s += ", ";
      }
      return s + '}';
    }, 
    
  expandArgumentsForLog: function(args) {
    var s = '';
    var count = args.length;
    var a;
    for(a = 0; a < count; a++) {
      var arg = args[a];
      if(typeof(arg) == 'object')
        arg = this.expandObject(arg);
        
      s += arg;
      if(a < count - 1)
        s += '; ';
    }
    
    return s;
  },
  
  testLogging: function() {
    this.log('beginning logging test'); 
    this.error('this is an error');
    this.trace('this is a trace');
    this.log(1, null, {'foo': 'hello', 'bar': 2}, 'whatever');
    this.log('ending logging test');
  }, 
  
  
  isRightClick: function(event) {
    if(event.which)
      return (event.which == 3);
    else if(event.button) 
      return (event.button == 2);
    
    return false;
  },
  
  
  getMilliseconds: function() {
  	var date = new Date();
  	return date.getTime();
  },
  
  
  getBounds: function(el) {
    var $el = $(el);
    return new Rect(
      parseInt($el.css('left')), 
      parseInt($el.css('top')),
      $el.width(),
      $el.height()
    );
  },

  
  isJQuery: function(object) {
    
    return (object && typeof(object.fadeIn) == 'function' ? true : false);
  }   
};

window.Utils = Utils;

})();
