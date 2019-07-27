



"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function createHtml(link) {
  return 'data:text/html,<html><head>' + link + '<body></body></html>';
}

function createLink(name, sizes, rel) {
  var s = sizes ? 'sizes="' + sizes + '"' : '';
  if (!rel) {
    rel = 'icon';
  }
  return '<link rel="' + rel + '" type="image/png" ' + s +
    ' href="http://example.com/' + name + '.png">';
}

function runTest() {
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
      is(e.detail.href, 'http://example.com/myicon.png');

      
      
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
      is(e.detail.href, 'http://example.com/newicon.png');

      
      iframe1.src = createHtml(createLink('3rdicon'));
    }
    else if (numIconChanges == 3) {
      is(e.detail.href, 'http://example.com/3rdicon.png');

      
      
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=shortcuticon href=http://example.com/newicon.png>')",
                                     false);
      
      iframe1.src = createHtml(createLink('another') + createLink('icon'));
    }
    else if (numIconChanges == 4) {
      is(e.detail.href, 'http://example.com/another.png');
      
    }
    else if (numIconChanges == 5) {
      is(e.detail.href, 'http://example.com/icon.png');

      
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=ICON href=http://example.com/ucaseicon.png>')",
                                     false);
    }
    else if (numIconChanges == 6) {
      is(e.detail.href, 'http://example.com/ucaseicon.png');
      iframe1.src = createHtml(createLink('testsize', '50x50', 'icon'));
    }
    else if (numIconChanges == 7) {
      is(e.detail.href, 'http://example.com/testsize.png');
      is(e.detail.sizes, '50x50');
      iframe1.src = createHtml(createLink('testapple1', '100x100', 'apple-touch-icon'));
    } else if (numIconChanges == 8) {
      is(e.detail.href, 'http://example.com/testapple1.png');
      is(e.detail.rel, 'apple-touch-icon');
      is(e.detail.sizes, '100x100');
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

addEventListener('testready', runTest);
