


var testPath = "http://mochi.test:8888/browser/content/html/document/test/";
var ctx = {loadsDone : 0};


function test() {

  waitForExplicitFinish();

  ctx.tab1 = gBrowser.addTab(testPath + "bug592641_img.jpg");
  ctx.tab1Browser = gBrowser.getBrowserForTab(ctx.tab1);
  ctx.tab1Browser.addEventListener("load",
                                   
                                   
                                   
                                   
                                   function() {executeSoon(load1Done);},
                                   true);
}

function checkTitle(title) {

  ctx.loadsDone++;
  ok(/^bug592641_img\.jpg \(JPEG Image, 1500x1500 pixels\)/.test(title),
     "Title should be correct on load #" + ctx.loadsDone);
}


function load1Done() {

  
  var title = ctx.tab1Browser.contentWindow.document.title;
  checkTitle(title);

  
  
  ctx.tab2 = gBrowser.addTab(testPath + "bug592641_img.jpg");
  ctx.tab2Browser = gBrowser.getBrowserForTab(ctx.tab2);
  ctx.tab2Browser.addEventListener("load",
                                   
                                   
                                   
                                   
                                   function() {executeSoon(load2Done);},
                                   true);


}

function load2Done() {

  
  var title = ctx.tab2Browser.contentWindow.document.title;
  checkTitle(title);

  
  gBrowser.removeTab(ctx.tab1);
  gBrowser.removeTab(ctx.tab2);

  
  finish();
}
