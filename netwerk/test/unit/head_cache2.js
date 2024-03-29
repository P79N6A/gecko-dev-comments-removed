const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

function newCacheBackEndUsed()
{
  var cache1srv = Components.classes["@mozilla.org/network/cache-service;1"]
                            .getService(Components.interfaces.nsICacheService);
  var cache2srv = Components.classes["@mozilla.org/netwerk/cache-storage-service;1"]
                            .getService(Components.interfaces.nsICacheStorageService);

  return cache1srv.cacheIOTarget != cache2srv.ioTarget;
}

var callbacks = new Array();


const NORMAL =               0;

const NEW =             1 << 0;

const NOTVALID =        1 << 1;

const THROWAVAIL =      1 << 2;

const READONLY =        1 << 3;

const NOTFOUND =        1 << 4;

const REVAL =           1 << 5;

const PARTIAL =         1 << 6

const DOOMED =          1 << 7;

const WAITFORWRITE =    1 << 8;

const METAONLY =        1 << 9;

const RECREATE =        1 << 10;

const NOTWANTED =       1 << 11;

const COMPLETE =        1 << 12;

const DONTFILL =        1 << 13;

const DONTSETVALID =    1 << 14;

const NOTIFYBEFOREREAD = 1 << 15;

var log_c2 = true;
function LOG_C2(o, m)
{
  if (!log_c2) return;
  if (!m)
    dump("TEST-INFO | CACHE2: " + o + "\n");
  else
    dump("TEST-INFO | CACHE2: callback #" + o.order + "(" + (o.workingData ? o.workingData.substr(0, 10) : "---") + ") " + m + "\n");
}

function pumpReadStream(inputStream, goon)
{
  if (inputStream.isNonBlocking()) {
    
    var pump = Cc["@mozilla.org/network/input-stream-pump;1"]
               .createInstance(Ci.nsIInputStreamPump);
    pump.init(inputStream, -1, -1, 0, 0, true);
    var data = "";
    pump.asyncRead({
      onStartRequest: function (aRequest, aContext) { },
      onDataAvailable: function (aRequest, aContext, aInputStream, aOffset, aCount)
      {
        var wrapper = Cc["@mozilla.org/scriptableinputstream;1"].
                      createInstance(Ci.nsIScriptableInputStream);
        wrapper.init(aInputStream);
        var str = wrapper.read(wrapper.available());
        LOG_C2("reading data '" + str.substring(0,5) + "'");
        data += str;
      },
      onStopRequest: function (aRequest, aContext, aStatusCode)
      {
        LOG_C2("done reading data: " + aStatusCode);
        do_check_eq(aStatusCode, Cr.NS_OK);
        goon(data);
      },
    }, null);
  }
  else {
    
    var data = read_stream(inputStream, inputStream.available());
    goon(data);
  }
}

OpenCallback.prototype =
{
  QueryInterface: function listener_qi(iid) {
    if (iid.equals(Ci.nsISupports) ||
        iid.equals(Ci.nsICacheEntryOpenCallback)) {
      return this;
    }
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  onCacheEntryCheck: function(entry, appCache)
  {
    LOG_C2(this, "onCacheEntryCheck");
    do_check_true(!this.onCheckPassed);
    this.onCheckPassed = true;

    if (this.behavior & NOTVALID) {
      LOG_C2(this, "onCacheEntryCheck DONE, return ENTRY_WANTED");
      return Ci.nsICacheEntryOpenCallback.ENTRY_WANTED;
    }

    if (this.behavior & NOTWANTED) {
      LOG_C2(this, "onCacheEntryCheck DONE, return ENTRY_NOT_WANTED");
      return Ci.nsICacheEntryOpenCallback.ENTRY_NOT_WANTED;
    }

    do_check_eq(entry.getMetaDataElement("meto"), this.workingMetadata);

    
    do_check_neq(this.behavior & (REVAL|PARTIAL), REVAL|PARTIAL);

    if (this.behavior & (REVAL|PARTIAL)) {
      LOG_C2(this, "onCacheEntryCheck DONE, return ENTRY_NEEDS_REVALIDATION");
      return Ci.nsICacheEntryOpenCallback.ENTRY_NEEDS_REVALIDATION;
    }

    if (this.behavior & COMPLETE) {
      LOG_C2(this, "onCacheEntryCheck DONE, return RECHECK_AFTER_WRITE_FINISHED");
      if (newCacheBackEndUsed()) {
        
        
        
        
        
        
        this.onCheckPassed = false;
        
        this.behavior &= ~COMPLETE;
      }
      return Ci.nsICacheEntryOpenCallback.RECHECK_AFTER_WRITE_FINISHED;
    }

    LOG_C2(this, "onCacheEntryCheck DONE, return ENTRY_WANTED");
    return Ci.nsICacheEntryOpenCallback.ENTRY_WANTED;
  },
  onCacheEntryAvailable: function(entry, isnew, appCache, status)
  {
    LOG_C2(this, "onCacheEntryAvailable, " + this.behavior);
    do_check_true(!this.onAvailPassed);
    this.onAvailPassed = true;

    do_check_eq(isnew, !!(this.behavior & NEW));

    if (this.behavior & (NOTFOUND|NOTWANTED)) {
      do_check_eq(status, Cr.NS_ERROR_CACHE_KEY_NOT_FOUND);
      do_check_false(!!entry);
      if (this.behavior & THROWAVAIL)
        this.throwAndNotify(entry);
      this.goon(entry);
    }
    else if (this.behavior & (NEW|RECREATE)) {
      do_check_true(!!entry);

      if (this.behavior & RECREATE) {
        entry = entry.recreate();
        do_check_true(!!entry);
      }

      if (this.behavior & THROWAVAIL)
        this.throwAndNotify(entry);

      if (!(this.behavior & WAITFORWRITE))
        this.goon(entry);

      if (!(this.behavior & PARTIAL)) {
        try {
          entry.getMetaDataElement("meto");
          do_check_true(false);
        }
        catch (ex) {}
      }

      if (this.behavior & DONTFILL) {
        do_check_false(this.behavior & WAITFORWRITE);
        return;
      }

      var self = this;
      do_execute_soon(function() { 
        entry.setMetaDataElement("meto", self.workingMetadata);
        entry.metaDataReady();
        if (self.behavior & METAONLY) {
          
          if (!(self.behavior & DONTSETVALID))
            entry.setValid();

          entry.close();
          if (self.behavior & WAITFORWRITE)
            self.goon(entry);

          return;
        }
        do_execute_soon(function() { 
          if (self.behavior & DOOMED) {
            try {
              var os = entry.openOutputStream(0);
              do_check_true(false);
            } catch (ex) {
              do_check_true(true);
            }
            if (self.behavior & WAITFORWRITE)
              self.goon(entry);
            return;
          }

          var offset = (self.behavior & PARTIAL)
            ? entry.dataSize
            : 0;
          LOG_C2(self, "openOutputStream @ " + offset);
          var os = entry.openOutputStream(offset);
          LOG_C2(self, "writing data");
          var wrt = os.write(self.workingData, self.workingData.length);
          do_check_eq(wrt, self.workingData.length);
          os.close();
          if (self.behavior & WAITFORWRITE)
            self.goon(entry);

          entry.close();
        })
      })
    }
    else { 
      do_check_true(!!entry);
      do_check_eq(entry.getMetaDataElement("meto"), this.workingMetadata);
      if (this.behavior & THROWAVAIL)
        this.throwAndNotify(entry);
      if (this.behavior & NOTIFYBEFOREREAD)
        this.goon(entry, true);

      var wrapper = Cc["@mozilla.org/scriptableinputstream;1"].
                    createInstance(Ci.nsIScriptableInputStream);
      var self = this;
      pumpReadStream(entry.openInputStream(0), function(data) {
        do_check_eq(data, self.workingData);
        self.onDataCheckPassed = true;
        LOG_C2(self, "entry read done");
        self.goon(entry);
        entry.close();
      });
    }
  },
  selfCheck: function()
  {
    LOG_C2(this, "selfCheck");

    do_check_true(this.onCheckPassed);
    do_check_true(this.onAvailPassed);
    do_check_true(this.onDataCheckPassed);
  },
  throwAndNotify: function(entry)
  {
    LOG_C2(this, "Throwing");
    var self = this;
    do_execute_soon(function() {
      LOG_C2(self, "Notifying");
      self.goon(entry);
    });
    throw Cr.NS_ERROR_FAILURE;
  }
};

function OpenCallback(behavior, workingMetadata, workingData, goon)
{
  this.behavior = behavior;
  this.workingMetadata = workingMetadata;
  this.workingData = workingData;
  this.goon = goon;
  this.onCheckPassed = (!!(behavior & (NEW|RECREATE)) || !workingMetadata) && !(behavior & NOTVALID);
  this.onAvailPassed = false;
  this.onDataCheckPassed = !!(behavior & (NEW|RECREATE|NOTWANTED)) || !workingMetadata;
  callbacks.push(this);
  this.order = callbacks.length;
}

VisitCallback.prototype =
{
  QueryInterface: function listener_qi(iid) {
    if (iid.equals(Ci.nsISupports) ||
        iid.equals(Ci.nsICacheStorageVisitor)) {
      return this;
    }
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  onCacheStorageInfo: function(num, consumption)
  {
    LOG_C2(this, "onCacheStorageInfo: num=" + num + ", size=" + consumption);
    do_check_eq(this.num, num);
    if (newCacheBackEndUsed()) {
      
      do_check_eq(this.consumption, consumption);
    }
    if (!this.entries)
      this.notify();
  },
  onCacheEntryInfo: function(aURI, aIdEnhance, aDataSize, aFetchCount, aLastModifiedTime, aExpirationTime)
  {
    var key = (aIdEnhance ? (aIdEnhance + ":") : "") + aURI.asciiSpec;
    LOG_C2(this, "onCacheEntryInfo: key=" + key);

    do_check_true(!!this.entries);

    var index = this.entries.indexOf(key);
    do_check_true(index > -1);

    this.entries.splice(index, 1);
  },
  onCacheEntryVisitCompleted: function()
  {
    LOG_C2(this, "onCacheEntryVisitCompleted");
    if (this.entries)
      do_check_eq(this.entries.length, 0);
    this.notify();
  },
  notify: function()
  {
    do_check_true(!!this.goon);
    var goon = this.goon;
    this.goon = null;
    do_execute_soon(goon);
  },
  selfCheck: function()
  {
    do_check_true(!this.entries || !this.entries.length);
  }
};

function VisitCallback(num, consumption, entries, goon)
{
  this.num = num;
  this.consumption = consumption;
  this.entries = entries;
  this.goon = goon;
  callbacks.push(this);
  this.order = callbacks.length;
}

EvictionCallback.prototype =
{
  QueryInterface: function listener_qi(iid) {
    if (iid.equals(Ci.nsISupports) ||
        iid.equals(Ci.nsICacheEntryDoomCallback)) {
      return this;
    }
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  onCacheEntryDoomed: function(result)
  {
    do_check_eq(this.expectedSuccess, result == Cr.NS_OK);
    this.goon();
  },
  selfCheck: function() {}
}

function EvictionCallback(success, goon)
{
  this.expectedSuccess = success;
  this.goon = goon;
  callbacks.push(this);
  this.order = callbacks.length;
}

MultipleCallbacks.prototype =
{
  fired: function()
  {
    if (--this.pending == 0)
    {
      var self = this;
      if (this.delayed)
        do_execute_soon(function() { self.goon(); });
      else
        this.goon();
    }
  }
}

function MultipleCallbacks(number, goon, delayed)
{
  this.pending = number;
  this.goon = goon;
  this.delayed = delayed;
}

function finish_cache2_test()
{
  callbacks.forEach(function(callback, index) {
    callback.selfCheck();
  });
  do_test_finished();
}
