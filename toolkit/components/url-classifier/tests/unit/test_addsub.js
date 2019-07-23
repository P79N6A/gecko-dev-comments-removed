
function doTest(updates, assertions)
{
  doUpdateTest(updates, assertions, runNextTest, updateError);
}


function testSimpleAdds() {
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : addUrls
  };

  doTest([update], assertions);
}



function testMultipleAdds() {
  var add1Urls = [ "foo.com/a", "bar.com/c" ];
  var add2Urls = [ "foo.com/b" ];

  var update = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "urls" : add1Urls },
      { "chunkNum" : 2,
        "urls" : add2Urls }]);
  var assertions = {
    "tableData" : "test-phish-simple;a:1-2",
    "urlsExist" : add1Urls.concat(add2Urls)
  };

  doTest([update], assertions);
}


function testSimpleSub()
{
  var addUrls = ["foo.com/a", "bar.com/b"];
  var subUrls = ["1:foo.com/a"];

  var addUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1, 
       "urls": addUrls }]);

  var subUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 50,
       "chunkType" : "s",
       "urls": subUrls }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1:s:50",
    "urlsExist" : [ "bar.com/b" ],
    "urlsDontExist": ["foo.com/a" ],
    "subsDontExist" : [ "foo.com/a" ]
  }

  doTest([addUpdate, subUpdate], assertions);

}


function testSubEmptiesAdd()
{
  var subUrls = ["1:foo.com/a"];
  var addUrls = ["foo.com/a", "bar.com/b"];

  var subUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 50,
       "chunkType" : "s",
       "urls": subUrls }]);

  var addUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "urls": addUrls }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1:s:50",
    "urlsExist" : [ "bar.com/b" ],
    "urlsDontExist": ["foo.com/a" ],
    "subsDontExist" : [ "foo.com/a" ] 
  }

  doTest([subUpdate, addUpdate], assertions);
}



function testSubPartiallyEmptiesAdd()
{
  var subUrls = ["1:foo.com/a"];
  var addUrls = ["foo.com/a", "foo.com/b", "bar.com/b"];

  var subUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "chunkType" : "s",
       "urls": subUrls }]);

  var addUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1, 
       "urls": addUrls }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1:s:1",
    "urlsExist" : [ "foo.com/b", "bar.com/b" ],
    "urlsDontExist" : ["foo.com/a" ],
    "subsDontExist" : [ "foo.com/a" ] 
  }

  doTest([subUpdate, addUpdate], assertions);
}





function testPendingSubRemoved()
{
  var subUrls = ["1:foo.com/a", "2:foo.com/b"];
  var addUrls = ["foo.com/a", "foo.com/b"];

  var subUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "chunkType" : "s",
       "urls": subUrls }]);

  var addUpdate1 = buildPhishingUpdate(
    [{ "chunkNum" : 1, 
       "urls": addUrls }]);

  var addUpdate2 = buildPhishingUpdate(
    [{ "chunkNum" : 2,
       "urls": addUrls }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1-2:s:1",
    "urlsExist" : [ "foo.com/a", "foo.com/b" ],
    "subsDontExist" : [ "foo.com/a", "foo.com/b" ] 
  }

  doTest([subUpdate, addUpdate1, addUpdate2], assertions);
}


function testPendingSubExpire()
{
  var subUrls = ["1:foo.com/a", "1:foo.com/b"];
  var addUrls = ["foo.com/a", "foo.com/b"];

  var subUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "chunkType" : "s",
       "urls": subUrls }]);

  var expireUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "chunkType" : "sd" }]);

  var addUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1, 
       "urls": addUrls }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : [ "foo.com/a", "foo.com/b" ],
    "subsDontExist" : [ "foo.com/a", "foo.com/b" ] 
  }

  doTest([subUpdate, expireUpdate, addUpdate], assertions);
}


function testDuplicateAdds()
{
  var urls = ["foo.com/a"];

  var addUpdate1 = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "urls": urls }]);
  var addUpdate2 = buildPhishingUpdate(
    [{ "chunkNum" : 2,
       "urls": urls }]);
  var subUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 3,
       "chunkType" : "s",
       "urls": ["2:foo.com/a"]}]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1-2:s:3",
    "urlsExist" : [ "foo.com/a"],
    "subsDontExist" : [ "foo.com/a"]
  }

  doTest([addUpdate1, addUpdate2, subUpdate], assertions);
}


function testSubPartiallyMatches()
{
  var subUrls = ["foo.com/a"];
  var addUrls = ["1:foo.com/a", "2:foo.com/b"];

  var addUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "urls" : addUrls }]);

  var subUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "chunkType" : "s",
       "urls" : addUrls }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1:s:1",
    "urlsDontExist" : ["foo.com/a"],
    "subsDontExist" : ["foo.com/a"],
    "subsExist" : ["foo.com/b"]
  };

  doTest([addUpdate, subUpdate], assertions);
}




function testSubPartiallyMatches2()
{
  var addUrls = ["foo.com/a"];
  var subUrls = ["1:foo.com/a", "2:foo.com/b"];
  var addUrls2 = ["foo.com/b"];

  var addUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "urls" : addUrls }]);

  var subUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "chunkType" : "s",
       "urls" : subUrls }]);

  var addUpdate2 = buildPhishingUpdate(
    [{ "chunkNum" : 2,
       "urls" : addUrls2 }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1-2:s:1",
    "urlsDontExist" : ["foo.com/a", "foo.com/b"],
    "subsDontExist" : ["foo.com/a", "foo.com/b"]
  };

  doTest([addUpdate, subUpdate, addUpdate2], assertions);
}



function testSubsDifferentChunks() {
  var subUrls1 = [ "3:foo.com/a" ];
  var subUrls2 = [ "3:foo.com/b" ];

  var addUrls = [ "foo.com/a", "foo.com/b", "foo.com/c" ];

  var subUpdate1 = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "chunkType" : "s",
       "urls": subUrls1 }]);
  var subUpdate2 = buildPhishingUpdate(
    [{ "chunkNum" : 2,
       "chunkType" : "s",
       "urls" : subUrls2 }]);
  var addUpdate = buildPhishingUpdate(
    [{ "chunkNum" : 3,
       "urls" : addUrls }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:3:s:1-2",
    "urlsExist" : [ "foo.com/c" ],
    "urlsDontExist" : [ "foo.com/a", "foo.com/b" ],
    "subsDontExist" : [ "foo.com/a", "foo.com/b" ]
  };

  doTest([subUpdate1, subUpdate2, addUpdate], assertions);
}


function testExpireLists() {
  var addUpdate = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : [ "foo.com/a" ]
          },
          { "chunkNum" : 3,
            "urls" : [ "bar.com/a" ]
          },
          { "chunkNum" : 4,
            "urls" : [ "baz.com/a" ]
          },
          { "chunkNum" : 5,
            "urls" : [ "blah.com/a" ]
          },
          ]);
  var subUpdate = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "chunkType" : "s",
            "urls" : [ "50:foo.com/1" ]
          },
          { "chunkNum" : 2,
            "chunkType" : "s",
            "urls" : [ "50:bar.com/1" ]
          },
          { "chunkNum" : 3,
            "chunkType" : "s",
            "urls" : [ "50:baz.com/1" ]
          },
          { "chunkNum" : 5,
            "chunkType" : "s",
            "urls" : [ "50:blah.com/1" ]
          },
          ]);

  var expireUpdate = buildPhishingUpdate(
    [ { "chunkType" : "ad:1,3-5" },
      { "chunkType" : "sd:1-3,5" }]);

  var assertions = {
    "tableData" : "test-phish-simple;"
  };

  doTest([addUpdate, subUpdate, expireUpdate], assertions);
}


function testDuplicateAddChunks() {
  var addUrls1 = [ "foo.com/a" ];
  var addUrls2 = [ "bar.com/b" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls1
          },
          { "chunkNum" : 1,
            "urls" : addUrls2
          }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : addUrls1,
    "urlsDontExist" : addUrls2
  };

  doTest([update], assertions);
}






function testExpireWholeSub()
{
  var subUrls = ["1:foo.com/a"];

  var update = buildPhishingUpdate(
        [{ "chunkNum" : 5,
           "chunkType" : "s",
           "urls" : subUrls
          },
          
          { "chunkNum" : 1,
            "urls" : []
          },
          
          
          

          
          {
            "chunkType" : "ad:1"
          },
          { "chunkNum" : 1,
            "urls" : [ "foo.com/a" ]
          }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1:s:5",
    "urlsExist" : ["foo.com/a"]
  };

  doTest([update], assertions);
}





function testPreventWholeSub()
{
  var subUrls = ["1:foo.com/a"];

  var update = buildPhishingUpdate(
        [  
          { "chunkNum" : 1,
            "urls" : []
          },
          { "chunkNum" : 5,
           "chunkType" : "s",
           "urls" : subUrls
          },
          
          
          

          
          {
            "chunkType" : "ad:1"
          },
          { "chunkNum" : 1,
            "urls" : [ "foo.com/a" ]
          }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1:s:5",
    "urlsExist" : ["foo.com/a"]
  };

  doTest([update], assertions);
}

function run_test()
{
  runTests([
    testSimpleAdds,
    testMultipleAdds,
    testSimpleSub,
    testSubEmptiesAdd,
    testSubPartiallyEmptiesAdd,
    testPendingSubRemoved,
    testPendingSubExpire,
    testDuplicateAdds,
    testSubPartiallyMatches,
    testSubPartiallyMatches2,
    testSubsDifferentChunks,
    testExpireLists,
    testDuplicateAddChunks,
    testExpireWholeSub,
    testPreventWholeSub,
  ]);
}

do_test_pending();
