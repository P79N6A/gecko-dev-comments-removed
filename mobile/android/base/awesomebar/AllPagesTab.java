




package org.mozilla.gecko;

import android.app.Activity;
import android.content.Context;
import android.widget.ListView;
import android.widget.ImageView;
import android.widget.TextView;
import android.view.View;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.app.Activity;
import android.widget.AdapterView;
import android.database.Cursor;
import android.widget.AdapterView;
import android.util.Log;
import android.text.TextUtils;
import android.widget.Toast;
import android.widget.SimpleCursorAdapter;
import android.widget.LinearLayout;
import android.widget.TabHost.TabContentFactory;
import android.view.ViewGroup;
import android.graphics.drawable.Drawable;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.content.Intent;
import android.widget.FilterQueryProvider;
import android.os.SystemClock;
import android.view.MenuInflater;
import android.widget.TabHost;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.ArrayList;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

import org.mozilla.gecko.AwesomeBar.ContextMenuSubject;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.db.BrowserContract.Combined;

public class AllPagesTab extends AwesomeBarTab {
    public static final String LOGTAG = "ALL_PAGES";
    private static final String TAG = "allPages";
    private SearchEngine mSuggestEngine;
    private ArrayList<SearchEngine> mSearchEngines;
    private ListView mView = null;
    private AwesomeBarCursorAdapter mCursorAdapter = null;

    private class SearchEntryViewHolder {
        public FlowLayout suggestionView;
        public ImageView iconView;
        public LinearLayout userEnteredView;
        public TextView userEnteredTextView;
    }

    public AllPagesTab(Context context) {
        super(context);
        mSearchEngines = new ArrayList<SearchEngine>();
    }

    public boolean onBackPressed() {
        return false;
    }

    public TabContentFactory getFactory() {
        return new TabContentFactory() {
           public View createTabContent(String tag) {
               final ListView list = getListView();
               list.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                   public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                        handleItemClick(parent, view, position, id);
                   }
               });
               return list;
           }
      };
    }

    public int getTitleStringId() {
        return R.string.awesomebar_all_pages_title;
    }

    public String getTag() {
        return TAG;
    }

    public ListView getListView() {
        if (mView == null) {
            mView = new ListView(mContext, null, R.style.AwesomeBarList);
            ((Activity)mContext).registerForContextMenu(mView);
            mView.setTag(TAG);
            AwesomeBarCursorAdapter adapter = getCursorAdapter();
            mView.setAdapter(adapter);
            mView.setOnTouchListener(mListListener);
        }
        return mView;
    }

    public void destroy() {
        AwesomeBarCursorAdapter adapter = getCursorAdapter();
        if (adapter == null) {
            return;
        }

        Cursor cursor = adapter.getCursor();
        if (cursor != null)
            cursor.close();
    }

    public void filter(String searchTerm) {
        AwesomeBarCursorAdapter adapter = getCursorAdapter();
        adapter.filter(searchTerm);
    }

    protected AwesomeBarCursorAdapter getCursorAdapter() {
        if (mCursorAdapter == null) {
            
            mCursorAdapter = new AwesomeBarCursorAdapter(mContext);

            mCursorAdapter.setFilterQueryProvider(new FilterQueryProvider() {
                public Cursor runQuery(CharSequence constraint) {
                    long start = SystemClock.uptimeMillis();
    
                    Cursor c = BrowserDB.filter(getContentResolver(), constraint, MAX_RESULTS);
                    c.getCount();
    
                    long end = SystemClock.uptimeMillis();
                    Log.i(LOGTAG, "Got cursor in " + (end - start) + "ms");
    
                    return c;
                }
            });
        }
        return mCursorAdapter;
    }

    private interface AwesomeBarItem {
        public void onClick();
    }

    private class AwesomeBarCursorAdapter extends SimpleCursorAdapter {
        private String mSearchTerm;

        private static final int ROW_SEARCH = 0;
        private static final int ROW_STANDARD = 1;

        private class AwesomeBarCursorItem implements AwesomeBarItem {
            private Cursor mCursor;

            public AwesomeBarCursorItem(Cursor cursor) {
                mCursor = cursor;
            }

            public void onClick() {
                AwesomeBarTabs.OnUrlOpenListener listener = getUrlListener();
                if (listener == null)
                    return;

                String url = mCursor.getString(mCursor.getColumnIndexOrThrow(URLColumns.URL));

                int display = mCursor.getInt(mCursor.getColumnIndexOrThrow(Combined.DISPLAY));
                if (display == Combined.DISPLAY_READER) {
                    url = getReaderForUrl(url);
                }
                listener.onUrlOpen(url);
            }
        }

        private class AwesomeBarSearchEngineItem implements AwesomeBarItem {
            private String mSearchEngine;

            public AwesomeBarSearchEngineItem(String searchEngine) {
                mSearchEngine = searchEngine;
            }

            public void onClick() {
                AwesomeBarTabs.OnUrlOpenListener listener = getUrlListener();
                if (listener != null)
                    listener.onSearch(mSearchEngine, mSearchTerm);
            }
        }

        public AwesomeBarCursorAdapter(Context context) {
            super(context, -1, null, new String[] {}, new int[] {});
            mSearchTerm = "";
        }

        public void filter(String searchTerm) {
            mSearchTerm = searchTerm;
            getFilter().filter(searchTerm);
        }

        private int getSuggestEngineCount() {
            return (mSearchTerm.length() == 0 || mSuggestEngine == null) ? 0 : 1;
        }

        
        @Override
        public int getCount() {
            final int resultCount = super.getCount();

            
            if (mSearchTerm.length() == 0)
                return resultCount;

            return resultCount + mSearchEngines.size() + getSuggestEngineCount();
        }

        
        
        @Override
        public Object getItem(int position) {
            int engineIndex = getEngineIndex(position);

            if (engineIndex == -1) {
                
                position -= getSuggestEngineCount();
                return new AwesomeBarCursorItem((Cursor) super.getItem(position));
            }

            
            return new AwesomeBarSearchEngineItem(getEngine(engineIndex).name);
        }

        private SearchEngine getEngine(int index) {
            final int suggestEngineCount = getSuggestEngineCount();
            if (index < suggestEngineCount)
                return mSuggestEngine;
            return mSearchEngines.get(index - suggestEngineCount);
        }

        private int getEngineIndex(int position) {
            final int resultCount = super.getCount();
            final int suggestEngineCount = getSuggestEngineCount();

            
            if (position < suggestEngineCount)
                return 0;

            
            if (position - suggestEngineCount < resultCount)
                return -1;

            
            return position - resultCount;
        }

        @Override
        public int getItemViewType(int position) {
            return getEngineIndex(position) == -1 ? ROW_STANDARD : ROW_SEARCH;
        }

        @Override
        public int getViewTypeCount() {
            
            return 2;
        }

        @Override
        public boolean isEnabled(int position) {
            
            
            
            
            int index = getEngineIndex(position);
            if (index != -1) {
                return getEngine(index).suggestions.isEmpty();
            }
            return true;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (getItemViewType(position) == ROW_SEARCH) {
                SearchEntryViewHolder viewHolder = null;

                if (convertView == null) {
                    convertView = getInflater().inflate(R.layout.awesomebar_suggestion_row, null);

                    viewHolder = new SearchEntryViewHolder();
                    viewHolder.suggestionView = (FlowLayout) convertView.findViewById(R.id.suggestion_layout);
                    viewHolder.iconView = (ImageView) convertView.findViewById(R.id.suggestion_icon);
                    viewHolder.userEnteredView = (LinearLayout) convertView.findViewById(R.id.suggestion_user_entered);
                    viewHolder.userEnteredTextView = (TextView) convertView.findViewById(R.id.suggestion_text);

                    convertView.setTag(viewHolder);
                } else {
                    viewHolder = (SearchEntryViewHolder) convertView.getTag();
                }

                bindSearchEngineView(getEngine(getEngineIndex(position)), viewHolder);
            } else {
                AwesomeEntryViewHolder viewHolder = null;

                if (convertView == null) {
                    convertView = getInflater().inflate(R.layout.awesomebar_row, null);

                    viewHolder = new AwesomeEntryViewHolder();
                    viewHolder.titleView = (TextView) convertView.findViewById(R.id.title);
                    viewHolder.urlView = (TextView) convertView.findViewById(R.id.url);
                    viewHolder.faviconView = (ImageView) convertView.findViewById(R.id.favicon);
                    viewHolder.bookmarkIconView = (ImageView) convertView.findViewById(R.id.bookmark_icon);

                    convertView.setTag(viewHolder);
                } else {
                    viewHolder = (AwesomeEntryViewHolder) convertView.getTag();
                }

                position -= getSuggestEngineCount();
                Cursor cursor = getCursor();
                if (!cursor.moveToPosition(position))
                    throw new IllegalStateException("Couldn't move cursor to position " + position);

                updateTitle(viewHolder.titleView, cursor);
                updateUrl(viewHolder.urlView, cursor);
                updateFavicon(viewHolder.faviconView, cursor);
                updateBookmarkIcon(viewHolder.bookmarkIconView, cursor);
            }

            return convertView;
        }

        private void bindSearchEngineView(final SearchEngine engine, SearchEntryViewHolder viewHolder) {
            
            OnClickListener clickListener = new OnClickListener() {
                public void onClick(View v) {
                    AwesomeBarTabs.OnUrlOpenListener listener = getUrlListener();
                    if (listener != null) {
                        String suggestion = ((TextView) v.findViewById(R.id.suggestion_text)).getText().toString();
                        listener.onSearch(engine.name, suggestion);
                    }
                }
            };

            
            OnLongClickListener longClickListener = new OnLongClickListener() {
                public boolean onLongClick(View v) {
                    AwesomeBarTabs.OnUrlOpenListener listener = getUrlListener();
                    if (listener != null) {
                        String suggestion = ((TextView) v.findViewById(R.id.suggestion_text)).getText().toString();
                        listener.onEditSuggestion(suggestion);
                        return true;
                    }
                    return false;
                }
            };

            
            FlowLayout suggestionView = viewHolder.suggestionView;
            viewHolder.iconView.setImageDrawable(engine.icon);

            
            viewHolder.userEnteredTextView.setText(mSearchTerm);
            viewHolder.userEnteredView.setOnClickListener(clickListener);
            
            
            int recycledSuggestionCount = suggestionView.getChildCount();
            int suggestionCount = engine.suggestions.size();
            int i = 0;
            for (i = 0; i < suggestionCount; i++) {
                String suggestion = engine.suggestions.get(i);
                View suggestionItem = null;

                
                if (i+1 < recycledSuggestionCount) {
                    suggestionItem = suggestionView.getChildAt(i+1);
                    suggestionItem.setVisibility(View.VISIBLE);
                } else {
                    suggestionItem = getInflater().inflate(R.layout.awesomebar_suggestion_item, null);
                    ((ImageView) suggestionItem.findViewById(R.id.suggestion_magnifier)).setVisibility(View.GONE);
                    suggestionView.addView(suggestionItem);
                }
                ((TextView) suggestionItem.findViewById(R.id.suggestion_text)).setText(suggestion);

                suggestionItem.setOnClickListener(clickListener);
                suggestionItem.setOnLongClickListener(longClickListener);
            }
            
            
            for (++i; i < recycledSuggestionCount; i++) {
                suggestionView.getChildAt(i).setVisibility(View.GONE);
            }
        }
    };

    private class SearchEngine {
        public String name;
        public Drawable icon;
        public ArrayList<String> suggestions;

        public SearchEngine(String name) {
            this(name, null);
        }

        public SearchEngine(String name, Drawable icon) {
            this.name = name;
            this.icon = icon;
            this.suggestions = new ArrayList<String>();
        }
    };

    




    public void setSuggestEngine(String name, Drawable icon) {
        
        
        
        
        final SearchEngine suggestEngine = new SearchEngine(name, icon);
        if (mSuggestEngine != null)
            suggestEngine.suggestions = mSuggestEngine.suggestions;

        mSuggestEngine = suggestEngine;
    }

    



    public void setSuggestions(final ArrayList<String> suggestions) {
        if (mSuggestEngine != null) {
            mSuggestEngine.suggestions = suggestions;
            getCursorAdapter().notifyDataSetChanged();
        }
    }

    


    public void setSearchEngines(String suggestEngine, JSONArray engines) {
        mSearchEngines = new ArrayList<SearchEngine>();
        for (int i = 0; i < engines.length(); i++) {
            try {
                JSONObject engineJSON = engines.getJSONObject(i);
                String name = engineJSON.getString("name");
                String iconURI = engineJSON.getString("iconURI");
                Drawable icon = getDrawableFromDataURI(iconURI);
                if (name.equals(suggestEngine)) {
                    setSuggestEngine(name, icon);
                } else {
                    mSearchEngines.add(new SearchEngine(name, icon));
                }
            } catch (JSONException e) {
                Log.e(LOGTAG, "Error getting search engine JSON", e);
                return;
            }
        }
        AwesomeBarCursorAdapter adapter = getCursorAdapter();
        if (adapter != null)
            adapter.notifyDataSetChanged();
    }

    private Drawable getDrawableFromDataURI(String dataURI) {
        String base64 = dataURI.substring(dataURI.indexOf(',') + 1);
        Drawable drawable = null;
        try {
            byte[] bytes = GeckoAppShell.decodeBase64(base64, GeckoAppShell.BASE64_DEFAULT);
            ByteArrayInputStream stream = new ByteArrayInputStream(bytes);
            drawable = Drawable.createFromStream(stream, "src");
            stream.close();
        } catch (IllegalArgumentException e) {
            Log.i(LOGTAG, "exception while decoding drawable: " + base64, e);
        } catch (IOException e) { }
        return drawable;
    }

    public void handleItemClick(AdapterView<?> parent, View view, int position, long id) {
        ListView listview = getListView();
        if (listview == null)
            return;

        AwesomeBarItem item = (AwesomeBarItem)listview.getItemAtPosition(position);
        item.onClick();
    }

    protected void updateBookmarkIcon(ImageView bookmarkIconView, Cursor cursor) {
        int bookmarkIdIndex = cursor.getColumnIndexOrThrow(Combined.BOOKMARK_ID);
        long id = cursor.getLong(bookmarkIdIndex);

        int displayIndex = cursor.getColumnIndexOrThrow(Combined.DISPLAY);
        int display = cursor.getInt(displayIndex);

        
        
        int visibility = (id == 0 ? View.GONE : View.VISIBLE);
        bookmarkIconView.setVisibility(visibility);

        if (display == Combined.DISPLAY_READER) {
            bookmarkIconView.setImageResource(R.drawable.ic_awesomebar_reader);
        } else {
            bookmarkIconView.setImageResource(R.drawable.ic_awesomebar_star);
        }
    }

    public ContextMenuSubject getSubject(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
        ContextMenuSubject subject = null;

        if (!(menuInfo instanceof AdapterView.AdapterContextMenuInfo)) {
            Log.e(LOGTAG, "menuInfo is not AdapterContextMenuInfo");
            return subject;
        }

        ListView list = (ListView)view;
        AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;
        Object selectedItem = list.getItemAtPosition(info.position);

        if (!(selectedItem instanceof Cursor)) {
            Log.e(LOGTAG, "item at " + info.position + " is not a Cursor");
            return subject;
        }

        Cursor cursor = (Cursor) selectedItem;

        
        String keyword = null;
        int keywordCol = cursor.getColumnIndex(URLColumns.KEYWORD);
        if (keywordCol != -1)
            keyword = cursor.getString(keywordCol);

        
        int id = cursor.getInt(cursor.getColumnIndexOrThrow(Combined._ID));

        subject = new ContextMenuSubject(id,
                                        cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL)),
                                        cursor.getBlob(cursor.getColumnIndexOrThrow(URLColumns.FAVICON)),
                                        cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.TITLE)),
                                        keyword);

        if (subject == null)
            return subject;

        MenuInflater inflater = new MenuInflater(mContext);
        inflater.inflate(R.menu.awesomebar_contextmenu, menu);
        menu.findItem(R.id.remove_bookmark).setVisible(false);
        menu.findItem(R.id.edit_bookmark).setVisible(false);

        
        if (subject.id < 0)
            menu.findItem(R.id.remove_history).setVisible(false);

        menu.setHeaderTitle(subject.title);
        return subject;
    }
}
