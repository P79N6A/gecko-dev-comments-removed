




package org.mozilla.gecko;

import org.mozilla.gecko.AwesomeBarTabs.OnUrlOpenListener;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.widget.FaviconView;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.animation.AlphaAnimation;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

class SearchEngineRow extends AnimatedHeightLayout {
    
    private static final int ANIMATION_DURATION = 250;

    
    private final FlowLayout mSuggestionView;
    private final FaviconView mIconView;
    private final LinearLayout mUserEnteredView;
    private final TextView mUserEnteredTextView;

    
    private final LayoutInflater mInflater;

    
    private SearchEngine mSearchEngine;

    
    private final OnClickListener mClickListener;
    private final OnLongClickListener mLongClickListener;

    
    private OnUrlOpenListener mUrlOpenListener;

    public SearchEngineRow(Context context) {
        this(context, null);
    }

    public SearchEngineRow(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SearchEngineRow(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        mClickListener = new OnClickListener() {
            @Override
            public void onClick(View v) {
                final String suggestion = getSuggestionTextFromView(v);

                
                
                
                if (mUrlOpenListener != null) {
                    if (v != mUserEnteredView && !StringUtils.isSearchQuery(suggestion, false)) {
                        mUrlOpenListener.onUrlOpen(suggestion, null);
                    } else {
                        mUrlOpenListener.onSearch(mSearchEngine, suggestion);
                    }
                }
            }
        };

        mLongClickListener = new OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                if (mUrlOpenListener != null) {
                    final String suggestion = getSuggestionTextFromView(v);
                    mUrlOpenListener.onEditSuggestion(suggestion);
                    return true;
                }

                return false;
            }
        };

        mInflater = LayoutInflater.from(context);
        mInflater.inflate(R.layout.search_engine_row, this);

        mSuggestionView = (FlowLayout) findViewById(R.id.suggestion_layout);
        mIconView = (FaviconView) findViewById(R.id.suggestion_icon);

        
        mUserEnteredView = (LinearLayout) findViewById(R.id.suggestion_user_entered);
        mUserEnteredView.setOnClickListener(mClickListener);

        mUserEnteredTextView = (TextView) findViewById(R.id.suggestion_text);
    }

    private String getSuggestionTextFromView(View v) {
        final TextView suggestionText = (TextView) v.findViewById(R.id.suggestion_text);
        return suggestionText.getText().toString();
    }

    private void setSuggestionOnView(View v, String suggestion) {
        final TextView suggestionText = (TextView) v.findViewById(R.id.suggestion_text);
        suggestionText.setText(suggestion);
    }

    public void setSearchTerm(String searchTerm) {
        mUserEnteredTextView.setText(searchTerm);
    }

    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }

    public void updateFromSearchEngine(SearchEngine searchEngine, boolean doAnimation) {
        
        mSearchEngine = searchEngine;

        
        mIconView.updateImage(mSearchEngine.icon, mSearchEngine.name);

        
        final int recycledSuggestionCount = mSuggestionView.getChildCount();
        final int suggestionCount = mSearchEngine.suggestions.size();

        for (int i = 0; i < suggestionCount; i++) {
            final View suggestionItem;

            
            if (i + 1 < recycledSuggestionCount) {
                suggestionItem = mSuggestionView.getChildAt(i + 1);
                suggestionItem.setVisibility(View.VISIBLE);
            } else {
                suggestionItem = mInflater.inflate(R.layout.suggestion_item, null);

                suggestionItem.setOnClickListener(mClickListener);
                suggestionItem.setOnLongClickListener(mLongClickListener);

                final ImageView magnifier =
                        (ImageView) suggestionItem.findViewById(R.id.suggestion_magnifier);
                magnifier.setVisibility(View.GONE);

                mSuggestionView.addView(suggestionItem);
            }

            final String suggestion = mSearchEngine.suggestions.get(i);
            setSuggestionOnView(suggestionItem, suggestion);

            if (doAnimation) {
                AlphaAnimation anim = new AlphaAnimation(0, 1);
                anim.setDuration(ANIMATION_DURATION);
                anim.setStartOffset(i * ANIMATION_DURATION);
                suggestionItem.startAnimation(anim);
            }
        }

        
        for (int i = suggestionCount + 1; i < recycledSuggestionCount; i++) {
            mSuggestionView.getChildAt(i).setVisibility(View.GONE);
        }
    }
}
