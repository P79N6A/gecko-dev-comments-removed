



package org.mozilla.gecko.home;

import java.util.EnumSet;
import java.util.Locale;

import org.mozilla.gecko.R;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.activities.FxAccountCreateAccountActivity;
import org.mozilla.gecko.fxa.activities.FxAccountUpdateCredentialsActivity;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;

import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
















public class RemoteTabsStaticFragment extends HomeFragment implements OnClickListener {
    @SuppressWarnings("unused")
    private static final String LOGTAG = "GeckoRemoteTabsStatic";

    protected static final String RESOURCE_ID = "resource_id";
    protected static final int DEFAULT_RESOURCE_ID = R.layout.remote_tabs_setup;

    private static final String CONFIRM_ACCOUNT_SUPPORT_URL =
            "https://support.mozilla.org/kb/im-having-problems-confirming-my-firefox-account";

    protected int mLayoutId;

    public static RemoteTabsStaticFragment newInstance(int resourceId) {
        final RemoteTabsStaticFragment fragment = new RemoteTabsStaticFragment();

        final Bundle args = new Bundle();
        args.putInt(RESOURCE_ID, resourceId);
        fragment.setArguments(args);

        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final Bundle args = getArguments();
        if (args != null) {
            mLayoutId = args.getInt(RESOURCE_ID, DEFAULT_RESOURCE_ID);
        } else {
            mLayoutId = DEFAULT_RESOURCE_ID;
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(mLayoutId, container, false);
    }

    protected boolean maybeSetOnClickListener(View view, int resourceId) {
        final View button = view.findViewById(resourceId);
        if (button != null) {
            button.setOnClickListener(this);
            return true;
        }
        return false;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        for (int resourceId : new int[] {
                R.id.remote_tabs_setup_get_started,
                R.id.remote_tabs_setup_old_sync_link,
                R.id.remote_tabs_needs_verification_resend_email,
                R.id.remote_tabs_needs_verification_help,
                R.id.remote_tabs_needs_password_sign_in, }) {
            maybeSetOnClickListener(view, resourceId);
        }
    }

    @Override
    public void onClick(final View v) {
        final int id = v.getId();
        if (id == R.id.remote_tabs_setup_get_started) {
            
            final Intent intent = new Intent(getActivity(), FxAccountCreateAccountActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(intent);
        } else if (id == R.id.remote_tabs_setup_old_sync_link) {
            final String url = FirefoxAccounts.getOldSyncUpgradeURL(getResources(), Locale.getDefault());
            
            final EnumSet<OnUrlOpenListener.Flags> flags = EnumSet.noneOf(OnUrlOpenListener.Flags.class);
            mUrlOpenListener.onUrlOpen(url, flags);
        } else if (id == R.id.remote_tabs_needs_verification_resend_email) {
            
            FirefoxAccounts.resendVerificationEmail(getActivity());
        } else if (id == R.id.remote_tabs_needs_verification_help) {
            
            final EnumSet<OnUrlOpenListener.Flags> flags = EnumSet.noneOf(OnUrlOpenListener.Flags.class);
            mUrlOpenListener.onUrlOpen(CONFIRM_ACCOUNT_SUPPORT_URL, flags);
        } else if (id == R.id.remote_tabs_needs_password_sign_in) {
            
            final Intent intent = new Intent(getActivity(), FxAccountUpdateCredentialsActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(intent);
        }
    }

    @Override
    protected void load() {
        
    }
}
