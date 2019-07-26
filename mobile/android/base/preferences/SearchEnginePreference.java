



package org.mozilla.gecko.preferences;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.os.Build;
import android.preference.Preference;
import android.text.SpannableString;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.R;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.ThreadUtils;




public class SearchEnginePreference extends Preference {
    private static final String LOGTAG = "SearchEnginePreference";

    
    public static int sIconSize;

    
    public static final int INDEX_SET_DEFAULT_BUTTON = 0;
    public static final int INDEX_REMOVE_BUTTON = 1;

    
    public final String LABEL_IS_DEFAULT;

    
    private boolean mIsDefaultEngine;
    
    private boolean mIsImmutableEngine;

    
    private String[] mDialogItems;

    
    private AlertDialog mDialog;

    private final SearchPreferenceCategory mParentCategory;

    






    public SearchEnginePreference(Context context, SearchPreferenceCategory parentCategory) {
        super(context);
        mParentCategory = parentCategory;

        Resources res = getContext().getResources();

        
        sIconSize = res.getDimensionPixelSize(R.dimen.searchpreferences_icon_size);
        setOnPreferenceClickListener(new OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                SearchEnginePreference sPref = (SearchEnginePreference) preference;
                sPref.showDialog();

                return true;
            }
        });

        
        LABEL_IS_DEFAULT = res.getString(R.string.pref_search_default);

        
        mDialogItems = new String[] { res.getString(R.string.pref_search_set_default),
                                      res.getString(R.string.pref_search_remove) };
    }

    




    public void setSearchEngineFromJSON(JSONObject geckoEngineJSON) throws JSONException {
        final String engineName = geckoEngineJSON.getString("name");
        SpannableString titleSpannable = new SpannableString(engineName);
        mIsImmutableEngine = geckoEngineJSON.getBoolean("immutable");

        if (mIsImmutableEngine) {
            
            mDialogItems = new String[] { getContext().getResources().getString(R.string.pref_search_set_default) };
        }
        setTitle(titleSpannable);

        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            
            String iconURI = geckoEngineJSON.getString("iconURI");
            Bitmap iconBitmap = BitmapUtils.getBitmapFromDataURI(iconURI);
            Bitmap scaledIconBitmap = Bitmap.createScaledBitmap(iconBitmap, sIconSize, sIconSize, false);
            BitmapDrawable drawable = new BitmapDrawable(scaledIconBitmap);
            setIcon(drawable);
        }
    }

    



    public void setIsDefaultEngine(boolean isDefault) {
        mIsDefaultEngine = isDefault;
        if (isDefault) {
            setOrder(0);
            setSummary(LABEL_IS_DEFAULT);
        } else {
            setOrder(1);
            setSummary("");
        }
    }

    






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

        
        if (mIsDefaultEngine && mIsImmutableEngine) {
            return;
        }

        final AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder.setTitle(getTitle().toString());
        builder.setItems(mDialogItems, new DialogInterface.OnClickListener() {
            
            @Override
            public void onClick(DialogInterface dialog, int indexClicked) {
                hideDialog();
                switch (indexClicked) {
                    case INDEX_SET_DEFAULT_BUTTON:
                        mParentCategory.setDefault(SearchEnginePreference.this);
                        break;
                    case INDEX_REMOVE_BUTTON:
                        mParentCategory.uninstall(SearchEnginePreference.this);
                        break;
                    default:
                        Log.w(LOGTAG, "Selected index out of range.");
                        break;
                }
            }
        });

        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            builder.setIcon(getIcon());
        }

        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                mDialog = builder.create();
                mDialog.setOnShowListener(new DialogInterface.OnShowListener() {
                    
                    @Override
                    public void onShow(DialogInterface dialog) {
                        configureShownDialog();
                    }
                });
                mDialog.show();
            }
        });
    }

    


    public void hideDialog() {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                
                
                if (mDialog != null && mDialog.isShowing()) {
                    mDialog.dismiss();
                }
            }
        });
    }

    




    private void configureShownDialog() {
        
        TextView defaultButton = (TextView) mDialog.getListView().getChildAt(INDEX_SET_DEFAULT_BUTTON);
        
        if (mIsDefaultEngine) {
            defaultButton.setEnabled(false);
            
            
            defaultButton.setOnClickListener(null);
        }
    }
}
