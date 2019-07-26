




package org.mozilla.gecko;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.MenuItem;
import android.widget.TextView;
import android.widget.RelativeLayout;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.view.animation.AnimationUtils;
import android.view.animation.Animation;
import android.widget.ImageView;
import android.view.Display;

import java.io.File;
import java.net.URI;

public class WebappImpl extends GeckoApp {
    private static final String LOGTAG = "GeckoWebappImpl";

    private URI mOrigin;
    private TextView mTitlebarText = null;
    private View mTitlebar = null;

    private View mSplashscreen;

    protected int getIndex() { return 0; }

    @Override
    public int getLayout() { return R.layout.web_app; }

    @Override
    public boolean hasTabsSideBar() { return false; }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        mSplashscreen = (RelativeLayout) findViewById(R.id.splashscreen);
        if (!GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            overridePendingTransition(R.anim.grow_fade_in_center, android.R.anim.fade_out);
            showSplash();
        }

        String action = getIntent().getAction();
        Bundle extras = getIntent().getExtras();
        String title = extras != null ? extras.getString(Intent.EXTRA_SHORTCUT_NAME) : null;
        setTitle(title != null ? title : "Web App");

        mTitlebarText = (TextView)findViewById(R.id.webapp_title);
        mTitlebar = findViewById(R.id.webapp_titlebar);
        if (!action.startsWith(ACTION_WEBAPP_PREFIX)) {
            Log.e(LOGTAG, "Webapp launch, but intent action is " + action + "!");
            return;
        }

        
        String origin = WebappAllocator.getInstance(this).getAppForIndex(getIndex());
        try {
            mOrigin = new URI(origin);
        } catch (java.net.URISyntaxException ex) {
            
            if (!origin.startsWith("app://")) {
                return;
            }

            
            Log.i(LOGTAG, "Webapp is not registered with allocator");
            try {
                mOrigin = new URI(getIntent().getData().toString());
            } catch (java.net.URISyntaxException ex2) {
                Log.e(LOGTAG, "Unable to parse intent url: ", ex);
            }
        }
    }

    @Override
    protected void loadStartupTab(String uri) {
        String action = getIntent().getAction();
        if (GeckoApp.ACTION_WEBAPP_PREFIX.equals(action)) {
            
            
            
            int index = WebappAllocator.getInstance(this).findAndAllocateIndex(uri, "App", (Bitmap) null);
            Intent appIntent = GeckoAppShell.getWebappIntent(index, uri);
            startActivity(appIntent);
            finish();
        }
    }

    private void showSplash() {
        SharedPreferences prefs = getSharedPreferences("webapps", Context.MODE_PRIVATE | Context.MODE_MULTI_PROCESS);

        
        int[] colors = new int[2];
        int dominantColor = prefs.getInt(WebappAllocator.iconKey(getIndex()), -1);

        
        float[] f = new float[3];
        Color.colorToHSV(dominantColor, f);
        f[2] = Math.min(f[2]*2, 1.0f);
        colors[0] = Color.HSVToColor(255, f);

        
        f[2] *= 0.75;
        colors[1] = Color.HSVToColor(255, f);

        
        GradientDrawable gd = new GradientDrawable(GradientDrawable.Orientation.TL_BR, colors);
        gd.setGradientType(GradientDrawable.RADIAL_GRADIENT);
        Display display = getWindowManager().getDefaultDisplay();
        gd.setGradientCenter(0.5f, 0.5f);
        gd.setGradientRadius(Math.max(display.getWidth()/2, display.getHeight()/2));
        mSplashscreen.setBackgroundDrawable((Drawable)gd);

        
        File profile = getProfile().getDir();
        File logoFile = new File(profile, "logo.png");
        if (logoFile.exists()) {
            ImageView image = (ImageView)findViewById(R.id.splashscreen_icon);
            Drawable d = Drawable.createFromPath(logoFile.getPath());
            image.setImageDrawable(d);

            Animation fadein = AnimationUtils.loadAnimation(this, R.anim.grow_fade_in_center);
            fadein.setStartOffset(500);
            fadein.setDuration(1000);
            image.startAnimation(fadein);
        }
    }

    @Override
    protected String getDefaultProfileName() {
        String action = getIntent().getAction();
        if (!action.startsWith(ACTION_WEBAPP_PREFIX)) {
            Log.e(LOGTAG, "Webapp launch, but intent action is " + action + "!");
            return null;
        }

        return "webapp" + action.substring(ACTION_WEBAPP_PREFIX.length());
    }

    @Override
    protected boolean getSessionRestoreState(Bundle savedInstanceState) {
        
        return false;
    }

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch(msg) {
            case SELECTED:
            case LOCATION_CHANGE:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    final String urlString = tab.getURL();
                    final URI uri;

                    try {
                        uri = new URI(urlString);
                    } catch (java.net.URISyntaxException ex) {
                        mTitlebarText.setText(urlString);

                        
                        
                        
                        if (!urlString.startsWith("app://")) {
                            mTitlebar.setVisibility(View.VISIBLE);
                        } else {
                            mTitlebar.setVisibility(View.GONE);
                        }
                        return;
                    }

                    if (mOrigin != null && mOrigin.getHost().equals(uri.getHost())) {
                        mTitlebar.setVisibility(View.GONE);
                    } else {
                        mTitlebarText.setText(uri.getScheme() + "://" + uri.getHost());
                        mTitlebar.setVisibility(View.VISIBLE);
                    }
                }
                break;
            case LOADED:
                if (mSplashscreen != null && mSplashscreen.getVisibility() == View.VISIBLE) {
                    Animation fadeout = AnimationUtils.loadAnimation(this, android.R.anim.fade_out);
                    fadeout.setAnimationListener(new Animation.AnimationListener() {
                        @Override
                        public void onAnimationEnd(Animation animation) {
                          mSplashscreen.setVisibility(View.GONE);
                        }
                        @Override
                        public void onAnimationRepeat(Animation animation) { }
                        @Override
                        public void onAnimationStart(Animation animation) { }
                    });
                    mSplashscreen.startAnimation(fadeout);
                }
                break;
            case START:
                if (mSplashscreen != null && mSplashscreen.getVisibility() == View.VISIBLE) {
                    View area = findViewById(R.id.splashscreen_progress);
                    area.setVisibility(View.VISIBLE);
                    Animation fadein = AnimationUtils.loadAnimation(this, android.R.anim.fade_in);
                    fadein.setDuration(1000);
                    area.startAnimation(fadein);
                }
                break;
        }
        super.onTabChanged(tab, msg, data);
    }
};
