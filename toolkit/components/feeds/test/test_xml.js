
























"use strict";



function FeedListener(testcase) {
  this.testcase = testcase;
}

FeedListener.prototype = {
  handleResult: function(result) {
    var feed = result.doc;
    try {
      do_print(true, "Testing feed " + this.testcase.file.path);
      Assert.ok(isIID(feed, Ci.nsIFeed), "Has feed interface");

      if (!eval(this.testcase.expect)) {
        Assert.ok(false, "expect failed for " + this.testcase.desc);
      } else {
        Assert.ok(true, "expect passed for " + this.testcase.desc);
      }
    } catch(e) {
      Assert.ok(false, "expect failed for " + this.testcase.desc + " ---- " + e.message);
    }

    run_next_test();
  }
}

function createTest(data) {
  return function() {
    var uri;

    if (data.base == null) {
      uri = NetUtil.newURI('http://example.org/' + data.path);
    } else {
      uri = data.base;
    }

    do_print("Testing " + data.file.leafName);

    var parser = Cc["@mozilla.org/feed-processor;1"].createInstance(Ci.nsIFeedProcessor);
    var stream = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(Ci.nsIFileInputStream);
    stream.init(data.file, 0x01, parseInt("0444", 8), 0);
    parser.listener = new FeedListener(data);

    try {
      parser.parseFromStream(stream, uri);
    } catch(e) {
      Assert.ok(false, "parse failed for " + data.file.leafName + " ---- " + e.message);
      
      run_next_test();
    } finally {
      stream.close();
    }
  }
}

function run_test() {
  
  var topDir = Services.dirsvc.get("CurWorkD", Ci.nsIFile);
  topDir.append("xml");

  
  
  iterateDir(topDir, true, file => {
    var data = readTestData(file);
    add_test(createTest(data));
  });

  
  run_next_test();
}
