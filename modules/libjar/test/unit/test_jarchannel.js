








const {classes: Cc,
       interfaces: Ci,
       results: Cr,
       Constructor: ctor
       } = Components;

const ios = Cc["@mozilla.org/network/io-service;1"].
                getService(Ci.nsIIOService);
const dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                getService(Ci.nsIProperties);
const obs = Cc["@mozilla.org/observer-service;1"].
                getService(Ci.nsIObserverService);

const nsIBinaryInputStream = ctor("@mozilla.org/binaryinputstream;1",
                               "nsIBinaryInputStream",
                               "setInputStream"
                               );

const fileBase = "test_bug637286.zip";
const file = do_get_file("data/" + fileBase);

const jarBase = "jar:" + filePrefix + ios.newFileURI(file).spec + "!";
const tmpDir = dirSvc.get("TmpD", Ci.nsIFile);

function Listener(callback) {
    this._callback = callback;
}
Listener.prototype = {
    gotStartRequest: false,
    available: -1,
    gotStopRequest: false,
    QueryInterface: function(iid) {
        if (iid.equals(Ci.nsISupports) ||
            iid.equals(Ci.nsIRequestObserver))
            return this;
        throw Cr.NS_ERROR_NO_INTERFACE;
    },
    onDataAvailable: function(request, ctx, stream, offset, count) {
        try {
            this.available = stream.available();
            do_check_eq(this.available, count);
            
            new nsIBinaryInputStream(stream).readBytes(count);
        }
        catch (ex) {
            do_throw(ex);
        }
    },
    onStartRequest: function(request, ctx) {
        this.gotStartRequest = true;
    },
    onStopRequest: function(request, ctx, status) {
        this.gotStopRequest = true;
        do_check_eq(status, 0);
        if (this._callback) {
            this._callback.call(null, this);
        }
    }
};




function testAsync() {
    var uri = jarBase + "/inner40.zip";
    var chan = ios.newChannel(uri, null, null);
    do_check_true(chan.contentLength < 0);
    chan.asyncOpen(new Listener(function(l) {
        do_check_true(chan.contentLength > 0);
        do_check_true(l.gotStartRequest);
        do_check_true(l.gotStopRequest);
        do_check_eq(l.available, chan.contentLength);

        run_next_test();
    }), null);
}

add_test(testAsync);

add_test(testAsync);





function testZipEntry() {
    var uri = jarBase + "/inner40.zip";
    var chan = ios.newChannel(uri, null, null).QueryInterface(Ci.nsIJARChannel);
    var entry = chan.zipEntry;
    do_check_true(entry.CRC32 == 0x8b635486);
    do_check_true(entry.realSize == 184);
    run_next_test();
}

add_test(testZipEntry);





if (!inChild) {

  


  add_test(function testSync() {
      var uri = jarBase + "/inner40.zip";
      var chan = ios.newChannel(uri, null, null);
      var stream = chan.open();
      do_check_true(chan.contentLength > 0);
      do_check_eq(stream.available(), chan.contentLength);
      stream.close();
      stream.close(); 

      run_next_test();
  });


  


  add_test(function testSyncNested() {
      var uri = "jar:" + jarBase + "/inner40.zip!/foo";
      var chan = ios.newChannel(uri, null, null);
      var stream = chan.open();
      do_check_true(chan.contentLength > 0);
      do_check_eq(stream.available(), chan.contentLength);
      stream.close();
      stream.close(); 

      run_next_test();
  });

  


  add_test(function testAsyncNested(next) {
      var uri = "jar:" + jarBase + "/inner40.zip!/foo";
      var chan = ios.newChannel(uri, null, null);
      chan.asyncOpen(new Listener(function(l) {
          do_check_true(chan.contentLength > 0);
          do_check_true(l.gotStartRequest);
          do_check_true(l.gotStopRequest);
          do_check_eq(l.available, chan.contentLength);

          run_next_test();
      }), null);
  });

  



  add_test(function testSyncCloseUnlocks() {
      var copy = tmpDir.clone();
      copy.append(fileBase);
      file.copyTo(copy.parent, copy.leafName);

      var uri = "jar:" + ios.newFileURI(copy).spec + "!/inner40.zip";
      var chan = ios.newChannel(uri, null, null);
      var stream = chan.open();
      do_check_true(chan.contentLength > 0);
      stream.close();

      
      obs.notifyObservers(null, "chrome-flush-caches", null);

      try {
          copy.remove(false);
      }
      catch (ex) {
          do_throw(ex);
      }

      run_next_test();
  });

  



  add_test(function testAsyncCloseUnlocks() {
      var copy = tmpDir.clone();
      copy.append(fileBase);
      file.copyTo(copy.parent, copy.leafName);

      var uri = "jar:" + ios.newFileURI(copy).spec + "!/inner40.zip";
      var chan = ios.newChannel(uri, null, null);
      chan.asyncOpen(new Listener(function (l) {
          do_check_true(chan.contentLength > 0);

          
          obs.notifyObservers(null, "chrome-flush-caches", null);

          try {
              copy.remove(false);
          }
          catch (ex) {
              do_throw(ex);
          }

          run_next_test();
      }), null);
  });

} 

if (inChild) {
    


    add_test(function testSimultaneous() {
        var uri = jarBase + "/inner1.zip";

        
        obs.notifyObservers(null, "chrome-flush-caches", null);

        
        var chan_first = ios.newChannel(uri, null, null)
                            .QueryInterface(Ci.nsIJARChannel);
        chan_first.asyncOpen(new Listener(function(l) {
        }), null);

        
        var num = 10;
        var chan = [];
        for (var i = 0; i < num; i++) {
            chan[i] = ios.newChannel(uri, null, null)
                         .QueryInterface(Ci.nsIJARChannel);
            chan[i].ensureChildFd();
            chan[i].asyncOpen(new Listener(function(l) {
            }), null);
        }

        
        var chan_last = ios.newChannel(uri, null, null)
                           .QueryInterface(Ci.nsIJARChannel);
        chan_last.ensureChildFd();
        chan_last.asyncOpen(new Listener(function(l) {
            run_next_test();
        }), null);
    });
} 

function run_test() run_next_test();
