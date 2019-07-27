



"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function createHtml(meta) {
  return 'data:text/html,<html xmlns:xml="http://www.w3.org/XML/1998/namespace"><head>' + meta + '<body></body></html>';
}

function createHtmlWithLang(meta, lang) {
  return 'data:text/html,<html xmlns:xml="http://www.w3.org/XML/1998/namespace" lang="' + lang + '"><head>' + meta + '<body></body></html>';
}

function createMeta(name, content) {
  return '<meta name="' + name + '" content="' + content + '">';
}

function createMetaWithLang(name, content, lang) {
  return '<meta name="' + name + '" content="' + content + '" lang="' + lang + '">';
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

  var numMetaChanges = 0;

  iframe1.addEventListener('mozbrowsermetachange', function(e) {

    numMetaChanges++;

    if (numMetaChanges == 1) {
      is(e.detail.name, 'application-name');
      is(e.detail.content, 'foobar');

      
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.title='New title';",
                                     false);

      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<meta name=application-name content=new_foobar>')",
                                     false);

      SpecialPowers.getBrowserFrameMessageManager(iframe2)
                   .loadFrameScript("data:,content.document.head.insertAdjacentHTML('beforeend', '<meta name=application-name content=new_foobar>')",
                                     false);
    }
    else if (numMetaChanges == 2) {
      is(e.detail.name, 'application-name', 'name matches');
      is(e.detail.content, 'new_foobar', 'content matches');
      ok(!("lang" in e.detail), 'lang not present');

      
      iframe1.src = createHtml(createMeta('application-name', '3rd_foobar'));
    }
    else if (numMetaChanges == 3) {
      is(e.detail.name, 'application-name', 'name matches');
      is(e.detail.content, '3rd_foobar', 'content matches');
      ok(!("lang" in e.detail), 'lang not present');

      
      iframe1.src = createHtml(createMeta('application-name', 'foobar_1') + createMeta('application-name', 'foobar_2'));
    }
    else if (numMetaChanges == 4) {
      is(e.detail.name, 'application-name', 'name matches');
      is(e.detail.content, 'foobar_1', 'content matches');
      ok(!("lang" in e.detail), 'lang not present');
      
    }
    else if (numMetaChanges == 5) {
      is(e.detail.name, 'application-name', 'name matches');
      is(e.detail.content, 'foobar_2', 'content matches');
      ok(!("lang" in e.detail), 'lang not present');

      
      iframe1.src = createHtml(createMetaWithLang('application-name', 'foobar_lang_1', 'en'));
    }
    else if (numMetaChanges == 6) {
      is(e.detail.name, 'application-name', 'name matches');
      is(e.detail.content, 'foobar_lang_1', 'content matches');
      is(e.detail.lang, 'en', 'language matches');

      
      iframe1.src = createHtmlWithLang(createMeta('application-name', 'foobar_lang_2'), 'es');
    }
    else if (numMetaChanges == 7) {
      is(e.detail.name, 'application-name', 'name matches');
      is(e.detail.content, 'foobar_lang_2', 'content matches');
      is(e.detail.lang, 'es', 'language matches');

      
      iframe1.src = createHtmlWithLang(createMetaWithLang('application-name', 'foobar_lang_3', 'it'), 'fi');
    }
    else if (numMetaChanges == 8) {
      is(e.detail.name, 'application-name', 'name matches');
      is(e.detail.content, 'foobar_lang_3', 'content matches');
      is(e.detail.lang, 'it', 'language matches');

      
      iframe1.src = "http://test/tests/dom/browser-element/mochitest/file_browserElement_Metachange.sjs?ru";
    }
    else if (numMetaChanges == 9) {
      is(e.detail.name, 'application-name', 'name matches');
      is(e.detail.content, 'sjs', 'content matches');
      is(e.detail.lang, 'ru', 'language matches');

      
      iframe1.src = "http://test/tests/dom/browser-element/mochitest/file_browserElement_Metachange.sjs?ru|dk";
    }
    else if (numMetaChanges == 10) {
      is(e.detail.name, 'application-name', 'name matches');
      is(e.detail.content, 'sjs', 'content matches');
      is(e.detail.lang, 'dk', 'language matches');

      
      SimpleTest.finish();
    } else {
      ok(false, 'Too many metachange events.');
    }
  });

  iframe3.addEventListener('mozbrowsermetachange', function(e) {
    ok(false, 'Should not get a metachange event for iframe3.');
  });


  iframe1.src = createHtml(createMeta('application-name', 'foobar'));
  
  iframe2.src = createHtml(createMeta('application-name', 'foobar'));
  iframe3.src = createHtml(createMeta('application-name', 'foobar'));

}

addEventListener('testready', runTest);
