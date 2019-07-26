



package org.mozilla.gecko.preferences;

import android.app.AlertDialog;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.text.SpannableString;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import java.util.Iterator;

import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.favicons.decoders.FaviconDecoder;
import org.mozilla.gecko.favicons.decoders.LoadFaviconResult;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.R;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.widget.FaviconView;




public class SearchEnginePreference extends CustomListPreference {
    protected String LOGTAG = "SearchEnginePreference";

    protected static final int INDEX_REMOVE_BUTTON = 1;

    
    private BitmapDrawable mPromptIcon;

    
    private Bitmap mIconBitmap;

    private FaviconView mFaviconView;

    public SearchEnginePreference(Context context, SearchPreferenceCategory parentCategory) {
        super(context, parentCategory);
    }

    





    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        
        mFaviconView = ((FaviconView) view.findViewById(R.id.search_engine_icon));
        mFaviconView.updateAndScaleImage(mIconBitmap, getTitle().toString());
    }

    @Override
    protected int getPreferenceLayoutResource() {
        return R.layout.preference_search_engine;
    }

    


    @Override
    protected String[] getDialogStrings() {
        Resources res = getContext().getResources();
        return new String[] { LABEL_SET_AS_DEFAULT,
                              res.getString(R.string.pref_dialog_remove) };
    }

    @Override
    public void showDialog() {
        
        
        if (mParentCategory.getPreferenceCount() == 1) {
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(getContext(), R.string.pref_search_last_toast, Toast.LENGTH_SHORT).show();
                }
            });
            return;
        }

        super.showDialog();
    }

    @Override
    protected void configureDialogBuilder(AlertDialog.Builder builder) {
        
        
        if (mPromptIcon == null && mIconBitmap != null) {
            mPromptIcon = new BitmapDrawable(getContext().getResources(), mFaviconView.getBitmap());
        }

        builder.setIcon(mPromptIcon);
    }

    @Override
    protected void onDialogIndexClicked(int index) {
        switch (index) {
            case INDEX_SET_DEFAULT_BUTTON:
                mParentCategory.setDefault(this);
                break;

            case INDEX_REMOVE_BUTTON:
                mParentCategory.uninstall(this);
                break;

            default:
                Log.w(LOGTAG, "Selected index out of range.");
                break;
        }
     }

    




    public void setSearchEngineFromJSON(JSONObject geckoEngineJSON) throws JSONException {
        final String engineName = geckoEngineJSON.getString("name");
        final SpannableString titleSpannable = new SpannableString(engineName);

        setTitle(titleSpannable);

        final String iconURI = geckoEngineJSON.getString("iconURI");
        
        try {

            
            

            LoadFaviconResult result = FaviconDecoder.decodeDataURI(iconURI);
            if (result == null) {
                
                Log.w(LOGTAG, "Unable to decode icon URI.");
                return;
            }

            Iterator<Bitmap> bitmaps = result.getBitmaps();
            if (!bitmaps.hasNext()) {
                Log.w(LOGTAG, "No bitmaps in decoded icon.");
                return;
            }

            mIconBitmap = bitmaps.next();

            if (!bitmaps.hasNext()) {
                
                return;
            }

            
            final int desiredWidth;
            if (mFaviconView != null) {
                desiredWidth = mFaviconView.getWidth();
            } else {
                
                
                
                if (Favicons.sLargestFaviconSize == 0) {
                    desiredWidth = 128;
                } else {
                    desiredWidth = Favicons.sLargestFaviconSize;
                }
            }

            int currentWidth = mIconBitmap.getWidth();
            while ((currentWidth < desiredWidth) &&
                   bitmaps.hasNext()) {
                Bitmap b = bitmaps.next();
                if (b.getWidth() > currentWidth) {
                    currentWidth = b.getWidth();
                    mIconBitmap = b;
                }
            }

        } catch (IllegalArgumentException e) {
            Log.e(LOGTAG, "IllegalArgumentException creating Bitmap. Most likely a zero-length bitmap.", e);
        } catch (NullPointerException e) {
            Log.e(LOGTAG, "NullPointerException creating Bitmap. Most likely a zero-length bitmap.", e);
        }
    }
}
