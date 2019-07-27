



package org.mozilla.search;

import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.webkit.WebViewClient;

public class PostSearchFragment extends Fragment {

    private static final String LOGTAG = "PostSearchFragment";
    private WebView webview;

    public PostSearchFragment() {}


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View mainView = inflater.inflate(R.layout.search_activity_detail, container, false);


        webview = (WebView) mainView.findViewById(R.id.webview);
        webview.setWebViewClient(new WebViewClient(){
            @Override
            public void onPageStarted(WebView view, String url, Bitmap favicon) {
                if (isSearchResultsPage(url)) {
                    super.onPageStarted(view, url, favicon);
                } else {
                    webview.stopLoading();
                    Intent i = new Intent(Intent.ACTION_VIEW);
                    i.setData(Uri.parse(url));
                    startActivity(i);
                }
            }
        });
        return mainView;
    }

    








    protected boolean isSearchResultsPage(String url) {
        return url.contains(Constants.YAHOO_WEB_SEARCH_RESULTS_FILTER);
    }

    public void startSearch(String query) {
        setUrl(Constants.YAHOO_WEB_SEARCH_BASE_URL + Uri.encode(query));
    }

    public void setUrl(String url) {
        webview.loadUrl(url);
    }
}
