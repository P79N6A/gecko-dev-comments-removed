



package org.mozilla.gecko;

import java.util.Map;

import org.mozilla.gecko.tests.BaseRobocopTest;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;







public class LaunchFennecWithConfigurationActivity extends Activity {
    @Override
    public void onCreate(Bundle arguments) {
        super.onCreate(arguments);
    }

    @Override
    public void onResume() {
        super.onResume();

        final String configFile = FennecNativeDriver.getFile(BaseRobocopTest.DEFAULT_ROOT_PATH + "/robotium.config");
        final Map<String, String> config = FennecNativeDriver.convertTextToTable(configFile);
        final Intent intent = BaseRobocopTest.createActivityIntent(config);

        intent.setClassName(AppConstants.ANDROID_PACKAGE_NAME, AppConstants.MOZ_ANDROID_BROWSER_INTENT_CLASS);

        this.finish();
        this.startActivity(intent);
    }
}
