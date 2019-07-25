



#ifndef nsHtml5TokenizerLoopPolicies_h_
#define nsHtml5TokenizerLoopPolicies_h_





struct nsHtml5SilentPolicy
{
  static const bool reportErrors = false;
  static int32_t transition(nsHtml5Highlighter* aHighlighter,
                            int32_t aState,
                            bool aReconsume,
                            int32_t aPos) {
    return aState;
  }
  static void completedNamedCharacterReference(nsHtml5Highlighter* aHighlighter) {
  }
};





struct nsHtml5ViewSourcePolicy
{
  static const bool reportErrors = true;
  static int32_t transition(nsHtml5Highlighter* aHighlighter,
                            int32_t aState,
                            bool aReconsume,
                            int32_t aPos) {
    return aHighlighter->Transition(aState, aReconsume, aPos);
  }
  static void completedNamedCharacterReference(nsHtml5Highlighter* aHighlighter) {
    aHighlighter->CompletedNamedCharacterReference();
  }
};

#endif 
