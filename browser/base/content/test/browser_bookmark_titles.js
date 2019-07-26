





let tests = [
    ['http://example.com/browser/browser/base/content/test/dummy_page.html',
     'Dummy test page'],
    ['data:text/html;charset=utf-8,<title>test data: url</title>',
     'test data: url'],
    ['http://unregistered-domain.example',
     'http://unregistered-domain.example/'],
    ['https://untrusted.example.com/somepage.html',
     'https://untrusted.example.com/somepage.html']
];

function generatorTest() {
    gBrowser.selectedTab = gBrowser.addTab();
    let browser = gBrowser.selectedBrowser;

    browser.addEventListener("DOMContentLoaded", nextStep, true);
    registerCleanupFunction(function () {
        browser.removeEventListener("DOMContentLoaded", nextStep, true);
        gBrowser.removeCurrentTab();
    });

    yield; 

    
    for (let i = 0; i < tests.length; ++i) {
        let [uri, title] = tests[i];
        content.location = uri;
        yield;
        checkBookmark(uri, title);
    }

    
    

    
    
    BrowserOffline.toggleOfflineStatus();
    let proxy = Services.prefs.getIntPref('network.proxy.type');
    Services.prefs.setIntPref('network.proxy.type', 0);
    registerCleanupFunction(function () {
        BrowserOffline.toggleOfflineStatus();
        Services.prefs.setIntPref('network.proxy.type', proxy);
    });

    
    Services.cache.evictEntries(Services.cache.STORE_ANYWHERE);

    let [uri, title] = tests[0];
    content.location = uri;
    yield;
    
    is(content.document.documentURI.substring(0, 14), 'about:neterror',
        "Offline mode successfully simulated network outage.");
    checkBookmark(uri, title);
}



function checkBookmark(uri, expected_title) {
    PlacesCommandHook.bookmarkCurrentPage(false);
    
    let id = PlacesUtils.getMostRecentBookmarkForURI(PlacesUtils._uri(uri));
    let title = PlacesUtils.bookmarks.getItemTitle(id);

    is(title, expected_title, "Bookmark got a good default title.");

    PlacesUtils.bookmarks.removeItem(id);
}

