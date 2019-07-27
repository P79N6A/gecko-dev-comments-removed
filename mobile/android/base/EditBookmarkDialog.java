




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UIAsyncTask;

import android.content.ContentResolver;
import android.content.Context;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.database.Cursor;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;







public class EditBookmarkDialog {
    private final Context mContext;

    public EditBookmarkDialog(Context context) {
        mContext = context;
    }

    


    private class Bookmark {
        final int id;
        final String title;
        final String url;
        final String keyword;

        public Bookmark(int aId, String aTitle, String aUrl, String aKeyword) {
            id = aId;
            title = aTitle;
            url = aUrl;
            keyword = aKeyword;
        }
    }

    







    private class EditBookmarkTextWatcher implements TextWatcher {
        
        protected AlertDialog mDialog;

        
        protected EditBookmarkTextWatcher mPairedTextWatcher;

        
        protected boolean mEnabled = true;

        public EditBookmarkTextWatcher(AlertDialog aDialog) {
            mDialog = aDialog;
        }

        public void setPairedTextWatcher(EditBookmarkTextWatcher aTextWatcher) {
            mPairedTextWatcher = aTextWatcher;
        }

        public boolean isEnabled() {
            return mEnabled;
        }

        
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            
            boolean enabled = mEnabled && (mPairedTextWatcher == null || mPairedTextWatcher.isEnabled());
            mDialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(enabled);
        }

        @Override
        public void afterTextChanged(Editable s) {}
        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
    }

    



    private class LocationTextWatcher extends EditBookmarkTextWatcher {
        public LocationTextWatcher(AlertDialog aDialog) {
            super(aDialog);
        }

        
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            mEnabled = (s.toString().trim().length() > 0);
            super.onTextChanged(s, start, before, count);
        }
    }

    



    private class KeywordTextWatcher extends EditBookmarkTextWatcher {
        public KeywordTextWatcher(AlertDialog aDialog) {
            super(aDialog);
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            
            mEnabled = (s.toString().trim().indexOf(' ') == -1);
            super.onTextChanged(s, start, before, count);
       }
    }

    








    public void show(final String url) {
        final ContentResolver cr = mContext.getContentResolver();
        (new UIAsyncTask.WithoutParams<Bookmark>(ThreadUtils.getBackgroundHandler()) {
            @Override
            public Bookmark doInBackground() {
                final Cursor cursor = BrowserDB.getBookmarkForUrl(cr, url);
                if (cursor == null) {
                    return null;
                }

                Bookmark bookmark = null;
                try {
                    cursor.moveToFirst();
                    bookmark = new Bookmark(cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks._ID)),
                                                          cursor.getString(cursor.getColumnIndexOrThrow(Bookmarks.TITLE)),
                                                          cursor.getString(cursor.getColumnIndexOrThrow(Bookmarks.URL)),
                                                          cursor.getString(cursor.getColumnIndexOrThrow(Bookmarks.KEYWORD)));
                } finally {
                    cursor.close();
                }
                return bookmark;
            }

            @Override
            public void onPostExecute(Bookmark bookmark) {
                if (bookmark == null) {
                    return;
                }

                show(bookmark.id, bookmark.title, bookmark.url, bookmark.keyword);
            }
        }).execute();
    }

    










    public void show(final int id, final String title, final String url, final String keyword) {
        final Context context = mContext;

        AlertDialog.Builder editPrompt = new AlertDialog.Builder(context);
        final View editView = LayoutInflater.from(context).inflate(R.layout.bookmark_edit, null);
        editPrompt.setTitle(R.string.bookmark_edit_title);
        editPrompt.setView(editView);

        final EditText nameText = ((EditText) editView.findViewById(R.id.edit_bookmark_name));
        final EditText locationText = ((EditText) editView.findViewById(R.id.edit_bookmark_location));
        final EditText keywordText = ((EditText) editView.findViewById(R.id.edit_bookmark_keyword));
        nameText.setText(title);
        locationText.setText(url);
        keywordText.setText(keyword);

        editPrompt.setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int whichButton) {
                (new UIAsyncTask.WithoutParams<Void>(ThreadUtils.getBackgroundHandler()) {
                    @Override
                    public Void doInBackground() {
                        String newUrl = locationText.getText().toString().trim();
                        String newKeyword = keywordText.getText().toString().trim();
                        BrowserDB.updateBookmark(context.getContentResolver(), id, newUrl, nameText.getText().toString(), newKeyword);
                        return null;
                    }

                    @Override
                    public void onPostExecute(Void result) {
                        Toast.makeText(context, R.string.bookmark_updated, Toast.LENGTH_SHORT).show();
                    }
                }).execute();
            }
        });

        editPrompt.setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int whichButton) {
                  
              }
        });

        final AlertDialog dialog = editPrompt.create();

        
        LocationTextWatcher locationTextWatcher = new LocationTextWatcher(dialog);
        KeywordTextWatcher keywordTextWatcher = new KeywordTextWatcher(dialog);

        
        locationTextWatcher.setPairedTextWatcher(keywordTextWatcher);
        keywordTextWatcher.setPairedTextWatcher(locationTextWatcher);

        
        locationText.addTextChangedListener(locationTextWatcher);
        keywordText.addTextChangedListener(keywordTextWatcher);

        dialog.show();
    }
}
