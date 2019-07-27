




package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;
import android.annotation.TargetApi;
import android.content.Context;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.UserManager;
import android.util.Log;

@RobocopTarget
public class RestrictedProfiles {
    private static final String LOGTAG = "GeckoRestrictedProfiles";

    private static volatile Boolean inGuest = null;

    @SuppressWarnings("serial")
    private static final List<String> BANNED_SCHEMES = new ArrayList<String>() {{
        add("file");
        add("chrome");
        add("resource");
        add("jar");
        add("wyciwyg");
    }};

    





    public static void initWithProfile(GeckoProfile profile) {
        inGuest = profile.inGuestMode();
    }

    private static boolean getInGuest() {
        if (inGuest == null) {
            inGuest = GeckoAppShell.getGeckoInterface().getProfile().inGuestMode();
        }

        return inGuest;
    }

    @SuppressWarnings("serial")
    private static final List<String> BANNED_URLS = new ArrayList<String>() {{
        add("about:config");
    }};

    



    public static enum Restriction {
        DISALLOW_DOWNLOADS(1, "no_download_files"),
        DISALLOW_INSTALL_EXTENSION(2, "no_install_extensions"),
        DISALLOW_INSTALL_APPS(3, "no_install_apps"), 
        DISALLOW_BROWSE_FILES(4, "no_browse_files"),
        DISALLOW_SHARE(5, "no_share"),
        DISALLOW_BOOKMARK(6, "no_bookmark"),
        DISALLOW_ADD_CONTACTS(7, "no_add_contacts"),
        DISALLOW_SET_IMAGE(8, "no_set_image"),
        DISALLOW_MODIFY_ACCOUNTS(9, "no_modify_accounts"), 
        DISALLOW_REMOTE_DEBUGGING(10, "no_remote_debugging"),
        DISALLOW_IMPORT_SETTINGS(11, "no_import_settings"),
        DISALLOW_TOOLS_MENU(12, "no_tools_menu"),
        DISALLOW_REPORT_SITE_ISSUE(13, "no_report_site_issue");

        public final int id;
        public final String name;

        private Restriction(final int id, final String name) {
            this.id = id;
            this.name = name;
        }
    }

    private static List<Restriction> restrictionsOfGuestProfile = Arrays.asList(
        Restriction.DISALLOW_DOWNLOADS,
        Restriction.DISALLOW_INSTALL_EXTENSION,
        Restriction.DISALLOW_INSTALL_APPS,
        Restriction.DISALLOW_BROWSE_FILES,
        Restriction.DISALLOW_SHARE,
        Restriction.DISALLOW_BOOKMARK,
        Restriction.DISALLOW_ADD_CONTACTS,
        Restriction.DISALLOW_SET_IMAGE,
        Restriction.DISALLOW_MODIFY_ACCOUNTS,
        Restriction.DISALLOW_REMOTE_DEBUGGING,
        Restriction.DISALLOW_IMPORT_SETTINGS
    );

    
    private static List<Restriction> defaultRestrictionsOfRestrictedProfiles = Arrays.asList(
        Restriction.DISALLOW_TOOLS_MENU,
        Restriction.DISALLOW_REPORT_SITE_ISSUE
    );

    private static Restriction geckoActionToRestriction(int action) {
        for (Restriction rest : Restriction.values()) {
            if (rest.id == action) {
                return rest;
            }
        }

        throw new IllegalArgumentException("Unknown action " + action);
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
    private static Bundle getRestrictions(final Context context) {
        final UserManager mgr = (UserManager) context.getSystemService(Context.USER_SERVICE);
        return mgr.getUserRestrictions();
    }

    








    private static boolean getRestriction(final Context context, final String name) {
        
        
        if (Versions.preJBMR2) {
            return false;
        }

        
        if (isUserRestricted(context) && defaultRestrictionsOfRestrictedProfiles.contains(Restriction.DISALLOW_TOOLS_MENU)) {
            return true;
        }

        return getRestrictions(context).getBoolean(name, false);
    }

    private static boolean canLoadUrl(final Context context, final String url) {
        
        if (url == null) {
            return true;
        }

        try {
            
            if (!getInGuest() &&
                !getRestriction(context, Restriction.DISALLOW_BROWSE_FILES.name)) {
                return true;
            }
        } catch (IllegalArgumentException ex) {
            Log.i(LOGTAG, "Invalid action", ex);
        }

        final Uri u = Uri.parse(url);
        final String scheme = u.getScheme();
        if (BANNED_SCHEMES.contains(scheme)) {
            return false;
        }

        for (String banned : BANNED_URLS) {
            if (url.startsWith(banned)) {
                return false;
            }
        }

        
        return true;
    }

    @WrapElementForJNI
    public static boolean isUserRestricted() {
        return isUserRestricted(GeckoAppShell.getContext());
    }

    private static boolean isUserRestricted(final Context context) {
        
        if (getInGuest()) {
            return true;
        }

        if (Versions.preJBMR2) {
            return false;
        }

        return !getRestrictions(context).isEmpty();
    }

    public static boolean isAllowed(final Context context, final Restriction action) {
        return isAllowed(context, action, null);
    }

    @WrapElementForJNI
    public static boolean isAllowed(int action, String url) {
        return isAllowed(GeckoAppShell.getContext(), action, url);
    }

    private static boolean isAllowed(final Context context, int action, String url) {
        final Restriction restriction;
        try {
            restriction = geckoActionToRestriction(action);
        } catch (IllegalArgumentException ex) {
            
            
            Log.e(LOGTAG, "Unknown action " + action + "; check calling code.");
            return false;
        }

        return isAllowed(context, restriction, url);
    }

    private static boolean isAllowed(final Context context, final Restriction restriction, String url) {
        if (getInGuest()) {
            if (Restriction.DISALLOW_BROWSE_FILES == restriction) {
                return canLoadUrl(context, url);
            }

            return !restrictionsOfGuestProfile.contains(restriction);
        }

        
        return !getRestriction(context, restriction.name);
    }

    @WrapElementForJNI
    public static String getUserRestrictions() {
        return getUserRestrictions(GeckoAppShell.getContext());
    }

    private static String getUserRestrictions(final Context context) {
        
        if (getInGuest()) {
            StringBuilder builder = new StringBuilder("{ ");

            for (Restriction restriction : Restriction.values()) {
                builder.append("\"" + restriction.name + "\": true, ");
            }

            builder.append(" }");
            return builder.toString();
        }

        if (Versions.preJBMR2) {
            return "{}";
        }

        final JSONObject json = new JSONObject();
        final Bundle restrictions = getRestrictions(context);
        final Set<String> keys = restrictions.keySet();

        for (String key : keys) {
            try {
                json.put(key, restrictions.get(key));
            } catch (JSONException e) {
            }
        }

        return json.toString();
    }
}
