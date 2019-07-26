





#ifndef _SECURITY_SANDBOX_SHIM_BASE_STRING_PIECE_H
#define _SECURITY_SANDBOX_SHIM_BASE_STRING_PIECE_H
#include "sandbox/base/strings/string_piece.h"

namespace {
bool IsStringASCII(const base::StringPiece& ascii)
{
  return IsStringASCII(ascii.as_string());
}
}
#endif
