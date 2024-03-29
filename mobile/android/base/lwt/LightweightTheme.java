




package org.mozilla.gecko.lwt;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONObject;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.WindowUtils;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.ThreadUtils.AssertBehavior;

import android.app.Application;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewParent;

public class LightweightTheme implements GeckoEventListener {
    private static final String LOGTAG = "GeckoLightweightTheme";

    private static final String PREFS_URL = "lightweightTheme.headerURL";
    private static final String PREFS_COLOR = "lightweightTheme.color";

    private final Application mApplication;

    private Bitmap mBitmap;
    private int mColor;
    private boolean mIsLight;

    public static interface OnChangeListener {
        
        public void onLightweightThemeChanged();

        
        public void onLightweightThemeReset();
    }

    private final List<OnChangeListener> mListeners;

    class LightweightThemeRunnable implements Runnable {
        private String mHeaderURL;
        private String mColor;

        private String mSavedURL;
        private String mSavedColor;

        LightweightThemeRunnable() {
        }

        LightweightThemeRunnable(final String headerURL, final String color) {
            mHeaderURL = headerURL;
            mColor = color;
        }

        private void loadFromPrefs() {
            SharedPreferences prefs = GeckoSharedPrefs.forProfile(mApplication);
            mSavedURL = prefs.getString(PREFS_URL, null);
            mSavedColor = prefs.getString(PREFS_COLOR, null);
        }

        private void saveToPrefs() {
            GeckoSharedPrefs.forProfile(mApplication)
                            .edit()
                            .putString(PREFS_URL, mHeaderURL)
                            .putString(PREFS_COLOR, mColor)
                            .apply();

            
            mSavedURL = mHeaderURL;
            mSavedColor = mColor;
        }

        @Override
        public void run() {
            
            loadFromPrefs();

            if (TextUtils.isEmpty(mHeaderURL)) {
                
                
                mHeaderURL = mSavedURL;
                mColor = mSavedColor;
                if (TextUtils.isEmpty(mHeaderURL)) {
                    
                    
                    return;
                }
            } else if (TextUtils.equals(mHeaderURL, mSavedURL)) {
                
                
                return;
            } else {
                
                saveToPrefs();
            }

            String croppedURL = mHeaderURL;
            int mark = croppedURL.indexOf('?');
            if (mark != -1) {
                croppedURL = croppedURL.substring(0, mark);
            }

            
            final Bitmap bitmap = BitmapUtils.decodeUrl(croppedURL);
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    setLightweightTheme(bitmap, mColor);
                }
            });
        }
    }

    public LightweightTheme(Application application) {
        mApplication = application;
        mListeners = new ArrayList<OnChangeListener>();

        
        EventDispatcher.getInstance().registerGeckoThreadListener(this,
            "LightweightTheme:Update",
            "LightweightTheme:Disable");

        ThreadUtils.postToBackgroundThread(new LightweightThemeRunnable());
    }

    public void addListener(final OnChangeListener listener) {
        
        
        mListeners.add(listener);
    }

    public void removeListener(OnChangeListener listener) {
        mListeners.remove(listener);
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("LightweightTheme:Update")) {
                JSONObject lightweightTheme = message.getJSONObject("data");
                final String headerURL = lightweightTheme.getString("headerURL");
                final String color = lightweightTheme.optString("accentcolor");

                ThreadUtils.postToBackgroundThread(new LightweightThemeRunnable(headerURL, color));
            } else if (event.equals("LightweightTheme:Disable")) {
                
                
                clearPrefs();

                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        resetLightweightTheme();
                    }
                });
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    


    private void clearPrefs() {
        GeckoSharedPrefs.forProfile(mApplication)
                        .edit()
                        .remove(PREFS_URL)
                        .remove(PREFS_COLOR)
                        .apply();
    }

    







    private void setLightweightTheme(Bitmap bitmap, String color) {
        if (bitmap == null || bitmap.getWidth() == 0 || bitmap.getHeight() == 0) {
            mBitmap = null;
            return;
        }

        
        final int maxWidth = WindowUtils.getLargestDimension(mApplication);

        
        final int bitmapWidth = bitmap.getWidth();
        final int bitmapHeight = bitmap.getHeight();

        try {
            mColor = Color.parseColor(color);
        } catch (Exception e) {
            
            
            mColor = Color.TRANSPARENT;
        }

        
        double luminance = (0.2125 * ((mColor & 0x00FF0000) >> 16)) +
                           (0.7154 * ((mColor & 0x0000FF00) >> 8)) +
                           (0.0721 * (mColor &0x000000FF));
        mIsLight = luminance > 110;

        
        
        if (bitmapWidth >= maxWidth) {
            mBitmap = Bitmap.createBitmap(bitmap, bitmapWidth - maxWidth, 0, maxWidth, bitmapHeight);
        } else {
            Paint paint = new Paint();
            paint.setAntiAlias(true);

            
            
            
            mBitmap = Bitmap.createBitmap(maxWidth, bitmapHeight, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(mBitmap);

            
            canvas.drawColor(mColor);

            
            Rect rect = new Rect();
            Gravity.apply(Gravity.TOP | Gravity.RIGHT,
                          bitmapWidth,
                          bitmapHeight,
                          new Rect(0, 0, maxWidth, bitmapHeight),
                          rect);

            
            canvas.drawBitmap(bitmap, null, rect, paint);
        }

        for (OnChangeListener listener : mListeners) {
            listener.onLightweightThemeChanged();
        }
    }

    




    private void resetLightweightTheme() {
        ThreadUtils.assertOnUiThread(AssertBehavior.NONE);
        if (mBitmap == null) {
            return;
        }

        
        mBitmap = null;

        for (OnChangeListener listener : mListeners) {
            listener.onLightweightThemeReset();
        }
    }

    




    public boolean isEnabled() {
        return (mBitmap != null);
    }

    




    public boolean isLightTheme() {
        return mIsLight;
    }

    






    private Bitmap getCroppedBitmap(View view) {
        if (mBitmap == null || view == null) {
            return null;
        }

        
        Rect rect = new Rect();
        view.getGlobalVisibleRect(rect);

        
        Rect window = new Rect();
        view.getWindowVisibleDisplayFrame(window);

        
        int screenWidth = view.getContext().getResources().getDisplayMetrics().widthPixels;
        int left = mBitmap.getWidth() - screenWidth + rect.left;
        int right = mBitmap.getWidth() - screenWidth + rect.right;
        int top = rect.top - window.top;
        int bottom = rect.bottom - window.top;

        int offsetX = 0;
        int offsetY = 0;

        
        ViewParent parent;
        View curView = view;
        do {
            if (Versions.feature11Plus) {
                offsetX += (int) curView.getTranslationX() - curView.getScrollX();
                offsetY += (int) curView.getTranslationY() - curView.getScrollY();
            } else {
                offsetX -= curView.getScrollX();
                offsetY -= curView.getScrollY();
            }

            parent = curView.getParent();

            if (parent instanceof View) {
                curView = (View) parent;
            }

        } while(parent instanceof View);

        
        left -= offsetX;
        right -= offsetX;
        top -= offsetY;
        bottom -= offsetY;

        
        
        int width = right - left;
        int height = (bottom > mBitmap.getHeight() ? mBitmap.getHeight() - top : bottom - top);

        
        
        
        
        try {
            return Bitmap.createBitmap(mBitmap, left, top, width, height);
        } catch (Exception e) {
            return null;
        }
    }

    





    public Drawable getDrawable(View view) {
        Bitmap bitmap = getCroppedBitmap(view);
        if (bitmap == null) {
            return null;
        }

        BitmapDrawable drawable = new BitmapDrawable(view.getContext().getResources(), bitmap);
        drawable.setGravity(Gravity.TOP | Gravity.RIGHT);
        drawable.setTileModeXY(Shader.TileMode.CLAMP, Shader.TileMode.CLAMP);
        return drawable;
    }

    





     public LightweightThemeDrawable getColorDrawable(View view) {
         return getColorDrawable(view, mColor, false);
     }

    






    public LightweightThemeDrawable getColorDrawable(View view, int color) {
        return getColorDrawable(view, color, false);
    }

    







    public LightweightThemeDrawable getColorDrawable(View view, int color, boolean needsDominantColor) {
        Bitmap bitmap = getCroppedBitmap(view);
        if (bitmap == null) {
            return null;
        }

        LightweightThemeDrawable drawable = new LightweightThemeDrawable(view.getContext().getResources(), bitmap);
        if (needsDominantColor) {
            drawable.setColorWithFilter(color, (mColor & 0x22FFFFFF));
        } else {
            drawable.setColor(color);
        }

        return drawable;
    }
}
