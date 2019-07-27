




package org.mozilla.gecko.webapp;

import java.io.File;
import java.io.IOException;
import java.net.URI;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.GeckoThread;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.StubBrowserDB;
import org.mozilla.gecko.mozglue.ContextUtils.SafeIntent;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.gecko.webapp.InstallHelper.InstallCallback;

import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Color;
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

    private URI mOrigin;
    private TextView mTitlebarText;
    private View mTitlebar;

    
    View mSplashscreen;

    private boolean mIsApk = true;
    private ApkResources mApkResources;
    private String mManifestUrl;
    private String mAppName;

    protected int getIndex() { return 0; }

    @Override
    public int getLayout() { return R.layout.web_app; }

    public WebappImpl() {
        GeckoProfile.setBrowserDBFactory(new BrowserDB.Factory() {
            @Override
            public BrowserDB get(String profileName, File profileDir) {
                return new StubBrowserDB(profileName);
            }
        });
    }

    @Override
    public void onCreate(Bundle savedInstance) {
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
            Log.w(LOGTAG, "no package name; treating as legacy shortcut");

            mIsApk = false;

            
            isInstalled = true;

            Uri data = getIntent().getData();
            if (data == null) {
                Log.wtf(LOGTAG, "can't get manifest URL from shortcut data");
                setResult(RESULT_CANCELED);
                finish();
                return;
            }
            mManifestUrl = data.toString();

            String shortcutName = extras.getString(Intent.EXTRA_SHORTCUT_NAME);
            mAppName = shortcutName != null ? shortcutName : "Web App";
        } else {
            try {
                mApkResources = new ApkResources(this, packageName);
            } catch (NameNotFoundException e) {
                Log.e(LOGTAG, "Can't find package for webapp " + packageName, e);
                setResult(RESULT_CANCELED);
                finish();
                return;
            }

            mManifestUrl = mApkResources.getManifestUrl();
            mAppName = mApkResources.getAppName();
        }

        
        super.onCreate(savedInstance);

        mTitlebarText = (TextView)findViewById(R.id.webapp_title);
        mTitlebar = findViewById(R.id.webapp_titlebar);
        mSplashscreen = findViewById(R.id.splashscreen);

        Allocator allocator = Allocator.getInstance(this);
        int index = getIndex();

        
        
        allocator.maybeMigrateOldPrefs(index);

        String origin = allocator.getOrigin(index);
        boolean isInstallCompleting = (origin == null);

        if (!GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning) || !isInstalled || isInstallCompleting) {
            
            overridePendingTransition(R.anim.grow_fade_in_center, android.R.anim.fade_out);
            showSplash();
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
        }

        launchWebapp(origin);

        setTitle(mAppName);
    }

    @Override
    protected String getURIFromIntent(SafeIntent intent) {
        String uri = super.getURIFromIntent(intent);
        if (uri != null) {
            return uri;
        }
        
        

        
        if (mIsApk) {
            return mApkResources.getManifestUrl();
        }

        
        
        
        Log.wtf(LOGTAG, "Couldn't get URI from intent nor APK resources");
        return null;
    }

    @Override
    protected void loadStartupTab(String uri, int flags) {
        
        
        flags = Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_USER_ENTERED | Tabs.LOADURL_EXTERNAL;
        super.loadStartupTab("about:blank", flags);
    }

    private void showSplash() {

        
        int dominantColor = Allocator.getInstance().getColor(getIndex());

        setBackgroundGradient(dominantColor);

        ImageView image = (ImageView)findViewById(R.id.splashscreen_icon);
        Drawable d = null;

        if (mIsApk) {
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

                    
                    
                    
                    if (urlString != null && urlString.equals("about:blank")) {
                        mTitlebar.setVisibility(View.GONE);
                        return;
                    }

                    final URI uri;

                    try {
                        uri = new URI(urlString);
                    } catch (java.net.URISyntaxException ex) {
                        mTitlebarText.setText(urlString);

                        
                        
                        
                        if (urlString != null && !urlString.startsWith("app://")) {
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
    public void installCompleted(InstallHelper installHelper, String event, NativeJSObject message) {
        if (event == null) {
            return;
        }

        if (event.equals("Webapps:Postinstall")) {
            String origin = message.optString("origin", null);
            launchWebapp(origin);
        }
    }

    @Override
    public void installErrored(InstallHelper installHelper, Exception exception) {
        Log.e(LOGTAG, "Install errored", exception);
    }

    private void setOrigin(String origin) {
        try {
            mOrigin = new URI(origin);
        } catch (java.net.URISyntaxException ex) {
            
            if (!origin.startsWith("app://")) {
                return;
            }

            
            if (!mIsApk) {
                Log.i(LOGTAG, "Origin is app: URL; falling back to intent URL");
                Uri data = getIntent().getData();
                if (data != null) {
                    try {
                        mOrigin = new URI(data.toString());
                    } catch (java.net.URISyntaxException ex2) {
                        Log.e(LOGTAG, "Unable to parse intent URL: ", ex);
                    }
                }
            }
        }
    }

    public void launchWebapp(String origin) {
        setOrigin(origin);

        try {
            JSONObject launchObject = new JSONObject();
            launchObject.putOpt("url", mManifestUrl);
            launchObject.putOpt("name", mAppName);
            Log.i(LOGTAG, "Trying to launch: " + launchObject);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Webapps:Load", launchObject.toString()));
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error populating launch message", e);
        }
    }

    @Override
    protected boolean getIsDebuggable() {
        if (mIsApk) {
            return mApkResources.isDebuggable();
        }

        
        
        return false;
    }
}
