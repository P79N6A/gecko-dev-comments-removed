



package org.mozilla.gecko.widget;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.sync.setup.activities.SetupSyncActivity;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.OnAccountsUpdateListener;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.text.SpannableString;
import android.text.style.StyleSpan;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Random;







public class AboutHomePromoBox extends TextView implements View.OnClickListener {
    private static final String LOGTAG = "GeckoAboutHomePromoBox";

    


    public static class Type {
        public int text;
        public int boldText;
        public int image;
        public Type(int aText, int aBoldText, int aImage) {
            text = aText;
            boldText = aBoldText;
            image = aImage;
        }
        public boolean canShow() {
            return true;
        }
        public void onClick(View v) { }
        public void onDestroy() { }
    }

    @Override
    protected void onDetachedFromWindow() {
        for (Type type : mTypes) {
            type.onDestroy();
        }
    }

    private class SyncType extends Type {
        private OnAccountsUpdateListener mAccountListener;
        public SyncType(int aText, int aBoldText, int aImage) {
            super(aText, aBoldText, aImage);
            
            mAccountListener = new OnAccountsUpdateListener() {
                @Override
                public void onAccountsUpdated(Account[] accounts) {
                    showRandomPromo();
                }
            };
            AccountManager.get(mContext).addOnAccountsUpdatedListener(mAccountListener, ThreadUtils.getBackgroundHandler(), false);
        }
        @Override
        public boolean canShow() {
             return !SyncAccounts.syncAccountsExist(mContext);
        }
        @Override
        public void onClick(View v) {
            final Context context = v.getContext();
            final Intent intent = new Intent(context, SetupSyncActivity.class);
            context.startActivity(intent);
        }

        @Override
        public void onDestroy() {
            if (mAccountListener != null) {
                AccountManager.get(mContext).removeOnAccountsUpdatedListener(mAccountListener);
                mAccountListener = null;
            }
        }
    }

    private static int sTypeIndex = -1;
    private ArrayList<Type> mTypes;
    private Type mType;

    private final Context mContext;

    public AboutHomePromoBox(Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
        setOnClickListener(this);

        mTypes = new ArrayList<Type>();
        mTypes.add(new SyncType(R.string.abouthome_about_sync,
                            R.string.abouthome_sync_bold_name,
                            R.drawable.abouthome_promo_logo_sync));

        mTypes.add(new Type(R.string.abouthome_about_apps,
                            R.string.abouthome_apps_bold_name,
                            R.drawable.abouthome_promo_logo_apps) {
            @Override
            public boolean canShow() {
                final ContentResolver resolver = mContext.getContentResolver();
                return !BrowserDB.isVisited(resolver, "https://marketplace.firefox.com/");
            }
            @Override
            public void onClick(View v) {
                Tabs.getInstance().loadUrl("https://marketplace.firefox.com/", Tabs.LOADURL_NEW_TAB);

                
                
                
                v.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        showRandomPromo();
                    }
                }, 5000);
            }
        });
    }

    @Override
    public void onClick(View v) {
        if (mType != null)
            mType.onClick(v);
    }

    private interface GetTypesCallback {
        void onGotTypes(ArrayList<Type> types);
    }

    



    public void showRandomPromo() {
        getAvailableTypes(new GetTypesCallback() {
            @Override
            public void onGotTypes(ArrayList<Type> types) {
                if (types.size() == 0) {
                    hide();
                    return;
                }

                
                if (AboutHomePromoBox.sTypeIndex == -1 || AboutHomePromoBox.sTypeIndex >= types.size()) {
                    AboutHomePromoBox.sTypeIndex = new Random().nextInt(types.size());
                }
                mType = types.get(AboutHomePromoBox.sTypeIndex);

                updateViewResources();
                setVisibility(View.VISIBLE);
            }
        });
    }

    public void hide() {
        setVisibility(View.GONE);
        mType = null;
    }

    private void updateViewResources() {
        updateTextViewResources();
        setCompoundDrawablesWithIntrinsicBounds(mType.image, 0, 0, 0);
    }

    private void updateTextViewResources() {
        final String text = mContext.getResources().getString(mType.text);
        final String boldText = mContext.getResources().getString(mType.boldText);
        final int styleIndex = text.indexOf(boldText);
        if (styleIndex < 0)
            setText(text);
        else {
            final SpannableString spannableText = new SpannableString(text);
            spannableText.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), styleIndex, styleIndex + boldText.length(), 0);
            setText(spannableText, TextView.BufferType.SPANNABLE);
        }
    }

    private void getAvailableTypes(final GetTypesCallback callback) {
        (new UiAsyncTask<Void, Void, ArrayList<Type>>(ThreadUtils.getBackgroundHandler()) {
            @Override
            public ArrayList<Type> doInBackground(Void... params) {
                
                ArrayList<Type> availTypes = new ArrayList<Type>();
                for (int i = 0; i < mTypes.size(); i++) {
                    Type t = mTypes.get(i);
                    if (t.canShow()) {
                        availTypes.add(t);
                    }
                }
                return availTypes;
            }

            @Override
            public void onPostExecute(ArrayList<Type> types) {
                callback.onGotTypes(types);
            }
        }).execute();
    }
}
