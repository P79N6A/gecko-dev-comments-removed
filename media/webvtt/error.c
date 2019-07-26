


























#include <webvtt/error.h>

static const char *errstr[] = {
   "error allocating object",
   "malformed 'WEBVTT' tag",
   "expected newline",
   "expected whitespace",
   "unexpected whitespace",
   "very long tag-comment",
   "webvtt-cue-id truncated",
   "malformed webvtt-timestamp",
   "expected webvtt-timestamp",
   "missing webvtt-cuetime-separator `-->'",
   "expected webvtt-cuetime-separator `-->'",
   "missing whitespace before webvtt-cuesetting",
   "invalid webvtt-cuesetting key:value delimiter. expected `:'",
   "webvtt-cue end-time must have value greater than start-time",
   "unrecognized webvtt-cue-setting",
   "unfinished webvtt cuetimes. expected 'start-timestamp --> end-timestamp'",
   "missing setting keyword for value",
   "'vertical' cue-setting already used",
   "'vertical' setting must have a value of either 'lr' or 'rl'",
   "'line' cue-setting already used",
   "'line' cue-setting must have a value that is an integer (signed) line number, or percentage (%) from top of video display",
   "'position' cue-setting already used",
   "'position' cue-setting must be a percentage (%) value representing the position in the direction orthogonal to the 'line' setting",
   "'size' cue-setting already used",
   "'size' cue-setting must have percentage (%) value",
   "'align' cue-setting already used",
   "'align' cue-setting must have a value of either 'start', 'middle', or 'end'",
   "cue-text line contains unescaped timestamp separator '-->'",
   "cue contains cue-id, but is missing cuetimes or cue text",
};







WEBVTT_EXPORT const char *
webvtt_strerror( webvtt_error err )
{
  if( err >= (sizeof(errstr) / sizeof(*errstr)) ) {
    return "";
  }
  return errstr[ err ];
}
