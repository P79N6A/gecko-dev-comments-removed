



"use strict";

SimpleTest.waitForExplicitFinish();

function createHtml(link) {
  return 'data:text/html,<html><head>' + link + '<body></body></html>';
}

function createLink(name) {
  return '<link rel="icon" type="image/png" href="http://example.com/' + name + '.png">';
}

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe1 = document.createElement('iframe');
  SpecialPowers.wrap(iframe1).mozbrowser = true;
  document.body.appendChild(iframe1);

  
  
  
  var iframe2 = document.createElement('iframe');
  SpecialPowers.wrap(iframe2).mozbrowser = true;
  document.body.appendChild(iframe2);

  
  
  var iframe3 = document.createElement('iframe');
  document.body.appendChild(iframe3);

  var numIconChanges = 0;

  iframe1.addEventListener('mozbrowsericonchange', function(e) {

    numIconChanges++;

    if (numIconChanges == 1) {
      is(e.detail, 'http://example.com/myicon.png');

      
      
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.title='New title';",
                                     false);

      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=ICON href=http://example.com/newicon.png>')",
                                     false);

      SpecialPowers.getBrowserFrameMessageManager(iframe2)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=ICON href=http://example.com/newicon.png>')",
                                     false);
    }
    else if (numIconChanges == 2) {
      is(e.detail, 'http://example.com/newicon.png');

      
      iframe1.src = createHtml(createLink('3rdicon'));
    }
    else if (numIconChanges == 3) {
      is(e.detail, 'http://example.com/3rdicon.png');

      
      
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=shortcuticon href=http://example.com/newicon.png>')",
                                     false);
      
      iframe1.src = createHtml(createLink('another') + createLink('icon'));
    }
    else if (numIconChanges == 4) {
      is(e.detail, 'http://example.com/another.png');
      
    }
    else if (numIconChanges == 5) {
      is(e.detail, 'http://example.com/icon.png');

      
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=ICON href=http://example.com/ucaseicon.png>')",
                                     false);
    }
    else if (numIconChanges == 6) {
      is(e.detail, 'http://example.com/ucaseicon.png');
      SimpleTest.finish();
    } else {
      ok(false, 'Too many iconchange events.');
    }
  });

  iframe3.addEventListener('mozbrowsericonchange', function(e) {
    ok(false, 'Should not get a iconchange event for iframe3.');
  });


  iframe1.src = createHtml(createLink('myicon'));
  
  iframe2.src = createHtml(createLink('myicon'));
  iframe3.src = createHtml(createLink('myicon'));

}

addEventListener('load', function() { SimpleTest.executeSoon(runTest); });
