




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONObject;

import android.app.Application;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.Rect;
import android.graphics.Shader;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.View;
import android.view.ViewParent;

import java.net.URL;
import java.io.InputStream;
import java.util.List;
import java.util.ArrayList;

import android.util.Log;

public class LightweightTheme implements GeckoEventListener {
    private static final String LOGTAG = "GeckoLightweightTheme";

    private Application mApplication;
    private Bitmap mBitmap;
    private int mColor;
    private boolean mIsLight;

    public static interface OnChangeListener {
        
        
        public boolean post(Runnable action);

        
        public void onLightweightThemeChanged();

        
        public void onLightweightThemeReset();
    }

    private List<OnChangeListener> mListeners;
    
    public LightweightTheme(Application application) {
        mApplication = application;
        mListeners = new ArrayList<OnChangeListener>();

        GeckoAppShell.getEventDispatcher().registerEventListener("LightweightTheme:Update", this);
        GeckoAppShell.getEventDispatcher().registerEventListener("LightweightTheme:Disable", this);
    }

    public void addListener(final OnChangeListener listener) {
        
        
        mListeners.add(listener);
    }

    public void removeListener(OnChangeListener listener) {
        mListeners.remove(listener);
    }

    public void setLightweightTheme(String headerURL) {
        try {
            
            URL url = new URL(headerURL);
            InputStream stream = url.openStream();
            mBitmap = BitmapFactory.decodeStream(stream);
            stream.close();

            
            if (mBitmap == null || mBitmap.getWidth() == 0 || mBitmap.getHeight() == 0) {
                mBitmap = null;
                return;
            }

            
            DisplayMetrics dm = mApplication.getResources().getDisplayMetrics();
            int maxWidth = Math.max(dm.widthPixels, dm.heightPixels);
            int height = (int) (mBitmap.getHeight() * 0.25);
            Bitmap cropped = Bitmap.createBitmap(mBitmap, mBitmap.getWidth() - maxWidth,
                                                          mBitmap.getHeight() - height, 
                                                          maxWidth, height);
            mColor = BitmapUtils.getDominantColor(cropped, false);

            double luminance = (0.2125 * ((mColor & 0x00FF0000) >> 16)) + 
                               (0.7154 * ((mColor & 0x0000FF00) >> 8)) + 
                               (0.0721 * (mColor &0x000000FF));
            mIsLight = (luminance > 110) ? true : false;

            notifyListeners();
        } catch(java.net.MalformedURLException e) {
            mBitmap = null;
        } catch(java.io.IOException e) {
            mBitmap = null;
        }
    }

    public void resetLightweightTheme() {
        if (mBitmap != null) {
            
            mBitmap = null;

            
            for (OnChangeListener listener : mListeners) {
                 final OnChangeListener oneListener = listener;
                 oneListener.post(new Runnable() {
                     @Override
                     public void run() {
                         oneListener.onLightweightThemeReset();
                     }
                 });
            }
        }
    }

    public void notifyListeners() {
        if (mBitmap == null)
            return;

        
        for (OnChangeListener listener : mListeners) {
             final OnChangeListener oneListener = listener;
             oneListener.post(new Runnable() {
                 @Override
                 public void run() {
                     oneListener.onLightweightThemeChanged();
                 }
             });
        }
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("LightweightTheme:Update")) {
                JSONObject lightweightTheme = message.getJSONObject("data");
                String headerURL = lightweightTheme.getString("headerURL"); 
                int mark = headerURL.indexOf('?');
                if (mark != -1)
                    headerURL = headerURL.substring(0, mark);
                setLightweightTheme(headerURL);
            } else if (event.equals("LightweightTheme:Disable")) {
                resetLightweightTheme();
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }


    




    public boolean isEnabled() {
        return (mBitmap != null);
    }

    




    public boolean isLightTheme() {
        return mIsLight;
    }

    






    private Bitmap getCroppedBitmap(View view) {
        if (mBitmap == null || view == null)
            return null;

        
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
            if (Build.VERSION.SDK_INT >= 11) {
                offsetX += (int) curView.getTranslationX() - curView.getScrollX();
                offsetY += (int) curView.getTranslationY() - curView.getScrollY();
            } else {
                offsetX -= curView.getScrollX();
                offsetY -= curView.getScrollY();
            }

            parent = curView.getParent();

            if (parent instanceof View)
                curView = (View) parent;

        } while(parent instanceof View && parent != null);

        
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
        if (bitmap == null)
            return null;

        BitmapDrawable drawable = new BitmapDrawable(view.getContext().getResources(), bitmap);
        drawable.setGravity(Gravity.TOP|Gravity.RIGHT);
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
        if (bitmap == null)
            return null;

        LightweightThemeDrawable drawable = new LightweightThemeDrawable(view.getContext().getResources(), bitmap);
        if (needsDominantColor)
            drawable.setColorWithFilter(color, (mColor & 0x22FFFFFF));
        else
            drawable.setColor(color);

        return drawable;
    }
}
