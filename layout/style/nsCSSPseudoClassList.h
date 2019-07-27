










































#ifdef DEFINED_CSS_STATE_DEPENDENT_PSEUDO_CLASS
#error "CSS_STATE_DEPENDENT_PSEUDO_CLASS shouldn't be defined"
#endif

#ifndef CSS_STATE_DEPENDENT_PSEUDO_CLASS
#define CSS_STATE_DEPENDENT_PSEUDO_CLASS(_name, _value, _flags, _pref, _bit)  \
  CSS_PSEUDO_CLASS(_name, _value, _flags, _pref)
#define DEFINED_CSS_STATE_DEPENDENT_PSEUDO_CLASS
#endif

#ifdef DEFINED_CSS_STATE_PSEUDO_CLASS
#error "CSS_STATE_PSEUDO_CLASS shouldn't be defined"
#endif

#ifndef CSS_STATE_PSEUDO_CLASS
#define CSS_STATE_PSEUDO_CLASS(_name, _value, _flags, _pref, _bit)      \
  CSS_STATE_DEPENDENT_PSEUDO_CLASS(_name, _value, _flags, _pref, _bit)
#define DEFINED_CSS_STATE_PSEUDO_CLASS
#endif






CSS_PSEUDO_CLASS(empty, ":empty", 0, "")
CSS_PSEUDO_CLASS(mozOnlyWhitespace, ":-moz-only-whitespace", 0, "")
CSS_PSEUDO_CLASS(mozEmptyExceptChildrenWithLocalname, ":-moz-empty-except-children-with-localname", 0, "")
CSS_PSEUDO_CLASS(lang, ":lang", 0, "")
CSS_PSEUDO_CLASS(mozBoundElement, ":-moz-bound-element", 0, "")
CSS_PSEUDO_CLASS(root, ":root", 0, "")
CSS_PSEUDO_CLASS(any, ":-moz-any", 0, "")

CSS_PSEUDO_CLASS(firstChild, ":first-child", 0, "")
CSS_PSEUDO_CLASS(firstNode, ":-moz-first-node", 0, "")
CSS_PSEUDO_CLASS(lastChild, ":last-child", 0, "")
CSS_PSEUDO_CLASS(lastNode, ":-moz-last-node", 0, "")
CSS_PSEUDO_CLASS(onlyChild, ":only-child", 0, "")
CSS_PSEUDO_CLASS(firstOfType, ":first-of-type", 0, "")
CSS_PSEUDO_CLASS(lastOfType, ":last-of-type", 0, "")
CSS_PSEUDO_CLASS(onlyOfType, ":only-of-type", 0, "")
CSS_PSEUDO_CLASS(nthChild, ":nth-child", 0, "")
CSS_PSEUDO_CLASS(nthLastChild, ":nth-last-child", 0, "")
CSS_PSEUDO_CLASS(nthOfType, ":nth-of-type", 0, "")
CSS_PSEUDO_CLASS(nthLastOfType, ":nth-last-of-type", 0, "")


CSS_PSEUDO_CLASS(mozIsHTML, ":-moz-is-html", 0, "")


 CSS_STATE_PSEUDO_CLASS(unresolved, ":unresolved", 0, "", NS_EVENT_STATE_UNRESOLVED)




CSS_PSEUDO_CLASS(mozNativeAnonymous, ":-moz-native-anonymous",
                 CSS_PSEUDO_CLASS_UA_SHEET_ONLY, "")


CSS_PSEUDO_CLASS(mozSystemMetric, ":-moz-system-metric", 0, "")



CSS_PSEUDO_CLASS(mozLocaleDir, ":-moz-locale-dir", 0, "")


CSS_PSEUDO_CLASS(mozLWTheme, ":-moz-lwtheme", 0, "")


CSS_PSEUDO_CLASS(mozLWThemeBrightText, ":-moz-lwtheme-brighttext", 0, "")


CSS_PSEUDO_CLASS(mozLWThemeDarkText, ":-moz-lwtheme-darktext", 0, "")


CSS_PSEUDO_CLASS(mozWindowInactive, ":-moz-window-inactive", 0, "")



CSS_PSEUDO_CLASS(mozTableBorderNonzero, ":-moz-table-border-nonzero", 0, "")



CSS_PSEUDO_CLASS(scope, ":scope", 0, "layout.css.scope-pseudo.enabled")



CSS_PSEUDO_CLASS(notPseudo, ":not", 0, "")



CSS_STATE_DEPENDENT_PSEUDO_CLASS(dir, ":-moz-dir", 0, "",
                                 NS_EVENT_STATE_LTR | NS_EVENT_STATE_RTL)

CSS_STATE_PSEUDO_CLASS(link, ":link", 0, "", NS_EVENT_STATE_UNVISITED)

CSS_STATE_PSEUDO_CLASS(mozAnyLink, ":-moz-any-link", 0, "",
                       NS_EVENT_STATE_VISITED | NS_EVENT_STATE_UNVISITED)
CSS_STATE_PSEUDO_CLASS(visited, ":visited", 0, "", NS_EVENT_STATE_VISITED)

CSS_STATE_PSEUDO_CLASS(active, ":active", 0, "", NS_EVENT_STATE_ACTIVE)
CSS_STATE_PSEUDO_CLASS(checked, ":checked", 0, "", NS_EVENT_STATE_CHECKED)
CSS_STATE_PSEUDO_CLASS(disabled, ":disabled", 0, "", NS_EVENT_STATE_DISABLED)
CSS_STATE_PSEUDO_CLASS(enabled, ":enabled", 0, "", NS_EVENT_STATE_ENABLED)
CSS_STATE_PSEUDO_CLASS(focus, ":focus", 0, "", NS_EVENT_STATE_FOCUS)
CSS_STATE_PSEUDO_CLASS(hover, ":hover", 0, "", NS_EVENT_STATE_HOVER)
CSS_STATE_PSEUDO_CLASS(mozDragOver, ":-moz-drag-over", 0, "", NS_EVENT_STATE_DRAGOVER)
CSS_STATE_PSEUDO_CLASS(target, ":target", 0, "", NS_EVENT_STATE_URLTARGET)
CSS_STATE_PSEUDO_CLASS(indeterminate, ":indeterminate", 0, "",
                       NS_EVENT_STATE_INDETERMINATE)

CSS_STATE_PSEUDO_CLASS(mozDevtoolsHighlighted, ":-moz-devtools-highlighted", 0, "",
                       NS_EVENT_STATE_DEVTOOLS_HIGHLIGHTED)



CSS_STATE_PSEUDO_CLASS(mozFullScreen, ":-moz-full-screen", 0, "", NS_EVENT_STATE_FULL_SCREEN)



CSS_STATE_PSEUDO_CLASS(mozFullScreenAncestor, ":-moz-full-screen-ancestor", 0, "", NS_EVENT_STATE_FULL_SCREEN_ANCESTOR)


CSS_STATE_PSEUDO_CLASS(mozFocusRing, ":-moz-focusring", 0, "", NS_EVENT_STATE_FOCUSRING)


CSS_STATE_PSEUDO_CLASS(mozBroken, ":-moz-broken", 0, "", NS_EVENT_STATE_BROKEN)
CSS_STATE_PSEUDO_CLASS(mozUserDisabled, ":-moz-user-disabled", 0, "",
                       NS_EVENT_STATE_USERDISABLED)
CSS_STATE_PSEUDO_CLASS(mozSuppressed, ":-moz-suppressed", 0, "",
                       NS_EVENT_STATE_SUPPRESSED)
CSS_STATE_PSEUDO_CLASS(mozLoading, ":-moz-loading", 0, "", NS_EVENT_STATE_LOADING)
CSS_STATE_PSEUDO_CLASS(mozTypeUnsupported, ":-moz-type-unsupported", 0, "",
                       NS_EVENT_STATE_TYPE_UNSUPPORTED)
CSS_STATE_PSEUDO_CLASS(mozTypeUnsupportedPlatform, ":-moz-type-unsupported-platform", 0, "",
                       NS_EVENT_STATE_TYPE_UNSUPPORTED_PLATFORM)
CSS_STATE_PSEUDO_CLASS(mozHandlerClickToPlay, ":-moz-handler-clicktoplay", 0, "",
                       NS_EVENT_STATE_TYPE_CLICK_TO_PLAY)
CSS_STATE_PSEUDO_CLASS(mozHandlerPlayPreview, ":-moz-handler-playpreview", 0, "",
                       NS_EVENT_STATE_TYPE_PLAY_PREVIEW)
CSS_STATE_PSEUDO_CLASS(mozHandlerVulnerableUpdatable, ":-moz-handler-vulnerable-updatable", 0, "",
                       NS_EVENT_STATE_VULNERABLE_UPDATABLE)
CSS_STATE_PSEUDO_CLASS(mozHandlerVulnerableNoUpdate, ":-moz-handler-vulnerable-no-update", 0, "",
                       NS_EVENT_STATE_VULNERABLE_NO_UPDATE)
CSS_STATE_PSEUDO_CLASS(mozHandlerDisabled, ":-moz-handler-disabled", 0, "",
                       NS_EVENT_STATE_HANDLER_DISABLED)
CSS_STATE_PSEUDO_CLASS(mozHandlerBlocked, ":-moz-handler-blocked", 0, "",
                       NS_EVENT_STATE_HANDLER_BLOCKED)
CSS_STATE_PSEUDO_CLASS(mozHandlerCrashed, ":-moz-handler-crashed", 0, "",
                       NS_EVENT_STATE_HANDLER_CRASHED)

CSS_STATE_PSEUDO_CLASS(mozMathIncrementScriptLevel,
                       ":-moz-math-increment-script-level", 0, "",
                       NS_EVENT_STATE_INCREMENT_SCRIPT_LEVEL)



CSS_STATE_PSEUDO_CLASS(required, ":required", 0, "", NS_EVENT_STATE_REQUIRED)
CSS_STATE_PSEUDO_CLASS(optional, ":optional", 0, "", NS_EVENT_STATE_OPTIONAL)
CSS_STATE_PSEUDO_CLASS(valid, ":valid", 0, "", NS_EVENT_STATE_VALID)
CSS_STATE_PSEUDO_CLASS(invalid, ":invalid", 0, "", NS_EVENT_STATE_INVALID)
CSS_STATE_PSEUDO_CLASS(inRange, ":in-range", 0, "", NS_EVENT_STATE_INRANGE)
CSS_STATE_PSEUDO_CLASS(outOfRange, ":out-of-range", 0, "", NS_EVENT_STATE_OUTOFRANGE)
CSS_STATE_PSEUDO_CLASS(defaultPseudo, ":default", 0, "", NS_EVENT_STATE_DEFAULT)
CSS_STATE_PSEUDO_CLASS(mozReadOnly, ":-moz-read-only", 0, "",
                       NS_EVENT_STATE_MOZ_READONLY)
CSS_STATE_PSEUDO_CLASS(mozReadWrite, ":-moz-read-write", 0, "",
                       NS_EVENT_STATE_MOZ_READWRITE)
CSS_STATE_PSEUDO_CLASS(mozSubmitInvalid, ":-moz-submit-invalid", 0, "",
                       NS_EVENT_STATE_MOZ_SUBMITINVALID)
CSS_STATE_PSEUDO_CLASS(mozUIInvalid, ":-moz-ui-invalid", 0, "",
                       NS_EVENT_STATE_MOZ_UI_INVALID)
CSS_STATE_PSEUDO_CLASS(mozUIValid, ":-moz-ui-valid", 0, "",
                       NS_EVENT_STATE_MOZ_UI_VALID)
CSS_STATE_PSEUDO_CLASS(mozMeterOptimum, ":-moz-meter-optimum", 0, "",
                       NS_EVENT_STATE_OPTIMUM)
CSS_STATE_PSEUDO_CLASS(mozMeterSubOptimum, ":-moz-meter-sub-optimum", 0, "",
                       NS_EVENT_STATE_SUB_OPTIMUM)
CSS_STATE_PSEUDO_CLASS(mozMeterSubSubOptimum, ":-moz-meter-sub-sub-optimum", 0, "",
                       NS_EVENT_STATE_SUB_SUB_OPTIMUM)


CSS_STATE_PSEUDO_CLASS(mozPlaceholder, ":-moz-placeholder", 0, "", NS_EVENT_STATE_IGNORE)

#ifdef DEFINED_CSS_STATE_PSEUDO_CLASS
#undef DEFINED_CSS_STATE_PSEUDO_CLASS
#undef CSS_STATE_PSEUDO_CLASS
#endif

#ifdef DEFINED_CSS_STATE_DEPENDENT_PSEUDO_CLASS
#undef DEFINED_CSS_STATE_DEPENDENT_PSEUDO_CLASS
#undef CSS_STATE_DEPENDENT_PSEUDO_CLASS
#endif
