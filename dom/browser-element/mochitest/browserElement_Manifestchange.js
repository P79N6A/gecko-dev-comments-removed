



"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function createHtml(manifest) {
  return 'data:text/html,<html xmlns:xml="http://www.w3.org/XML/1998/namespace"><head>' + manifest + '<body></body></html>';
}

function createManifest(href) {
  return '<link rel="manifest" href="' + href + '">';
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

  var numManifestChanges = 0;

  iframe1.addEventListener('mozbrowsermanifestchange', function(e) {

    numManifestChanges++;

    if (numManifestChanges == 1) {
      is(e.detail.href, 'manifest.1', 'manifest.1 matches');

      
      
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.title='New title';",
                                     false);

      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=manifest href=manifest.2>')",
                                     false);

      SpecialPowers.getBrowserFrameMessageManager(iframe2)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<link rel=manifest href=manifest.2>')",
                                     false);
    }
    else if (numManifestChanges == 2) {
      is(e.detail.href, 'manifest.2', 'manifest.2 matches');

      
      iframe1.src = createHtml(createManifest('manifest.3'));
    }
    else if (numManifestChanges == 3) {
      is(e.detail.href, 'manifest.3', 'manifest.3 matches');

      
      iframe1.src = createHtml(createManifest('manifest.4a') + createManifest('manifest.4b'));
    }
    else if (numManifestChanges == 4) {
      is(e.detail.href, 'manifest.4a', 'manifest.4a matches');
      
    }
    else if (numManifestChanges == 5) {
      is(e.detail.href, 'manifest.4b', 'manifest.4b matches');
      SimpleTest.finish();
    } else {
      ok(false, 'Too many manifestchange events.');
    }
  });

  iframe3.addEventListener('mozbrowsermanifestchange', function(e) {
    ok(false, 'Should not get a manifestchange event for iframe3.');
  });


  iframe1.src = createHtml(createManifest('manifest.1'));
  
  iframe2.src = createHtml(createManifest('manifest.1'));
  iframe3.src = createHtml(createManifest('manifest.1'));

}

addEventListener('testready', runTest);

