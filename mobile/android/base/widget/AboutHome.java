




package org.mozilla.gecko.widget;

import java.util.EnumSet;

import org.mozilla.gecko.GeckoApplication;
import org.mozilla.gecko.LightweightTheme;
import org.mozilla.gecko.R;
import org.mozilla.gecko.ScrollAnimator;
import org.mozilla.gecko.db.BrowserContract;

import android.app.Activity;
import android.content.res.Configuration;
import android.database.ContentObserver;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

public class AboutHome extends Fragment {
    public static enum UpdateFlags {
        TOP_SITES,
        PREVIOUS_TABS;

        public static final EnumSet<UpdateFlags> ALL = EnumSet.allOf(UpdateFlags.class);
    }

    private UriLoadListener mUriLoadListener;
    private LoadCompleteListener mLoadCompleteListener;
    private LightweightTheme mLightweightTheme;
    private int mTopPadding;
    private AboutHomeView mAboutHomeView;
    private LastTabsSection mLastTabsSection;
    private TopSitesView mTopSitesView;
    private ScrollAnimator mScrollAnimator;

    public interface UriLoadListener {
        public void onAboutHomeUriLoad(String uriSpec);
    }

    public interface LoadCompleteListener {
        public void onAboutHomeLoadComplete();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mLightweightTheme = ((GeckoApplication) getActivity().getApplication()).getLightweightTheme();
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        try {
            mUriLoadListener = (UriLoadListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement UriLoadListener");
        }

        try {
            mLoadCompleteListener = (LoadCompleteListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement LoadCompleteListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();

        mUriLoadListener = null;
        mLoadCompleteListener = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        super.onCreateView(inflater, container, savedInstanceState);

        mAboutHomeView = (AboutHomeView) inflater.inflate(R.layout.abouthome_content, container, false);
        mLastTabsSection = (LastTabsSection) mAboutHomeView.findViewById(R.id.last_tabs);
        mTopSitesView = (TopSitesView) mAboutHomeView.findViewById(R.id.top_sites_grid);

        mAboutHomeView.setLightweightTheme(mLightweightTheme);
        mLightweightTheme.addListener(mAboutHomeView);

        
        
        if (Build.VERSION.SDK_INT >= 12) {
            mScrollAnimator = new ScrollAnimator();
            mAboutHomeView.setOnGenericMotionListener(mScrollAnimator);
        }

        return mAboutHomeView;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        view.setPadding(0, mTopPadding, 0, 0);
        ((PromoBox) view.findViewById(R.id.promo_box)).showRandomPromo();
        update(AboutHome.UpdateFlags.ALL);

        mTopSitesView.setLoadCompleteListener(mLoadCompleteListener);
        mTopSitesView.setUriLoadListener(mUriLoadListener);
    }

    @Override
    public void onDestroyView() {
        mLightweightTheme.removeListener(mAboutHomeView);
        mTopSitesView.onDestroy();

        if (mScrollAnimator != null) {
            mScrollAnimator.cancel();
        }
        mScrollAnimator = null;

        mAboutHomeView = null;
        mLastTabsSection = null;
        mTopSitesView = null;

        super.onDestroyView();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        
        
        
        
        
        
        
        
        if (isVisible()) {
            getFragmentManager().beginTransaction()
                                .detach(this)
                                .attach(this)
                                .commitAllowingStateLoss();
        }
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        ContextMenuInfo info = item.getMenuInfo();

        if (getView() == null) {
            return true;
        }

        switch (item.getItemId()) {
            case R.id.abouthome_open_new_tab:
                mTopSitesView.openNewTab(info);
                return true;

            case R.id.abouthome_open_private_tab:
                mTopSitesView.openNewPrivateTab(info);
                return true;

            case R.id.abouthome_topsites_edit:
                mTopSitesView.editSite(info);
                return true;

            case R.id.abouthome_topsites_unpin:
                mTopSitesView.unpinSite(info, TopSitesView.UnpinFlags.REMOVE_PIN);
                return true;

            case R.id.abouthome_topsites_pin:
                mTopSitesView.pinSite(info);
                return true;

        }
        return super.onContextItemSelected(item);
    }

    public void update(final EnumSet<UpdateFlags> flags) {
        if (getView() == null) {
            return;
        }

        if (flags.contains(UpdateFlags.TOP_SITES)) {
            mTopSitesView.loadTopSites();
        }

        if (flags.contains(UpdateFlags.PREVIOUS_TABS)) {
            mLastTabsSection.readLastTabs();
        }
    }

    public void setLastTabsVisibility(boolean visible) {
        if (mAboutHomeView == null) {
            return;
        }

        if (visible)
            mLastTabsSection.show();
        else
            mLastTabsSection.hide();
    }

    public void requestFocus() {
        View view = getView();
        if (view != null) {
            view.requestFocus();
        }
    }

    public void setTopPadding(int topPadding) {
        View view = getView();
        if (view != null) {
            view.setPadding(0, topPadding, 0, 0);
        }

        
        
        
        mTopPadding = topPadding;
    }

    public int getTopPadding() {
        return mTopPadding;
    }
}
