



package org.mozilla.search;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.mozilla.gecko.GeckoView;
import org.mozilla.gecko.GeckoViewChrome;
import org.mozilla.gecko.GeckoViewContent;
import org.mozilla.gecko.PrefsHelper;

public class PostSearchFragment extends Fragment {

    private static final String LOGTAG = "PostSearchFragment";
    private GeckoView geckoView;


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View mainView = inflater.inflate(R.layout.search_activity_detail, container, false);


        geckoView = (GeckoView) mainView.findViewById(R.id.gecko_view);

        geckoView.setChromeDelegate(new MyGeckoViewChrome());
        geckoView.setContentDelegate(new SearchGeckoView());

        PrefsHelper.setPref("privacy.clearOnShutdown.cache", true);
        PrefsHelper.setPref("privacy.clearOnShutdown.cookies", true);


        if (null == geckoView.getCurrentBrowser()) {
            
            
            geckoView.addBrowser("https://search.yahoo.com/search?p=firefox%20android");

        }

        return mainView;
    }


    public void setUrl(String url) {
        if (null == geckoView.getCurrentBrowser()) {
            geckoView.addBrowser(url);
        } else {
            geckoView.getCurrentBrowser().loadUrl(url);
        }
    }


    private static class MyGeckoViewChrome extends GeckoViewChrome {
        @Override
        public void onReady(GeckoView view) {
            Log.i(LOGTAG, "Gecko is ready");

            PrefsHelper.setPref("devtools.debugger.remote-enabled", true);

            
            
        }

    }


    private class SearchGeckoView extends GeckoViewContent {

        @Override
        public void onPageStart(GeckoView geckoView, GeckoView.Browser browser, String s) {
            Log.i("OnPageStart", s);
            
            
            if (s.contains("//search.yahoo.com")) {
                super.onPageStart(geckoView, browser, s);


            } else {
                browser.stop();
                Intent i = new Intent(Intent.ACTION_VIEW);
                i.setData(Uri.parse(s));
                startActivity(i);
            }
        }

        @Override
        public void onPageShow(GeckoView geckoView, GeckoView.Browser browser) {

        }
    }
}
