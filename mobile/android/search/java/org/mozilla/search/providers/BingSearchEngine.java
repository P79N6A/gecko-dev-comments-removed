



package org.mozilla.search.providers;

import android.net.Uri;

public class BingSearchEngine extends SearchEngine {

    private static final String HIDE_BANNER_CSS = "#mHeader{display:none}#contentWrapper{margin-top:0}";

    private static final Uri RESULTS_URI = Uri.parse("https://www.bing.com/search");
    private static final String RESULTS_URI_QUERY_PARAM = "q";

    private static final Uri SUGGEST_URI = Uri.parse("http://api.bing.com/osjson.aspx");
    private static final String SUGGEST_URI_QUERY_PARAM = "query";

    @Override
    public String getInjectableCss() {
        return HIDE_BANNER_CSS;
    }

    @Override
    protected Uri getResultsUri() {
        return RESULTS_URI;
    }

    @Override
    protected Uri getSuggestionUri() {
        return SUGGEST_URI;
    }

    @Override
    protected String getSuggestionQueryParam() {
        return SUGGEST_URI_QUERY_PARAM;
    }

    @Override
    protected String getResultsQueryParam() {
        return RESULTS_URI_QUERY_PARAM;
    }
}
