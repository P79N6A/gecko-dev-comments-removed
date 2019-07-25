







































package org.mozilla.gecko;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ProgressBar;

public class BrowserToolbar extends LinearLayout {
    final private ProgressBar mProgressBar;
    final private Button mAwesomeBar;
    final private ImageButton mTabs;
    final private ImageButton mFavicon;

    public BrowserToolbar(Context context, AttributeSet attrs) {
        super(context, attrs);

        
        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.browser_toolbar, this);

        mProgressBar = (ProgressBar) findViewById(R.id.progress_bar);

        mAwesomeBar = (Button) findViewById(R.id.awesome_bar);
        mAwesomeBar.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                onAwesomeBarSearch();
            }
        });

        mTabs = (ImageButton) findViewById(R.id.tabs);
        mTabs.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                if (Tabs.getInstance().getCount() > 1)
                    showTabs();
                else
                    addTab();
            }
        });

        mFavicon = (ImageButton) findViewById(R.id.favicon);
    }

    private void onAwesomeBarSearch() {
        GeckoApp.mAppContext.onEditRequested();
    }

    private void addTab() {
        GeckoApp.mAppContext.addTab();
    }

    private void showTabs() {
        GeckoApp.mAppContext.showTabs();
    }
    
    public void updateTabs(int count) {
        if (count == 1)
            mTabs.setImageResource(R.drawable.tabs_plus);
        else
            mTabs.setImageResource(R.drawable.tabs_menu);
    }

    public void updateProgress(int progress, int total) {
        if (progress < 0 || total < 0) {
            mProgressBar.setIndeterminate(true);
        } else if (progress < total) {
            mProgressBar.setIndeterminate(false);
            mProgressBar.setMax(total);
            mProgressBar.setProgress(progress);
        } else {
            mProgressBar.setIndeterminate(false);
        }
    }

    public void setProgressVisibility(boolean visible) {
        mProgressBar.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    public void setTitle(CharSequence title) {
        mAwesomeBar.setText(title);
    }

    public void setFavicon(Drawable image) {
        if (image != null)
            mFavicon.setImageDrawable(image);
        else
            mFavicon.setImageResource(R.drawable.favicon);
    }
}
