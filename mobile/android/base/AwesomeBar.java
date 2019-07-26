




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.util.GeckoAsyncTask;
import org.mozilla.gecko.util.StringUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputType;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TabWidget;
import android.widget.Toast;

import java.net.URLEncoder;
import java.util.Arrays;
import java.util.Collection;

public class AwesomeBar extends GeckoActivity {
    private static final String LOGTAG = "GeckoAwesomeBar";

    private static final Collection<String> sSwypeInputMethods = Arrays.asList(new String[] {
                                                                 InputMethods.METHOD_SWYPE,
                                                                 InputMethods.METHOD_SWYPE_BETA,
                                                                 });

    static final String URL_KEY = "url";
    static final String CURRENT_URL_KEY = "currenturl";
    static final String TARGET_KEY = "target";
    static final String SEARCH_KEY = "search";
    static final String TITLE_KEY = "title";
    static final String USER_ENTERED_KEY = "user_entered";
    static final String READING_LIST_KEY = "reading_list";
    public static enum Target { NEW_TAB, CURRENT_TAB, PICK_SITE };

    private String mTarget;
    private AwesomeBarTabs mAwesomeTabs;
    private CustomEditText mText;
    private ImageButton mGoButton;
    private ContextMenuSubject mContextMenuSubject;
    private boolean mIsUsingSwype;
    private boolean mDelayRestartInput;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.d(LOGTAG, "creating awesomebar");

        LayoutInflater.from(this).setFactory(GeckoViewsFactory.getInstance());
        setContentView(R.layout.awesomebar);

        mGoButton = (ImageButton) findViewById(R.id.awesomebar_button);
        mText = (CustomEditText) findViewById(R.id.awesomebar_text);

        TabWidget tabWidget = (TabWidget) findViewById(android.R.id.tabs);
        tabWidget.setDividerDrawable(null);

        mAwesomeTabs = (AwesomeBarTabs) findViewById(R.id.awesomebar_tabs);
        mAwesomeTabs.setOnUrlOpenListener(new AwesomeBarTabs.OnUrlOpenListener() {
            public void onUrlOpen(String url, String title) {
                openUrlAndFinish(url, title, false);
            }

            public void onSearch(String engine, String text) {
                openSearchAndFinish(text, engine);
            }

            public void onEditSuggestion(final String text) {
                GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    public void run() {
                        mText.setText(text);
                        mText.setSelection(mText.getText().length());
                        mText.requestFocus();
                        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                        imm.showSoftInput(mText, InputMethodManager.SHOW_IMPLICIT);
                    }
                });
            }
        });

        mGoButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                openUserEnteredAndFinish(mText.getText().toString());
            }
        });

        Intent intent = getIntent();
        String currentUrl = intent.getStringExtra(CURRENT_URL_KEY);
        if (currentUrl != null) {
            mText.setText(currentUrl);
            mText.selectAll();
        }

        mTarget = intent.getStringExtra(TARGET_KEY);
        if (mTarget.equals(Target.CURRENT_TAB.name())) {
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null && tab.isPrivate()) {
                BrowserToolbarBackground mAddressBarBg = (BrowserToolbarBackground) findViewById(R.id.address_bar_bg);
                mAddressBarBg.setPrivateMode(true);

                TabsButton mTabs = (TabsButton) findViewById(R.id.dummy_tab);
                if (mTabs != null)
                    mTabs.setPrivateMode(true);

                mText.setPrivateMode(true);
            }
        }
        mAwesomeTabs.setTarget(mTarget);

        mText.setOnKeyPreImeListener(new CustomEditText.OnKeyPreImeListener() {
            public boolean onKeyPreIme(View v, int keyCode, KeyEvent event) {
                
                if (event.getAction() != KeyEvent.ACTION_DOWN)
                    return false;

                if (keyCode == KeyEvent.KEYCODE_ENTER) {
                    
                    
                    Editable content = mText.getText();
                    if (!hasCompositionString(content)) {
                        openUserEnteredAndFinish(content.toString());
                        return true;
                    }
                }

                
                
                InputMethodManager imm =
                        (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                if (keyCode == KeyEvent.KEYCODE_BACK && !imm.isFullscreenMode()) {
                    
                    
                    if (mAwesomeTabs.onBackPressed())
                        return true;

                    
                    
                    cancelAndFinish();
                    return true;
                }

                return false;
            }
        });

        mText.addTextChangedListener(new TextWatcher() {
            public void afterTextChanged(Editable s) {
                String text = s.toString();
                mAwesomeTabs.filter(text);

                
                
                if (!hasCompositionString(s)) {
                    updateGoButton(text);
                }

                if (Build.VERSION.SDK_INT >= 11) {
                    getActionBar().hide();
                }
            }

            public void beforeTextChanged(CharSequence s, int start, int count,
                                          int after) {
                
            }

            public void onTextChanged(CharSequence s, int start, int before,
                                      int count) {
                
            }
        });

        mText.setOnKeyListener(new View.OnKeyListener() {
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_ENTER) {
                    if (event.getAction() != KeyEvent.ACTION_DOWN)
                        return true;

                    openUserEnteredAndFinish(mText.getText().toString());
                    return true;
                } else {
                    return false;
                }
            }
        });

        mText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            public void onFocusChange(View v, boolean hasFocus) {
                if (v == null || hasFocus) {
                    return;
                }

                InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                try {
                    imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
                } catch (NullPointerException e) {
                    Log.e(LOGTAG, "InputMethodManagerService, why are you throwing"
                                  + " a NullPointerException? See bug 782096", e);
                }
            }
        });

        mText.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                if (Build.VERSION.SDK_INT >= 11) {
                    CustomEditText text = (CustomEditText) v;

                    if (text.getSelectionStart() == text.getSelectionEnd())
                        return false;

                    getActionBar().show();
                    return false;
                }

                return false;
            }
        });

        mText.setOnSelectionChangedListener(new CustomEditText.OnSelectionChangedListener() {
            @Override
            public void onSelectionChanged(int selStart, int selEnd) {
                if (Build.VERSION.SDK_INT >= 11 && selStart == selEnd) {
                    getActionBar().hide();
                }
            }
        });

        boolean showReadingList = intent.getBooleanExtra(READING_LIST_KEY, false);
        if (showReadingList) {
            BookmarksTab bookmarksTab = mAwesomeTabs.getBookmarksTab();
            bookmarksTab.setShowReadingList(true);
            mAwesomeTabs.setCurrentTabByTag(bookmarksTab.getTag());
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        
        
        
        
        

        if (!hasFocus)
            return;

        boolean wasUsingSwype = mIsUsingSwype;
        mIsUsingSwype = sSwypeInputMethods.contains(InputMethods.getCurrentInputMethod(this));

        if (mIsUsingSwype == wasUsingSwype)
            return;

        int currentInputType = mText.getInputType();
        int newInputType = mIsUsingSwype
                           ? (currentInputType & ~InputType.TYPE_TEXT_VARIATION_URI)    
                           : (currentInputType | InputType.TYPE_TEXT_VARIATION_URI);    

        mText.setRawInputType(newInputType);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfiguration) {
        super.onConfigurationChanged(newConfiguration);
    }

    @Override
    public boolean onSearchRequested() {
        cancelAndFinish();
        return true;
    }

    private void updateGoButton(String text) {
        if (text.length() == 0) {
            mGoButton.setVisibility(View.GONE);
            return;
        }

        mGoButton.setVisibility(View.VISIBLE);

        int imageResource = R.drawable.ic_awesomebar_go;
        String contentDescription = getString(R.string.go);
        int imeAction = EditorInfo.IME_ACTION_GO;

        int actionBits = mText.getImeOptions() & EditorInfo.IME_MASK_ACTION;
        if (StringUtils.isSearchQuery(text, actionBits == EditorInfo.IME_ACTION_SEARCH)) {
            imageResource = R.drawable.ic_awesomebar_search;
            contentDescription = getString(R.string.search);
            imeAction = EditorInfo.IME_ACTION_SEARCH;
        }
        mGoButton.setImageResource(imageResource);
        mGoButton.setContentDescription(contentDescription);

        InputMethodManager imm = InputMethods.getInputMethodManager(mText.getContext());
        if (imm == null) {
            return;
        }
        if (actionBits != imeAction) {
            int optionBits = mText.getImeOptions() & ~EditorInfo.IME_MASK_ACTION;
            mText.setImeOptions(optionBits | imeAction);

            mDelayRestartInput = (imeAction == EditorInfo.IME_ACTION_GO) &&
                                 (InputMethods.shouldDelayAwesomebarUpdate(mText.getContext()));
            if (!mDelayRestartInput) {
                imm.restartInput(mText);
            }
        } else if (mDelayRestartInput) {
            
            
            
            mDelayRestartInput = false;
            imm.restartInput(mText);
        }
    }

    private void cancelAndFinish() {
        setResult(Activity.RESULT_CANCELED);
        finish();
        overridePendingTransition(R.anim.awesomebar_hold_still, R.anim.awesomebar_fade_out);
    }

    private void finishWithResult(Intent intent) {
        setResult(Activity.RESULT_OK, intent);
        finish();
        overridePendingTransition(R.anim.awesomebar_hold_still, R.anim.awesomebar_fade_out);
    }

    private void openUrlAndFinish(String url) {
        openUrlAndFinish(url, null, false);
    }

    private void openUrlAndFinish(String url, String title, boolean userEntered) {
        Intent resultIntent = new Intent();
        resultIntent.putExtra(URL_KEY, url);
        if (title != null && !TextUtils.isEmpty(title))
            resultIntent.putExtra(TITLE_KEY, title);
        if (userEntered)
            resultIntent.putExtra(USER_ENTERED_KEY, userEntered);
        resultIntent.putExtra(TARGET_KEY, mTarget);
        finishWithResult(resultIntent);
    }

    private void openUserEnteredAndFinish(String url) {
        int index = url.indexOf(' ');
        String keywordUrl = null;
        String keywordSearch = null;

        if (index == -1) {
            keywordUrl = BrowserDB.getUrlForKeyword(getContentResolver(), url);
            keywordSearch = "";
        } else {
            keywordUrl = BrowserDB.getUrlForKeyword(getContentResolver(), url.substring(0, index));
            keywordSearch = url.substring(index + 1);
        }

        if (keywordUrl != null) {
            String search = URLEncoder.encode(keywordSearch);
            url = keywordUrl.replace("%s", search);
        }

        openUrlAndFinish(url, "", true);
    }

    private void openSearchAndFinish(String url, String engine) {
        Intent resultIntent = new Intent();
        resultIntent.putExtra(URL_KEY, url);
        resultIntent.putExtra(TARGET_KEY, mTarget);
        resultIntent.putExtra(SEARCH_KEY, engine);
        finishWithResult(resultIntent);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        
        
        if (keyCode > KeyEvent.getMaxKeyCode())
            return true;

        
        
        if (keyCode == KeyEvent.KEYCODE_BACK ||
            keyCode == KeyEvent.KEYCODE_MENU ||
            keyCode == KeyEvent.KEYCODE_DPAD_UP ||
            keyCode == KeyEvent.KEYCODE_DPAD_DOWN ||
            keyCode == KeyEvent.KEYCODE_DPAD_LEFT ||
            keyCode == KeyEvent.KEYCODE_DPAD_RIGHT ||
            keyCode == KeyEvent.KEYCODE_DPAD_CENTER ||
            keyCode == KeyEvent.KEYCODE_DEL ||
            keyCode == KeyEvent.KEYCODE_VOLUME_UP ||
            keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
            return super.onKeyDown(keyCode, event);
        } else if (keyCode == KeyEvent.KEYCODE_SEARCH) {
             mText.setText("");
             mText.requestFocus();
             InputMethodManager imm = (InputMethodManager) mText.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
             imm.showSoftInput(mText, InputMethodManager.SHOW_IMPLICIT);
             return true;
        } else {
            int prevSelStart = mText.getSelectionStart();
            int prevSelEnd = mText.getSelectionEnd();

            
            
            mText.dispatchKeyEvent(event);

            int curSelStart = mText.getSelectionStart();
            int curSelEnd = mText.getSelectionEnd();
            if (prevSelStart != curSelStart || prevSelEnd != curSelEnd) {
                mText.requestFocusFromTouch();
                
                mText.setSelection(curSelStart, curSelEnd);
            }
            return true;
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mText != null && mText.getText() != null) {
            updateGoButton(mText.getText().toString());
            if (mDelayRestartInput) {
                
                updateGoButton(mText.getText().toString());
            }
        }

        
        
        BrowserDB.invalidateCachedState();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mAwesomeTabs.destroy();
    }

    @Override
    public void onBackPressed() {
        
        
        if (mAwesomeTabs.onBackPressed())
            return;

        
        cancelAndFinish();
    }

    static public class ContextMenuSubject {
        public int id;
        public String url;
        public byte[] favicon;
        public String title;
        public String keyword;
        public int display;

        public ContextMenuSubject(int id, String url, byte[] favicon, String title, String keyword) {
            this(id, url, favicon, title, keyword, Combined.DISPLAY_NORMAL);
        }

        public ContextMenuSubject(int id, String url, byte[] favicon, String title, String keyword, int display) {
            this.id = id;
            this.url = url;
            this.favicon = favicon;
            this.title = title;
            this.keyword = keyword;
            this.display = display;
        }
    };

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, view, menuInfo);
        ListView list = (ListView) view;
        AwesomeBarTab tab = mAwesomeTabs.getAwesomeBarTabForView(view);
        mContextMenuSubject = tab.getSubject(menu, view, menuInfo);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        if (mContextMenuSubject == null)
            return false;

        final int id = mContextMenuSubject.id;
        final String url = mContextMenuSubject.url;
        final byte[] b = mContextMenuSubject.favicon;
        final String title = mContextMenuSubject.title;
        final String keyword = mContextMenuSubject.keyword;
        final int display = mContextMenuSubject.display;

        switch (item.getItemId()) {
            case R.id.open_new_tab:
            case R.id.open_private_tab: {
                if (url == null) {
                    Log.e(LOGTAG, "Can't open in new tab because URL is null");
                    break;
                }

                String newTabUrl = url;
                if (display == Combined.DISPLAY_READER)
                    newTabUrl = ReaderModeUtils.getAboutReaderForUrl(url, true);

                int flags = Tabs.LOADURL_NEW_TAB;
                if (item.getItemId() == R.id.open_private_tab)
                    flags |= Tabs.LOADURL_PRIVATE;

                Tabs.getInstance().loadUrl(newTabUrl, flags);
                Toast.makeText(this, R.string.new_tab_opened, Toast.LENGTH_SHORT).show();
                break;
            }
            case R.id.open_in_reader: {
                if (url == null) {
                    Log.e(LOGTAG, "Can't open in reader mode because URL is null");
                    break;
                }

                openUrlAndFinish(ReaderModeUtils.getAboutReaderForUrl(url, true));
                break;
            }
            case R.id.edit_bookmark: {
                AlertDialog.Builder editPrompt = new AlertDialog.Builder(this);
                View editView = getLayoutInflater().inflate(R.layout.bookmark_edit, null);
                editPrompt.setTitle(R.string.bookmark_edit_title);
                editPrompt.setView(editView);

                final EditText nameText = ((EditText) editView.findViewById(R.id.edit_bookmark_name));
                final EditText locationText = ((EditText) editView.findViewById(R.id.edit_bookmark_location));
                final EditText keywordText = ((EditText) editView.findViewById(R.id.edit_bookmark_keyword));
                nameText.setText(title);
                locationText.setText(url);
                keywordText.setText(keyword);

                editPrompt.setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        (new GeckoAsyncTask<Void, Void, Void>(GeckoApp.mAppContext, GeckoAppShell.getHandler()) {
                            @Override
                            public Void doInBackground(Void... params) {
                                String newUrl = locationText.getText().toString().trim();
                                BrowserDB.updateBookmark(getContentResolver(), id, newUrl, nameText.getText().toString(),
                                                         keywordText.getText().toString());
                                return null;
                            }

                            @Override
                            public void onPostExecute(Void result) {
                                Toast.makeText(AwesomeBar.this, R.string.bookmark_updated, Toast.LENGTH_SHORT).show();
                            }
                        }).execute();
                    }
                });

                editPrompt.setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener() {
                      public void onClick(DialogInterface dialog, int whichButton) {
                          
                      }
                });

                final AlertDialog dialog = editPrompt.create();

                
                locationText.addTextChangedListener(new TextWatcher() {
                    private boolean mEnabled = true;

                    public void afterTextChanged(Editable s) {}

                    public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                        boolean enabled = (s.toString().trim().length() > 0);
                        if (mEnabled != enabled) {
                            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(enabled);
                            mEnabled = enabled;
                        }
                    }
                });

                dialog.show();
                break;
            }
            case R.id.remove_bookmark: {
                (new GeckoAsyncTask<Void, Void, Void>(GeckoApp.mAppContext, GeckoAppShell.getHandler()) {
                    private boolean mInReadingList;

                    @Override
                    public void onPreExecute() {
                        mInReadingList = mAwesomeTabs.isInReadingList();
                    }

                    @Override
                    public Void doInBackground(Void... params) {
                        BrowserDB.removeBookmark(getContentResolver(), id);
                        return null;
                    }

                    @Override
                    public void onPostExecute(Void result) {
                        int messageId = R.string.bookmark_removed;
                        if (mInReadingList) {
                            messageId = R.string.reading_list_removed;

                            GeckoEvent e = GeckoEvent.createBroadcastEvent("Reader:Remove", url);
                            GeckoAppShell.sendEventToGecko(e);
                        }

                        Toast.makeText(AwesomeBar.this, messageId, Toast.LENGTH_SHORT).show();
                    }
                }).execute();
                break;
            }
            case R.id.remove_history: {
                (new GeckoAsyncTask<Void, Void, Void>(GeckoApp.mAppContext, GeckoAppShell.getHandler()) {
                    @Override
                    public Void doInBackground(Void... params) {
                        BrowserDB.removeHistoryEntry(getContentResolver(), id);
                        return null;
                    }

                    @Override
                    public void onPostExecute(Void result) {
                        Toast.makeText(AwesomeBar.this, R.string.history_removed, Toast.LENGTH_SHORT).show();
                    }
                }).execute();
                break;
            }
            case R.id.add_to_launcher: {
                if (url == null) {
                    Log.e(LOGTAG, "Can't add to home screen because URL is null");
                    break;
                }

                Bitmap bitmap = null;
                if (b != null)
                    bitmap = BitmapFactory.decodeByteArray(b, 0, b.length);

                String shortcutTitle = TextUtils.isEmpty(title) ? url.replaceAll("^([a-z]+://)?(www\\.)?", "") : title;
                GeckoAppShell.createShortcut(shortcutTitle, url, bitmap, "");
                break;
            }
            case R.id.share: {
                if (url == null) {
                    Log.e(LOGTAG, "Can't share because URL is null");
                    break;
                }

                GeckoAppShell.openUriExternal(url, "text/plain", "", "",
                                              Intent.ACTION_SEND, title);
                break;
            }
            default: {
                return super.onContextItemSelected(item);
            }
        }
        return true;
    }

    public static String getReaderForUrl(String url) {
        
        
        return "about:reader?url=" + Uri.encode(url) + "&readingList=1";
    }

    private static boolean hasCompositionString(Editable content) {
        Object[] spans = content.getSpans(0, content.length(), Object.class);
        if (spans != null) {
            for (Object span : spans) {
                if ((content.getSpanFlags(span) & Spanned.SPAN_COMPOSING) != 0) {
                    
                    return true;
                }
            }
        }
        return false;
    }
}
