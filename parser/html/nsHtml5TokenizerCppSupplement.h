




































void
nsHtml5Tokenizer::EnableViewSource(nsHtml5Highlighter* aHighlighter)
{
  mViewSource = aHighlighter;
}

bool
nsHtml5Tokenizer::FlushViewSource()
{
  return mViewSource->FlushOps();
}

void
nsHtml5Tokenizer::StartViewSource()
{
  mViewSource->Start();
}

void
nsHtml5Tokenizer::EndViewSource()
{
  mViewSource->End();
}
