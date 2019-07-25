





















#define nsHtml5NamedCharacters_cpp_
#include "prtypes.h"
#include "jArray.h"
#include "nscore.h"
#include "nsDebug.h"
#include "prlog.h"
#include "nsMemory.h"

#include "nsHtml5NamedCharacters.h"

const PRUnichar nsHtml5NamedCharacters::VALUES[][2] = {
#define NAMED_CHARACTER_REFERENCE(N, CHARS, LEN, FLAG, VALUE) \
{ VALUE },
#include "nsHtml5NamedCharactersInclude.h"
#undef NAMED_CHARACTER_REFERENCE
{0, 0} };

PRUnichar** nsHtml5NamedCharacters::WINDOWS_1252;
static PRUnichar const WINDOWS_1252_DATA[] = {
  0x20AC,
  0x0081,
  0x201A,
  0x0192,
  0x201E,
  0x2026,
  0x2020,
  0x2021,
  0x02C6,
  0x2030,
  0x0160,
  0x2039,
  0x0152,
  0x008D,
  0x017D,
  0x008F,
  0x0090,
  0x2018,
  0x2019,
  0x201C,
  0x201D,
  0x2022,
  0x2013,
  0x2014,
  0x02DC,
  0x2122,
  0x0161,
  0x203A,
  0x0153,
  0x009D,
  0x017E,
  0x0178
};









static const int8_t ALL_NAMES[] = {
#define NAMED_CHARACTER_REFERENCE(N, CHARS, LEN, FLAG, VALUE) \
CHARS ,
#include "nsHtml5NamedCharactersInclude.h"
#undef NAMED_CHARACTER_REFERENCE
};

enum NamePositions {
  DUMMY_INITIAL_NAME_POSITION = 0,

#define NAMED_CHARACTER_REFERENCE(N, CHARS, LEN, FLAG, VALUE) \
NAME_##N##_DUMMY, /* automatically one higher than previous */ \
NAME_##N##_START = NAME_##N##_DUMMY - 1, \
NAME_##N##_END = NAME_##N##_START + LEN + FLAG,
#include "nsHtml5NamedCharactersInclude.h"
#undef NAMED_CHARACTER_REFERENCE
  DUMMY_FINAL_NAME_VALUE
};


PR_STATIC_ASSERT(NS_ARRAY_LENGTH(ALL_NAMES) < 0x10000);

const nsHtml5CharacterName nsHtml5NamedCharacters::NAMES[] = {
#ifdef DEBUG
  #define NAMED_CHARACTER_REFERENCE(N, CHARS, LEN, FLAG, VALUE) \
{ NAME_##N##_START, LEN, N },
#else
  #define NAMED_CHARACTER_REFERENCE(N, CHARS, LEN, FLAG, VALUE) \
{ NAME_##N##_START, LEN, },
#endif
#include "nsHtml5NamedCharactersInclude.h"
#undef NAMED_CHARACTER_REFERENCE
};

int32_t
nsHtml5CharacterName::length() const
{
  return nameLen;
}

PRUnichar
nsHtml5CharacterName::charAt(int32_t index) const
{
  return static_cast<PRUnichar> (ALL_NAMES[nameStart + index]);
}

void
nsHtml5NamedCharacters::initializeStatics()
{
  WINDOWS_1252 = new PRUnichar*[32];
  for (int32_t i = 0; i < 32; ++i) {
    WINDOWS_1252[i] = (PRUnichar*)&(WINDOWS_1252_DATA[i]);
  }
}

void
nsHtml5NamedCharacters::releaseStatics()
{
  delete[] WINDOWS_1252;
}
