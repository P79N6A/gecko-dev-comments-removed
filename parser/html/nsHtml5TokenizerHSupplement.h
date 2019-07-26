



inline nsHtml5HtmlAttributes* GetAttributes()
{
  return attributes;
}

nsAutoPtr<nsHtml5Highlighter> mViewSource;





void StartPlainText();

void EnableViewSource(nsHtml5Highlighter* aHighlighter);

bool FlushViewSource();

void StartViewSource(const nsAutoString& aTitle);

void EndViewSource();

void errGarbageAfterLtSlash();

void errLtSlashGt();

void errWarnLtSlashInRcdata();

void errCharRefLacksSemicolon();

void errNoDigitsInNCR();

void errGtInSystemId();

void errGtInPublicId();

void errNamelessDoctype();

void errConsecutiveHyphens();

void errPrematureEndOfComment();

void errBogusComment();

void errUnquotedAttributeValOrNull(char16_t c);

void errSlashNotFollowedByGt();

void errNoSpaceBetweenAttributes();

void errLtOrEqualsOrGraveInUnquotedAttributeOrNull(char16_t c);

void errAttributeValueMissing();

void errBadCharBeforeAttributeNameOrNull(char16_t c);

void errEqualsSignBeforeAttributeName();

void errBadCharAfterLt(char16_t c);

void errLtGt();

void errProcessingInstruction();

void errUnescapedAmpersandInterpretedAsCharacterReference();

void errNotSemicolonTerminated();

void errNoNamedCharacterMatch();

void errQuoteBeforeAttributeName(char16_t c);

void errQuoteOrLtInAttributeNameOrNull(char16_t c);

void errExpectedPublicId();

void errBogusDoctype();

void maybeErrAttributesOnEndTag(nsHtml5HtmlAttributes* attrs);

void maybeErrSlashInEndTag(bool selfClosing);

char16_t errNcrNonCharacter(char16_t ch);

void errAstralNonCharacter(int32_t ch);

void errNcrSurrogate();

char16_t errNcrControlChar(char16_t ch);

void errNcrCr();

void errNcrInC1Range();

void errEofInPublicId();

void errEofInComment();

void errEofInDoctype();

void errEofInAttributeValue();

void errEofInAttributeName();

void errEofWithoutGt();

void errEofInTagName();

void errEofInEndTag();

void errEofAfterLt();

void errNcrOutOfRange();

void errNcrUnassigned();

void errDuplicateAttribute();

void errEofInSystemId();

void errExpectedSystemId();

void errMissingSpaceBeforeDoctypeName();

void errHyphenHyphenBang();

void errNcrControlChar();

void errNcrZero();

void errNoSpaceBetweenDoctypeSystemKeywordAndQuote();

void errNoSpaceBetweenPublicAndSystemIds();

void errNoSpaceBetweenDoctypePublicKeywordAndQuote();
