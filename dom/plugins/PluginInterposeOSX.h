




























namespace mac_plugin_interposing {

namespace parent {

void OnPluginShowWindow(uint32_t window_id, CGRect window_bounds, bool modal);
void OnPluginHideWindow(uint32_t window_id, pid_t aPluginPid);

}

namespace child {

void SetUpCocoaInterposing();

}

}
