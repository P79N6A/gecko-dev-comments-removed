







































package org.mozilla.gecko;

import android.content.Context;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.Color;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;

public class BrowserToolbar extends LinearLayout {
    final private Button mAwesomeBar;
    final private ImageButton mTabs;
    final private ImageButton mFavicon;
    final private AnimationDrawable mProgressSpinner;

    public BrowserToolbar(Context context, AttributeSet attrs) {
        super(context, attrs);

        
        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.browser_toolbar, this);

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
        mProgressSpinner = (AnimationDrawable) context.getResources().getDrawable(R.drawable.progress_spinner);
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

    public void setProgressVisibility(boolean visible) {
        if (visible) {
            mFavicon.setImageDrawable(mProgressSpinner);
            mProgressSpinner.start();
        } else {
            mProgressSpinner.stop();
            setFavicon(Tabs.getInstance().getSelectedTab().getFavicon());
        }
    }

    public void setTitle(CharSequence title) {
        mAwesomeBar.setText(title);
    }

    public void setFavicon(Drawable image) {
        if (Tabs.getInstance().getSelectedTab().isLoading())
            return;

        if (image != null)
            mFavicon.setImageDrawable(image);
        else
            mFavicon.setImageResource(R.drawable.favicon);
    }
    
    public void setSecurityMode(String mode) {
        if (mode.equals("identified"))
            mFavicon.setBackgroundColor(Color.rgb(137, 215, 21));
        else if (mode.equals("verified"))
            mFavicon.setBackgroundColor(Color.rgb(101, 121, 227));
        else
            mFavicon.setBackgroundColor(Color.TRANSPARENT);
    }
}
