
























































#ifdef DEFINED_CSS_STATE_PSEUDO_CLASS
#error "This shouldn't be defined"
#endif

#ifndef CSS_STATE_PSEUDO_CLASS
#define CSS_STATE_PSEUDO_CLASS(_name, _value, _bit) \
  CSS_PSEUDO_CLASS(_name, _value)
#define DEFINED_CSS_STATE_PSEUDO_CLASS
#endif

CSS_PSEUDO_CLASS(empty, ":empty")
CSS_PSEUDO_CLASS(mozOnlyWhitespace, ":-moz-only-whitespace")
CSS_PSEUDO_CLASS(mozEmptyExceptChildrenWithLocalname, ":-moz-empty-except-children-with-localname")
CSS_PSEUDO_CLASS(lang, ":lang")
CSS_PSEUDO_CLASS(notPseudo, ":not")
CSS_PSEUDO_CLASS(mozBoundElement, ":-moz-bound-element")
CSS_PSEUDO_CLASS(root, ":root")

CSS_PSEUDO_CLASS(link, ":link")
CSS_PSEUDO_CLASS(mozAnyLink, ":-moz-any-link") 
CSS_PSEUDO_CLASS(visited, ":visited")

CSS_STATE_PSEUDO_CLASS(active, ":active", NS_EVENT_STATE_ACTIVE)
CSS_STATE_PSEUDO_CLASS(checked, ":checked", NS_EVENT_STATE_CHECKED)
CSS_STATE_PSEUDO_CLASS(disabled, ":disabled", NS_EVENT_STATE_DISABLED)
CSS_STATE_PSEUDO_CLASS(enabled, ":enabled", NS_EVENT_STATE_ENABLED)
CSS_STATE_PSEUDO_CLASS(focus, ":focus", NS_EVENT_STATE_FOCUS)
CSS_STATE_PSEUDO_CLASS(hover, ":hover", NS_EVENT_STATE_HOVER)
CSS_STATE_PSEUDO_CLASS(mozDragOver, ":-moz-drag-over", NS_EVENT_STATE_DRAGOVER)
CSS_STATE_PSEUDO_CLASS(target, ":target", NS_EVENT_STATE_URLTARGET)
CSS_STATE_PSEUDO_CLASS(indeterminate, ":indeterminate",
                       NS_EVENT_STATE_INDETERMINATE)

CSS_PSEUDO_CLASS(firstChild, ":first-child")
CSS_PSEUDO_CLASS(firstNode, ":-moz-first-node")
CSS_PSEUDO_CLASS(lastChild, ":last-child")
CSS_PSEUDO_CLASS(lastNode, ":-moz-last-node")
CSS_PSEUDO_CLASS(onlyChild, ":only-child")
CSS_PSEUDO_CLASS(firstOfType, ":first-of-type")
CSS_PSEUDO_CLASS(lastOfType, ":last-of-type")
CSS_PSEUDO_CLASS(onlyOfType, ":only-of-type")
CSS_PSEUDO_CLASS(nthChild, ":nth-child")
CSS_PSEUDO_CLASS(nthLastChild, ":nth-last-child")
CSS_PSEUDO_CLASS(nthOfType, ":nth-of-type")
CSS_PSEUDO_CLASS(nthLastOfType, ":nth-last-of-type")


CSS_STATE_PSEUDO_CLASS(mozBroken, ":-moz-broken", NS_EVENT_STATE_BROKEN)
CSS_STATE_PSEUDO_CLASS(mozUserDisabled, ":-moz-user-disabled",
                       NS_EVENT_STATE_USERDISABLED)
CSS_STATE_PSEUDO_CLASS(mozSuppressed, ":-moz-suppressed",
                       NS_EVENT_STATE_SUPPRESSED)
CSS_STATE_PSEUDO_CLASS(mozLoading, ":-moz-loading", NS_EVENT_STATE_LOADING)
CSS_STATE_PSEUDO_CLASS(mozTypeUnsupported, ":-moz-type-unsupported",
                       NS_EVENT_STATE_TYPE_UNSUPPORTED)
CSS_STATE_PSEUDO_CLASS(mozHandlerDisabled, ":-moz-handler-disabled",
                       NS_EVENT_STATE_HANDLER_DISABLED)
CSS_STATE_PSEUDO_CLASS(mozHandlerBlocked, ":-moz-handler-blocked",
                       NS_EVENT_STATE_HANDLER_BLOCKED)

CSS_PSEUDO_CLASS(mozHasHandlerRef, ":-moz-has-handlerref")


CSS_PSEUDO_CLASS(mozIsHTML, ":-moz-is-html")


CSS_PSEUDO_CLASS(mozSystemMetric, ":-moz-system-metric")



CSS_PSEUDO_CLASS(mozLocaleDir, ":-moz-locale-dir")


CSS_PSEUDO_CLASS(mozLWTheme, ":-moz-lwtheme")


CSS_PSEUDO_CLASS(mozLWThemeBrightText, ":-moz-lwtheme-brighttext")


CSS_PSEUDO_CLASS(mozLWThemeDarkText, ":-moz-lwtheme-darktext")

#ifdef MOZ_MATHML
CSS_STATE_PSEUDO_CLASS(mozMathIncrementScriptLevel,
                       ":-moz-math-increment-script-level",
                       NS_EVENT_STATE_INCREMENT_SCRIPT_LEVEL)
#endif



CSS_STATE_PSEUDO_CLASS(required, ":required", NS_EVENT_STATE_REQUIRED)
CSS_STATE_PSEUDO_CLASS(optional, ":optional", NS_EVENT_STATE_OPTIONAL)
CSS_STATE_PSEUDO_CLASS(valid, ":valid", NS_EVENT_STATE_VALID)
CSS_STATE_PSEUDO_CLASS(invalid, ":invalid", NS_EVENT_STATE_INVALID)
CSS_STATE_PSEUDO_CLASS(inRange, ":in-range", NS_EVENT_STATE_INRANGE)
CSS_STATE_PSEUDO_CLASS(outOfRange, ":out-of-range", NS_EVENT_STATE_OUTOFRANGE)
CSS_STATE_PSEUDO_CLASS(defaultPseudo, ":default", NS_EVENT_STATE_DEFAULT)
CSS_STATE_PSEUDO_CLASS(mozReadOnly, ":-moz-read-only",
                       NS_EVENT_STATE_MOZ_READONLY)
CSS_STATE_PSEUDO_CLASS(mozReadWrite, ":-moz-read-write",
                       NS_EVENT_STATE_MOZ_READWRITE)

#ifdef DEFINED_CSS_STATE_PSEUDO_CLASS
#undef DEFINED_CSS_STATE_PSEUDO_CLASS
#undef CSS_STATE_PSEUDO_CLASS
#endif
