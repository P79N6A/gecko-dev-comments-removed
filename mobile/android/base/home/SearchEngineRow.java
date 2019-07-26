




package org.mozilla.gecko.home;

import org.mozilla.gecko.AnimatedHeightLayout;
import org.mozilla.gecko.FlowLayout;
import org.mozilla.gecko.R;
import org.mozilla.gecko.home.BrowserSearch.OnEditSuggestionListener;
import org.mozilla.gecko.home.BrowserSearch.OnSearchListener;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.widget.FaviconView;

import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.EnumSet;

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

    
    private OnSearchListener mSearchListener;

    
    private OnEditSuggestionListener mEditSuggestionListener;

    
    private int mSelectedView = 0;

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

                
                
                
                if (v != mUserEnteredView && !StringUtils.isSearchQuery(suggestion, false)) {
                    if (mUrlOpenListener != null) {
                        mUrlOpenListener.onUrlOpen(suggestion, EnumSet.noneOf(OnUrlOpenListener.Flags.class));
                    }
                } else if (mSearchListener != null) {
                    mSearchListener.onSearch(mSearchEngine.name, suggestion);
                }
            }
        };

        mLongClickListener = new OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                if (mEditSuggestionListener != null) {
                    final String suggestion = getSuggestionTextFromView(v);
                    mEditSuggestionListener.onEditSuggestion(suggestion);
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

    private void setDescriptionOnSuggestion(View v, String suggestion) {
        v.setContentDescription(getResources().getString(R.string.suggestion_for_engine,
                                                         mSearchEngine.name, suggestion));
    }

    private String getSuggestionTextFromView(View v) {
        final TextView suggestionText = (TextView) v.findViewById(R.id.suggestion_text);
        return suggestionText.getText().toString();
    }

    private void setSuggestionOnView(View v, String suggestion) {
        final TextView suggestionText = (TextView) v.findViewById(R.id.suggestion_text);
        suggestionText.setText(suggestion);
        setDescriptionOnSuggestion(suggestionText, suggestion);
    }

    


    public void performUserEnteredSearch() {
        String searchTerm = getSuggestionTextFromView(mUserEnteredView);
        if (mSearchListener != null) {
            mSearchListener.onSearch(mSearchEngine.name, searchTerm);
        }
    }

    public void setSearchTerm(String searchTerm) {
        mUserEnteredTextView.setText(searchTerm);

        
        
        if (mSearchEngine != null) {
            setDescriptionOnSuggestion(mUserEnteredTextView, searchTerm);
        }
    }

    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }

    public void setOnSearchListener(OnSearchListener listener) {
        mSearchListener = listener;
    }

    public void setOnEditSuggestionListener(OnEditSuggestionListener listener) {
        mEditSuggestionListener = listener;
    }

    public void updateFromSearchEngine(SearchEngine searchEngine, boolean animate) {
        
        mSearchEngine = searchEngine;

        
        mIconView.updateAndScaleImage(mSearchEngine.icon, mSearchEngine.name);

        
        setDescriptionOnSuggestion(mUserEnteredTextView, mUserEnteredTextView.getText().toString());

        
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

            if (animate) {
                AlphaAnimation anim = new AlphaAnimation(0, 1);
                anim.setDuration(ANIMATION_DURATION);
                anim.setStartOffset(i * ANIMATION_DURATION);
                suggestionItem.startAnimation(anim);
            }
        }

        
        for (int i = suggestionCount + 1; i < recycledSuggestionCount; i++) {
            mSuggestionView.getChildAt(i).setVisibility(View.GONE);
        }

        
        if (mSelectedView >= mSuggestionView.getChildCount()) {
            mSelectedView = mSuggestionView.getChildCount() - 1;
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, android.view.KeyEvent event) {
        final View suggestion = mSuggestionView.getChildAt(mSelectedView);

        if (event.getAction() != android.view.KeyEvent.ACTION_DOWN) {
            return false;
        }

        switch (event.getKeyCode()) {
        case KeyEvent.KEYCODE_DPAD_RIGHT:
            final View nextSuggestion = mSuggestionView.getChildAt(mSelectedView + 1);
            if (nextSuggestion != null) {
                changeSelectedSuggestion(suggestion, nextSuggestion);
                mSelectedView++;
                return true;
            }
            break;

        case KeyEvent.KEYCODE_DPAD_LEFT:
            final View prevSuggestion = mSuggestionView.getChildAt(mSelectedView - 1);
            if (prevSuggestion != null) {
                changeSelectedSuggestion(suggestion, prevSuggestion);
                mSelectedView--;
                return true;
            }
            break;

        case KeyEvent.KEYCODE_BUTTON_A:
            
            return suggestion.performClick();
        }

        return false;
    }

    private void changeSelectedSuggestion(View oldSuggestion, View newSuggestion) {
        oldSuggestion.setDuplicateParentStateEnabled(false);
        newSuggestion.setDuplicateParentStateEnabled(true);
        oldSuggestion.refreshDrawableState();
        newSuggestion.refreshDrawableState();
    }

    public void onSelected() {
        mSelectedView = 0;
        mUserEnteredView.setDuplicateParentStateEnabled(true);
        mUserEnteredView.refreshDrawableState();
    }

    public void onDeselected() {
        final View suggestion = mSuggestionView.getChildAt(mSelectedView);
        suggestion.setDuplicateParentStateEnabled(false);
        suggestion.refreshDrawableState();
    }
}
