




package org.mozilla.gecko.webapp;

import java.io.File;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoThread;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.webapp.ApkResources;
import org.mozilla.gecko.webapp.InstallHelper;
import org.mozilla.gecko.webapp.InstallHelper.InstallCallback;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.TextView;

public class WebappImpl extends GeckoApp implements InstallCallback {
    private static final String LOGTAG = "GeckoWebappImpl";

    private URL mOrigin;
    private TextView mTitlebarText = null;
    private View mTitlebar = null;

    private View mSplashscreen;

    private ApkResources mApkResources;

    protected int getIndex() { return 0; }

    @Override
    public int getLayout() { return R.layout.web_app; }

    @Override
    public boolean hasTabsSideBar() { return false; }

    @Override
    public void onCreate(Bundle savedInstance)
    {

        String action = getIntent().getAction();
        Bundle extras = getIntent().getExtras();
        if (extras == null) {
            extras = savedInstance;
        }

        if (extras == null) {
            extras = new Bundle();
        }

        boolean isInstalled = extras.getBoolean("isInstalled", false);
        String packageName = extras.getString("packageName");

        if (packageName == null) {
            
            Log.w(LOGTAG, "Can't find package name for webapp");
            setResult(RESULT_CANCELED);
            finish();
        }

        try {
            mApkResources = new ApkResources(this, packageName);
        } catch (NameNotFoundException e) {
            Log.e(LOGTAG, "Can't find package for webapp " + packageName, e);
            setResult(RESULT_CANCELED);
            finish();
        }

        
        super.onCreate(savedInstance);

        mTitlebarText = (TextView)findViewById(R.id.webapp_title);
        mTitlebar = findViewById(R.id.webapp_titlebar);
        mSplashscreen = findViewById(R.id.splashscreen);

        String origin = Allocator.getInstance(this).getOrigin(getIndex());
        boolean isInstallCompleting = (origin == null);

        if (!GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning) || !isInstalled || isInstallCompleting) {
            
            overridePendingTransition(R.anim.grow_fade_in_center, android.R.anim.fade_out);
            showSplash(true);
        } else {
            mSplashscreen.setVisibility(View.GONE);
        }

        if (!isInstalled || isInstallCompleting) {
            InstallHelper installHelper = new InstallHelper(getApplicationContext(), mApkResources, this);
            if (!isInstalled) {
                
                try {
                    installHelper.startInstall(getDefaultProfileName());
                } catch (IOException e) {
                    Log.e(LOGTAG, "Couldn't install packaged app", e);
                }
            } else {
                
                Log.i(LOGTAG, "Waiting for existing install to complete");
                installHelper.registerGeckoListener();
            }
            return;
        } else {
            launchWebapp(origin, mApkResources.getManifestUrl(), mApkResources.getAppName());
        }

        setTitle(mApkResources.getAppName());
    }

    @Override
    protected String getURIFromIntent(Intent intent) {
        String uri = super.getURIFromIntent(intent);
        if (uri != null) {
            return uri;
        }
        
        

        
        return mApkResources.getManifestUrl();
    }

    @Override
    protected void loadStartupTab(String uri) {
        
    }

    private void showSplash(boolean isApk) {

        
        int dominantColor = Allocator.getInstance().getColor(getIndex());

        setBackgroundGradient(dominantColor);

        ImageView image = (ImageView)findViewById(R.id.splashscreen_icon);
        Drawable d = null;

        if (isApk) {
            Uri uri = mApkResources.getAppIconUri();
            image.setImageURI(uri);
            d = image.getDrawable();
        } else {
            
            File profile = getProfile().getDir();
            File logoFile = new File(profile, "logo.png");
            if (logoFile.exists()) {
                d = Drawable.createFromPath(logoFile.getPath());
                image.setImageDrawable(d);
            }
        }

        if (d != null) {
            Animation fadein = AnimationUtils.loadAnimation(this, R.anim.grow_fade_in_center);
            fadein.setStartOffset(500);
            fadein.setDuration(1000);
            image.startAnimation(fadein);
        }
    }

    public void setBackgroundGradient(int dominantColor) {
        int[] colors = new int[2];
        
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
        mSplashscreen.setBackgroundDrawable(gd);
    }

    


    @Override
    protected String getDefaultProfileName() {
        return "webapp" + getIndex();
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
                    final URL url;

                    try {
                        url = new URL(urlString);
                    } catch (java.net.MalformedURLException ex) {
                        mTitlebarText.setText(urlString);

                        
                        
                        
                        if (urlString != null && !urlString.startsWith("app://")) {
                            mTitlebar.setVisibility(View.VISIBLE);
                        } else {
                            mTitlebar.setVisibility(View.GONE);
                        }
                        return;
                    }

                    if (mOrigin != null && mOrigin.getHost().equals(url.getHost())) {
                        mTitlebar.setVisibility(View.GONE);
                    } else {
                        mTitlebarText.setText(url.getProtocol() + "://" + url.getHost());
                        mTitlebar.setVisibility(View.VISIBLE);
                    }
                }
                break;
            case LOADED:
                hideSplash();
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

    protected void hideSplash() {
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
    }

    @Override
    public void installCompleted(InstallHelper installHelper, String event, JSONObject message) {
        if (event == null) {
            return;
        }

        if (event.equals("Webapps:Postinstall")) {
            String origin = message.optString("origin");
            launchWebapp(origin, mApkResources.getManifestUrl(), mApkResources.getAppName());
        }
    }

    @Override
    public void installErrored(InstallHelper installHelper, Exception exception) {
        Log.e(LOGTAG, "Install errored", exception);
    }

    public void launchWebapp(String origin, String manifestUrl, String name) {
        try {
            mOrigin = new URL(origin);
        } catch (java.net.MalformedURLException ex) {
            
            if (!origin.startsWith("app://")) {
                return;
            }

            
            Log.i(LOGTAG, "Webapp is not registered with allocator");
            Uri data = getIntent().getData();
            if (data != null) {
                try {
                    mOrigin = new URL(data.toString());
                } catch (java.net.MalformedURLException ex2) {
                    Log.e(LOGTAG, "Unable to parse intent url: ", ex);
                }
            }
        }
        try {
            JSONObject launchObject = new JSONObject();
            launchObject.putOpt("url", manifestUrl);
            launchObject.putOpt("name", mApkResources.getAppName());
            Log.i(LOGTAG, "Trying to launch: " + launchObject);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Webapps:Load", launchObject.toString()));
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error populating launch message", e);
        }
    }

    @Override
    protected boolean getIsDebuggable() {
        return mApkResources.isDebuggable();
    }
}
