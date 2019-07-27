



package org.mozilla.search;

import android.graphics.Rect;




public interface AcceptsSearchQuery {

    



    void onSuggest(String query);

    




    void onSearch(String query);

    





    void onSearch(String query, SuggestionAnimation suggestionAnimation);

    




    void onQueryChange(String query);

    


    public interface SuggestionAnimation {
        public Rect getStartBounds();
    }
}
