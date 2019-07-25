




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.util.GeckoAsyncTask;
import org.mozilla.gecko.util.GeckoBackgroundThread;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.content.SharedPreferences;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.Toast;

import java.io.InputStream;
import java.net.URL;
import java.util.EnumSet;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

abstract public class BrowserApp extends GeckoApp
                                 implements TabsPanel.TabsLayoutChangeListener,
                                            PropertyAnimator.PropertyAnimationListener {
    private static final String LOGTAG = "GeckoBrowserApp";

    public static BrowserToolbar mBrowserToolbar;
    private AboutHomeContent mAboutHomeContent;

    static Vector<MenuItem> sAddonMenuItems = new Vector<MenuItem>();

    private PropertyAnimator mMainLayoutAnimator;

    private FindInPageBar mFindInPageBar;

    
    private static final int FEEDBACK_LAUNCH_COUNT = 10;

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch(msg) {
            case LOCATION_CHANGE:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    String url = tab.getURL();
                    if (url.equals("about:home"))
                        showAboutHome();
                    else 
                        hideAboutHome();
                    maybeCancelFaviconLoad(tab);
                }
                break;
            case LOAD_ERROR:
            case START:
            case STOP:
            case MENU_UPDATED:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    invalidateOptionsMenu();
                }
                break;
            case SELECTED:
                if ("about:home".equals(tab.getURL()))
                    showAboutHome();
                else
                    hideAboutHome();
                break;
        }
        super.onTabChanged(tab, msg, data);
    }

    @Override
    void handlePageShow(final int tabId) {
        super.handlePageShow(tabId);
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        mMainHandler.post(new Runnable() {
            public void run() {
                loadFavicon(tab);
            }
        });
    }

    @Override
    void handleLinkAdded(final int tabId, String rel, final String href, int size) {
        super.handleLinkAdded(tabId, rel, href, size);
        if (rel.indexOf("[icon]") == -1)
            return;

        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        
        
        
        
        if (tab.getState() != Tab.STATE_LOADING) {
            mMainHandler.post(new Runnable() {
                public void run() {
                    loadFavicon(tab);
                }
            });
        }
    }

    @Override
    void handleClearHistory() {
        updateAboutHomeTopSites();
        super.handleClearHistory();
    }

    @Override
    void handleSecurityChange(final int tabId, final JSONObject identityData) {
        super.handleSecurityChange(tabId, identityData);
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        mMainHandler.post(new Runnable() { 
            public void run() {
                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setSecurityMode(tab.getSecurityMode());
            }
        });
    }

    void handleReaderEnabled(final int tabId) {
        super.handleReaderEnabled(tabId);
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        mMainHandler.post(new Runnable() {
            public void run() {
                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setReaderMode(tab.getReaderEnabled());
            }
        });
    }

    @Override
    void onStatePurged() {
        mMainHandler.post(new Runnable() {
            public void run() {
                if (mAboutHomeContent != null)
                    mAboutHomeContent.setLastTabsVisibility(false);
            }
        });

        super.onStatePurged();
    }

    @Override
    protected void loadRequest(String url, AwesomeBar.Target target, String searchEngine, boolean userEntered) {
        mBrowserToolbar.setTitle(url);
        super.loadRequest(url, target, searchEngine, userEntered);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LinearLayout actionBar = (LinearLayout) getActionBarLayout();
        mMainLayout.addView(actionBar, 0);

        mBrowserToolbar = new BrowserToolbar(this);
        mBrowserToolbar.from(actionBar);

        if (mTabsPanel != null)
            mTabsPanel.setTabsLayoutChangeListener(this);

        mFindInPageBar = (FindInPageBar) findViewById(R.id.find_in_page);

        if (savedInstanceState != null) {
            mBrowserToolbar.setTitle(savedInstanceState.getString(SAVED_STATE_TITLE));
        }

        registerEventListener("Feedback:LastUrl");
        registerEventListener("Feedback:OpenPlayStore");
        registerEventListener("Feedback:MaybeLater");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mAboutHomeContent != null)
            mAboutHomeContent.onDestroy();

        unregisterEventListener("Feedback:LastUrl");
        unregisterEventListener("Feedback:OpenPlayStore");
        unregisterEventListener("Feedback:MaybeLater");
    }

    @Override
    public void onContentChanged() {
        super.onContentChanged();
        if (mAboutHomeContent != null)
            mAboutHomeContent.onActivityContentChanged();
    }

    @Override
    protected void finishProfileMigration() {
        
        updateAboutHomeTopSites();

        super.finishProfileMigration();
    }

    
    
    @Override void initializeChrome(String uri, Boolean isExternalURL) {
        mBrowserToolbar.updateBackButton(false);
        mBrowserToolbar.updateForwardButton(false);

        Intent intent = getIntent();
        String action = intent.getAction();
        String args = intent.getStringExtra("args");
        if (args != null && args.contains("-profile")) {
            Pattern p = Pattern.compile("(?:-profile\\s*)(\\w*)(\\s*)");
            Matcher m = p.matcher(args);
            if (m.find()) {
                mBrowserToolbar.setTitle(null);
            }
        }

        if (uri != null && uri.length() > 0) {
            mBrowserToolbar.setTitle(uri);
        }

        if (!isExternalURL) {
            
            if (mRestoreMode == GeckoAppShell.RESTORE_NONE) {
                mBrowserToolbar.updateTabCount(1);
                showAboutHome();
            }
        } else {
            mBrowserToolbar.updateTabCount(1);
        }

        mBrowserToolbar.setProgressVisibility(isExternalURL || (mRestoreMode != GeckoAppShell.RESTORE_NONE));

        mDoorHangerPopup = new DoorHangerPopup(this, mBrowserToolbar.mFavicon);
    }

    void toggleChrome(final boolean aShow) {
        mMainHandler.post(new Runnable() {
            public void run() {
                if (aShow) {
                    mBrowserToolbar.show();
                } else {
                    mBrowserToolbar.hide();
                }
            }
        });

        super.toggleChrome(aShow);
    }

    @Override
    void focusChrome() {
        mMainHandler.post(new Runnable() {
            public void run() {
                mBrowserToolbar.setVisibility(View.VISIBLE);
                mBrowserToolbar.requestFocusFromTouch();
            }
        });
    }

    @Override
    public void refreshChrome() {
        
        if (Build.VERSION.SDK_INT >= 14 && !isTablet()) {
            int index = mMainLayout.indexOfChild(mBrowserToolbar.getLayout());
            mMainLayout.removeViewAt(index);

            LinearLayout actionBar = (LinearLayout) getActionBarLayout();
            mMainLayout.addView(actionBar, index);
            mBrowserToolbar.from(actionBar);
            mBrowserToolbar.refresh();

            
            if (mDoorHangerPopup != null)
                mDoorHangerPopup.setAnchor(mBrowserToolbar.mFavicon);
        }

        invalidateOptionsMenu();
        mTabsPanel.refresh();

        if (mAboutHomeContent != null)
            mAboutHomeContent.refresh();
    }

    public View getActionBarLayout() {
        int actionBarRes;

        if (!hasPermanentMenuKey() || isTablet())
           actionBarRes = R.layout.browser_toolbar_menu;
        else
           actionBarRes = R.layout.browser_toolbar;

        LinearLayout actionBar = (LinearLayout) LayoutInflater.from(this).inflate(actionBarRes, null);
        actionBar.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT,
                                                                (int) getResources().getDimension(R.dimen.browser_toolbar_height)));
        return actionBar;
    }

    @Override
    public boolean hasTabsSideBar() {
        return (mTabsPanel != null && mTabsPanel.isSideBar());
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Menu:Add")) {
                final String label = message.getString("name");
                final int id = message.getInt("id");
                String iconRes = null;
                try { 
                    iconRes = message.getString("icon");
                } catch (Exception ex) { }
                final String icon = iconRes;
                mMainHandler.post(new Runnable() {
                    public void run() {
                        addAddonMenuItem(id, label, icon);
                    }
                });
            } else if (event.equals("Menu:Remove")) {
                final int id = message.getInt("id");
                mMainHandler.post(new Runnable() {
                    public void run() {
                        removeAddonMenuItem(id);
                    }
                });
            } else if (event.equals("Feedback:OpenPlayStore")) {
                Intent intent = new Intent(Intent.ACTION_VIEW);
                intent.setData(Uri.parse("market://details?id=" + getPackageName()));
                startActivity(intent);
            } else if (event.equals("Feedback:MaybeLater")) {
                resetFeedbackLaunchCount();
            } else if (event.equals("Feedback:LastUrl")) {
                getLastUrl();
            } else {
                super.handleMessage(event, message);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    void addTab() {
        showAwesomebar(AwesomeBar.Target.NEW_TAB);
    }

    public void showLocalTabs() {
        showTabs(TabsPanel.Panel.LOCAL_TABS);
    }

    public void showRemoteTabs() {
        showTabs(TabsPanel.Panel.REMOTE_TABS);
    }

    private void showTabs(TabsPanel.Panel panel) {
        if (!sIsGeckoReady)
            return;

        mTabsPanel.show(panel);
    }

    public void hideTabs() {
        mTabsPanel.hide();
    }

    public boolean autoHideTabs() {
        if (!hasTabsSideBar() && areTabsShown()) {
            hideTabs();
            return true;
        }
        return false;
    }

    public boolean areTabsShown() {
        return mTabsPanel.isShown();
    }

    @Override
    public void onTabsLayoutChange(int width, int height) {
        if (mMainLayoutAnimator != null)
            mMainLayoutAnimator.stop();

        if (mTabsPanel.isShown())
            mTabsPanel.setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);

        mMainLayoutAnimator = new PropertyAnimator(150);
        mMainLayoutAnimator.setPropertyAnimationListener(this);

        if (hasTabsSideBar()) {
            mMainLayoutAnimator.attach(mBrowserToolbar.getLayout(),
                                       PropertyAnimator.Property.SHRINK_LEFT,
                                       width);

            
            if (!mTabsPanel.isShown()) {
                ((LinearLayout.LayoutParams) mGeckoLayout.getLayoutParams()).setMargins(0, 0, 0, 0);
                mGeckoLayout.scrollTo(mTabsPanel.getWidth() * -1, 0);
                mGeckoLayout.requestLayout();
            }

            mMainLayoutAnimator.attach(mGeckoLayout,
                                       PropertyAnimator.Property.SLIDE_LEFT,
                                       width);

        } else {
            mMainLayoutAnimator.attach(mMainLayout,
                                       PropertyAnimator.Property.SLIDE_TOP,
                                       height);
        }

        mMainLayoutAnimator.start();
    }

    @Override
    public void onPropertyAnimationStart() {
        mMainHandler.post(new Runnable() {
            public void run() {
                mBrowserToolbar.updateTabs(true);
            }
        });
    }

    @Override
    public void onPropertyAnimationEnd() {
        mMainHandler.post(new Runnable() {
            public void run() {
                if (hasTabsSideBar() && mTabsPanel.isShown()) {
                    
                    ((LinearLayout.LayoutParams) mGeckoLayout.getLayoutParams()).setMargins(mTabsPanel.getWidth(), 0, 0, 0);
                    mGeckoLayout.scrollTo(0, 0);
                    mGeckoLayout.requestLayout();
                }

                if (!mTabsPanel.isShown()) {
                    mBrowserToolbar.updateTabs(false);
                    mTabsPanel.setDescendantFocusability(ViewGroup.FOCUS_BLOCK_DESCENDANTS);
                }
            }
        });
    }

    
    private void loadFavicon(final Tab tab) {
        maybeCancelFaviconLoad(tab);

        long id = getFavicons().loadFavicon(tab.getURL(), tab.getFaviconURL(),
                        new Favicons.OnFaviconLoadedListener() {

            public void onFaviconLoaded(String pageUrl, Drawable favicon) {
                
                
                if (favicon == null)
                    return;

                Log.i(LOGTAG, "Favicon successfully loaded for URL = " + pageUrl);

                
                
                if (!tab.getURL().equals(pageUrl))
                    return;

                Log.i(LOGTAG, "Favicon is for current URL = " + pageUrl);

                tab.updateFavicon(favicon);
                tab.setFaviconLoadId(Favicons.NOT_LOADING);

                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setFavicon(tab.getFavicon());

                Tabs.getInstance().notifyListeners(tab, Tabs.TabEvents.FAVICON);
            }
        });

        tab.setFaviconLoadId(id);
    }

    private void maybeCancelFaviconLoad(Tab tab) {
        long faviconLoadId = tab.getFaviconLoadId();

        if (faviconLoadId == Favicons.NOT_LOADING)
            return;

        
        getFavicons().cancelFaviconLoad(faviconLoadId);

        
        tab.setFaviconLoadId(Favicons.NOT_LOADING);
    }


    
    void updateAboutHomeTopSites() {
        if (mAboutHomeContent == null)
            return;

        mAboutHomeContent.update(EnumSet.of(AboutHomeContent.UpdateFlags.TOP_SITES));
    }

    public void showAboutHome() {
        Runnable r = new AboutHomeRunnable(true);
        mMainHandler.postAtFrontOfQueue(r);
    }

    public void hideAboutHome() {
        Runnable r = new AboutHomeRunnable(false);
        mMainHandler.postAtFrontOfQueue(r);
    }

    public class AboutHomeRunnable implements Runnable {
        boolean mShow;
        AboutHomeRunnable(boolean show) {
            mShow = show;
        }

        public void run() {
            mFormAssistPopup.hide();
            if (mShow) {
                if (mAboutHomeContent == null) {
                    mAboutHomeContent = (AboutHomeContent) findViewById(R.id.abouthome_content);
                    mAboutHomeContent.init();
                    mAboutHomeContent.update(AboutHomeContent.UpdateFlags.ALL);
                    mAboutHomeContent.setUriLoadCallback(new AboutHomeContent.UriLoadCallback() {
                        public void callback(String url) {
                            mBrowserToolbar.setProgressVisibility(true);
                            loadUrl(url, AwesomeBar.Target.CURRENT_TAB);
                        }
                    });
                    mAboutHomeContent.setOnInterceptTouchListener(new ContentTouchListener());
                } else {
                    mAboutHomeContent.update(EnumSet.of(AboutHomeContent.UpdateFlags.TOP_SITES,
                                                        AboutHomeContent.UpdateFlags.REMOTE_TABS));
                }
            
                mAboutHomeContent.setVisibility(View.VISIBLE);
            } else {
                findViewById(R.id.abouthome_content).setVisibility(View.GONE);
            }
        } 
    }

    private void addAddonMenuItem(final int id, final String label, final String icon) {
        if (mMenu == null)
            return;

        final MenuItem item = mMenu.add(Menu.NONE, id, Menu.NONE, label);

        item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
                Log.i(LOGTAG, "menu item clicked");
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Menu:Clicked", Integer.toString(id)));
                ((Activity) GeckoApp.mAppContext).closeOptionsMenu();
                return true;
            }
        });

        if (icon != null) {
            if (icon.startsWith("data")) {
                byte[] raw = GeckoAppShell.decodeBase64(icon.substring(22), GeckoAppShell.BASE64_DEFAULT);
                Bitmap bitmap = BitmapFactory.decodeByteArray(raw, 0, raw.length);
                BitmapDrawable drawable = new BitmapDrawable(bitmap);
                item.setIcon(drawable);
            }
            else if (icon.startsWith("jar:") || icon.startsWith("file://")) {
                GeckoAppShell.getHandler().post(new Runnable() {
                    public void run() {
                        try {
                            URL url = new URL(icon);
                            InputStream is = (InputStream) url.getContent();
                            Drawable drawable = Drawable.createFromStream(is, "src");
                            item.setIcon(drawable);
                        } catch (Exception e) {
                            Log.w(LOGTAG, "Unable to set icon", e);
                        }
                    }
                });
            }
        }
        sAddonMenuItems.add(item);
    }

    private void removeAddonMenuItem(int id) {
        for (MenuItem item : sAddonMenuItems) {
            if (item.getItemId() == id) {
                sAddonMenuItems.remove(item);

                if (mMenu == null)
                    break;

                MenuItem menuItem = mMenu.findItem(id);
                if (menuItem != null)
                    mMenu.removeItem(id);

                break;
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);

        
        if (menu instanceof GeckoMenu && isTablet())
            ((GeckoMenu) menu).setActionItemBarPresenter(mBrowserToolbar);

        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.browser_app_menu, mMenu);
        return true;
    }

    @Override
    public void openOptionsMenu() {
        if (!hasTabsSideBar() && areTabsShown())
            return;

        
        if (mMenuPanel != null)
            mMenuPanel.scrollTo(0, 0);

        if (!mBrowserToolbar.openOptionsMenu())
            super.openOptionsMenu();
    }

    @Override
    public void closeOptionsMenu() {
        if (!mBrowserToolbar.closeOptionsMenu())
            super.closeOptionsMenu();
    }

    @Override
    public void setFullScreen(final boolean fullscreen) {
      super.setFullScreen(fullscreen);
      mMainHandler.post(new Runnable() {
          public void run() {
              if (fullscreen)
                  mBrowserToolbar.hide();
              else
                  mBrowserToolbar.show();
          }
      });
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu aMenu) {
        if (aMenu == null)
            return false;

        if (!sIsGeckoReady)
            aMenu.findItem(R.id.settings).setEnabled(false);

        Tab tab = Tabs.getInstance().getSelectedTab();
        MenuItem bookmark = aMenu.findItem(R.id.bookmark);
        MenuItem forward = aMenu.findItem(R.id.forward);
        MenuItem share = aMenu.findItem(R.id.share);
        MenuItem readingList = aMenu.findItem(R.id.reading_list);
        MenuItem saveAsPDF = aMenu.findItem(R.id.save_as_pdf);
        MenuItem charEncoding = aMenu.findItem(R.id.char_encoding);
        MenuItem findInPage = aMenu.findItem(R.id.find_in_page);
        MenuItem desktopMode = aMenu.findItem(R.id.desktop_mode);

        if (tab == null || tab.getURL() == null) {
            bookmark.setEnabled(false);
            forward.setEnabled(false);
            share.setEnabled(false);
            readingList.setEnabled(false);
            saveAsPDF.setEnabled(false);
            findInPage.setEnabled(false);
            return true;
        }

        bookmark.setEnabled(!tab.getURL().startsWith("about:reader"));
        bookmark.setCheckable(true);
        
        if (tab.isBookmark()) {
            bookmark.setChecked(true);
            bookmark.setIcon(R.drawable.ic_menu_bookmark_remove);
        } else {
            bookmark.setChecked(false);
            bookmark.setIcon(R.drawable.ic_menu_bookmark_add);
        }

        readingList.setEnabled(tab.getReaderEnabled());
        readingList.setCheckable(true);

        if (tab.isReadingListItem()) {
            readingList.setChecked(true);
            readingList.setIcon(R.drawable.ic_menu_reading_list_remove);
        } else {
            readingList.setChecked(false);
            readingList.setIcon(R.drawable.ic_menu_reading_list_add);
        }

        forward.setEnabled(tab.canDoForward());
        desktopMode.setChecked(tab.getDesktopMode());

        
        String scheme = Uri.parse(tab.getURL()).getScheme();
        share.setEnabled(!(scheme.equals("about") || scheme.equals("chrome") ||
                           scheme.equals("file") || scheme.equals("resource")));

        
        saveAsPDF.setEnabled(!(tab.getURL().equals("about:home") ||
                               tab.getContentType().equals("application/vnd.mozilla.xul+xml")));

        
        findInPage.setEnabled(!tab.getURL().equals("about:home"));

        charEncoding.setVisible(GeckoPreferences.getCharEncodingState());

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Tab tab = null;
        Intent intent = null;
        switch (item.getItemId()) {
            case R.id.bookmark:
                tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    if (item.isChecked()) {
                        tab.removeBookmark();
                        Toast.makeText(this, R.string.bookmark_removed, Toast.LENGTH_SHORT).show();
                        item.setIcon(R.drawable.ic_menu_bookmark_add);
                    } else {
                        tab.addBookmark();
                        Toast.makeText(this, R.string.bookmark_added, Toast.LENGTH_SHORT).show();
                        item.setIcon(R.drawable.ic_menu_bookmark_remove);
                    }
                }
                return true;
            case R.id.share:
                shareCurrentUrl();
                return true;
            case R.id.reading_list:
                tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    if (item.isChecked()) {
                        tab.removeFromReadingList();
                        item.setIcon(R.drawable.ic_menu_reading_list_add);
                        Toast.makeText(this, R.string.reading_list_removed, Toast.LENGTH_SHORT).show();
                    } else {
                        tab.addToReadingList();
                        item.setIcon(R.drawable.ic_menu_reading_list_remove);
                    }
                }
                return true;
            case R.id.reload:
                tab = Tabs.getInstance().getSelectedTab();
                if (tab != null)
                    tab.doReload();
                return true;
            case R.id.forward:
                tab = Tabs.getInstance().getSelectedTab();
                if (tab != null)
                    tab.doForward();
                return true;
            case R.id.save_as_pdf:
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SaveAs:PDF", null));
                return true;
            case R.id.settings:
                intent = new Intent(this, GeckoPreferences.class);
                startActivity(intent);
                return true;
            case R.id.addons:
                loadUrlInTab("about:addons");
                return true;
            case R.id.downloads:
                loadUrlInTab("about:downloads");
                return true;
            case R.id.apps:
                loadUrlInTab("about:apps");
                return true;
            case R.id.char_encoding:
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("CharEncoding:Get", null));
                return true;
            case R.id.find_in_page:
                mFindInPageBar.show();
                return true;
            case R.id.desktop_mode:
                Tab selectedTab = Tabs.getInstance().getSelectedTab();
                if (selectedTab == null)
                    return true;
                JSONObject args = new JSONObject();
                try {
                    args.put("desktopMode", !item.isChecked());
                    args.put("tabId", selectedTab.getId());
                } catch (JSONException e) {
                    Log.e(LOGTAG, "error building json arguments");
                }
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("DesktopMode:Change", args.toString()));
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    


 
    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);

        if (!Intent.ACTION_MAIN.equals(intent.getAction()))
            return;

        (new GeckoAsyncTask<Void, Void, Boolean>(mAppContext, GeckoAppShell.getHandler()) {
            @Override
            public synchronized Boolean doInBackground(Void... params) {
                
                SharedPreferences settings = getPreferences(Activity.MODE_PRIVATE);
                String keyName = getPackageName() + ".feedback_launch_count";
                int launchCount = settings.getInt(keyName, 0);
                if (launchCount >= FEEDBACK_LAUNCH_COUNT)
                    return false;

                
                launchCount++;
                settings.edit().putInt(keyName, launchCount).commit();

                
                return launchCount == FEEDBACK_LAUNCH_COUNT;
            }

            @Override
            public void onPostExecute(Boolean shouldShowFeedbackPage) {
                if (shouldShowFeedbackPage)
                    loadUrlInTab("about:feedback");
            }
        }).execute();
    }

    private void resetFeedbackLaunchCount() {
        GeckoBackgroundThread.post(new Runnable() {
            @Override
            public synchronized void run() {
                SharedPreferences settings = getPreferences(Activity.MODE_PRIVATE);
                settings.edit().putInt(getPackageName() + ".feedback_launch_count", 0).commit();
            }
        });
    }

    private void getLastUrl() {
        (new GeckoAsyncTask<Void, Void, String>(mAppContext, GeckoAppShell.getHandler()) {
            @Override
            public synchronized String doInBackground(Void... params) {
                
                String url = "";
                Cursor c = BrowserDB.getRecentHistory(getContentResolver(), 1);
                if (c.moveToFirst()) {
                    url = c.getString(c.getColumnIndexOrThrow(Combined.URL));
                }
                c.close();
                return url;
            }

            @Override
            public void onPostExecute(String url) {
                
                if (url.length() > 0)
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Feedback:LastUrl", url));
            }
        }).execute();
    }
}
