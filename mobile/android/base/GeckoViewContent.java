




package org.mozilla.gecko;

public class GeckoViewContent implements GeckoView.ContentDelegate {
    





    @Override
    public void onPageStart(GeckoView view, GeckoView.Browser browser, String url) {}

    





    @Override
    public void onPageStop(GeckoView view, GeckoView.Browser browser, boolean success) {}

    





    @Override
    public void onPageShow(GeckoView view, GeckoView.Browser browser) {}

    






    @Override
    public void onReceivedTitle(GeckoView view, GeckoView.Browser browser, String title) {}

    







    @Override
    public void onReceivedFavicon(GeckoView view, GeckoView.Browser browser, String url, int size) {}
}
