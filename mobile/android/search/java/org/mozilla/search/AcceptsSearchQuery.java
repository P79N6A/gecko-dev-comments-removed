



package org.mozilla.search;

import android.graphics.Rect;




public interface AcceptsSearchQuery {
    




    void onSearch(String query);

    





    void onSearch(String query, SuggestionAnimation suggestionAnimation);

    


    public interface SuggestionAnimation {
        public Rect getStartBounds();
        public void onAnimationEnd();
    }
}
