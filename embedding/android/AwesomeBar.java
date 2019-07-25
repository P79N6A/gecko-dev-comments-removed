






































package org.mozilla.gecko;

import java.io.*;

import org.mozilla.gecko.*;

import android.os.*;
import android.content.*;
import android.app.*;
import android.text.*;
import android.util.*;
import android.widget.*;
import android.database.sqlite.*;
import android.database.*;
import android.view.*;
import android.view.View.*;
import android.net.Uri;
import android.graphics.*;

public class AwesomeBar extends ListActivity {
    public static final String URL_KEY = "url";
    public static final String TITLE_KEY = "title";
    public static final String CURRENT_URL_KEY = "currenturl";
    public static final String TYPE = "type";
    public static enum Type { ADD, EDIT };
    
    private String mType;

    public class AwesomeBarCursorAdapter extends SimpleCursorAdapter {
        private Cursor mAdapterCursor;
        private Context mContext;

        public AwesomeBarCursorAdapter(Context context, int layout, Cursor c, String[] from, int[] to) {
            
            super(context, layout, c, from, to);
            mAdapterCursor = c;
            mContext = context;
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            super.bindView(view, context, cursor);

            
            
            
            
        }
    }
    
    private Cursor mCursor;
    private SQLiteDatabase mDb;
    private AwesomeBarCursorAdapter adapter;

    private String getProfilePath() {
        File home = new File(getFilesDir(), "mozilla");
        if (!home.exists())
            return null;

        File profile = null;
        String[] files = home.list();
        for (int i = 0; i < files.length; i++) {
            if (files[i].endsWith(".default")) {
                profile = new File(home, files[i]);
                break;
            }
        }

        if (profile == null)
            return null;

        File webapps = new File(profile, "places.sqlite");
        if (!webapps.exists())
            return null;

        return webapps.getPath();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.d("AwesomeBar", "creating awesomebar");

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.awesomebar_search);

        
        adapter = new AwesomeBarCursorAdapter(
            this,
            R.layout.awesomebar_row,
            null,
            new String[] { TITLE_KEY, URL_KEY },
            new int[] { R.id.title, R.id.url }
        );
        setListAdapter(adapter);

        final EditText text = (EditText)findViewById(R.id.awesomebar_text);

        Intent intent = getIntent();
        String currentUrl = intent.getStringExtra(CURRENT_URL_KEY);
        mType = intent.getStringExtra(TYPE);
        if (currentUrl != null) {
            text.setText(currentUrl);
            text.selectAll();
        }

        text.addTextChangedListener(new TextWatcher() {
                
            public void afterTextChanged(Editable s) {
                
            }
            
            public void beforeTextChanged(CharSequence s, int start, int count,
                                          int after) {
                
            }
            
            public void onTextChanged(CharSequence s, int start, int before,
                                      int count) {
                adapter.getFilter().filter(s.toString());
            }
                
        });

        text.setOnKeyListener(new View.OnKeyListener() {
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_ENTER) {
                    if (event.getAction() != KeyEvent.ACTION_DOWN)
                        return true;

                    Intent resultIntent = new Intent();
                    resultIntent.putExtra(URL_KEY, text.getText().toString());
                    resultIntent.putExtra(TYPE, mType);
                    setResult(Activity.RESULT_OK, resultIntent);
                    finish();
                    return true;
                } else {
                    return false;
                }
            }
        });

        DatabaseHelper dbHelper = new DatabaseHelper(this);
        mDb = dbHelper.getReadableDatabase();

        adapter.setFilterQueryProvider(new FilterQueryProvider() {
            public Cursor runQuery(CharSequence constraint) {

                
                mCursor = mDb.rawQuery(
                        "SELECT 0 AS _id, title, url "
                          + "FROM moz_places "
                          + "WHERE (url LIKE ? OR title LIKE ?) "
                          + "LIMIT 12",
                        new String[] {"%" + constraint.toString() + "%", "%" + constraint.toString() + "%",});

                return mCursor;
            }
        });

        
        adapter.getFilter().filter("");

    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mCursor != null) mCursor.close();
        if (mDb != null) mDb.close();
    }

    @Override
    public void onListItemClick(ListView l, View v, int position, long id) {
        Cursor cursor = (Cursor)l.getItemAtPosition(position);
        String url = cursor.getString(cursor.getColumnIndexOrThrow(URL_KEY));
        Intent resultIntent = new Intent();
        resultIntent.putExtra(URL_KEY, url);
        resultIntent.putExtra(TYPE, mType);
        setResult(Activity.RESULT_OK, resultIntent);
        finish();
    }
}
