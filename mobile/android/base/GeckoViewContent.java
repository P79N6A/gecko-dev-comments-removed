




package org.mozilla.gecko;

public class GeckoViewContent implements GeckoView.ContentDelegate {
    





    public void onPageStart(GeckoView view, GeckoView.Browser browser, String url) {}

    





    public void onPageStop(GeckoView view, GeckoView.Browser browser, boolean success) {}

    





    public void onPageShow(GeckoView view, GeckoView.Browser browser) {}

    






    public void onReceivedTitle(GeckoView view, GeckoView.Browser browser, String title) {}

    







    public void onReceivedFavicon(GeckoView view, GeckoView.Browser browser, String url, int size) {}
}
