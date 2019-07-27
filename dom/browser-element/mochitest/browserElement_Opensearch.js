



"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function createHtml(link) {
  return 'data:text/html,<html><head>' + link + '<body></body></html>';
}

function createLink(name) {
  return '<link rel="search" title="Test OpenSearch" type="application/opensearchdescription+xml" href="http://example.com/' + name + '.xml">';
}

function runTest() {
  var iframe1 = document.createElement('iframe');
  iframe1.setAttribute('mozbrowser', 'true');
  document.body.appendChild(iframe1);

  
  
  
  var iframe2 = document.createElement('iframe');
  iframe2.setAttribute('mozbrowser', 'true');
  document.body.appendChild(iframe2);

  
  
  var iframe3 = document.createElement('iframe');
  document.body.appendChild(iframe3);

  var numLinkChanges = 0;

  iframe1.addEventListener('mozbrowseropensearch', function(e) {

    numLinkChanges++;

    if (numLinkChanges == 1) {
      is(e.detail.title, 'Test OpenSearch');
      is(e.detail.href, 'http://example.com/mysearch.xml');

      
      
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.title='New title';",
                                     false);

      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=SEARCH type=application/opensearchdescription+xml href=http://example.com/newsearch.xml>')",
                                     false);

      SpecialPowers.getBrowserFrameMessageManager(iframe2)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=SEARCH type=application/opensearchdescription+xml href=http://example.com/newsearch.xml>')",
                                     false);
    }
    else if (numLinkChanges == 2) {
      is(e.detail.href, 'http://example.com/newsearch.xml');

      
      iframe1.src = createHtml(createLink('3rdsearch'));
    }
    else if (numLinkChanges == 3) {
      is(e.detail.href, 'http://example.com/3rdsearch.xml');

      
      
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=someopensearch type=application/opensearchdescription+xml href=http://example.com/newsearch.xml>')",
                                     false);
      
      iframe1.src = createHtml(createLink('another') + createLink('search'));
    }
    else if (numLinkChanges == 4) {
      is(e.detail.href, 'http://example.com/another.xml');
      
    }
    else if (numLinkChanges == 5) {
      is(e.detail.href, 'http://example.com/search.xml');

      
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=SEARCH type=application/opensearchdescription+xml href=http://example.com/ucasesearch.xml>')",
                                     false);
    }
    else if (numLinkChanges == 6) {
      is(e.detail.href, 'http://example.com/ucasesearch.xml');
      SimpleTest.finish();
    } else {
      ok(false, 'Too many opensearch events.');
    }
  });

  iframe3.addEventListener('mozbrowseropensearch', function(e) {
    ok(false, 'Should not get a opensearch event for iframe3.');
  });


  iframe1.src = createHtml(createLink('mysearch'));
  
  iframe2.src = createHtml(createLink('mysearch'));
  iframe3.src = createHtml(createLink('mysearch'));

}

addEventListener('testready', runTest);
