




#include "mozilla/Assertions.h"
#include "mozilla/BloomFilter.h"

#include <stddef.h>
#include <stdio.h>

using mozilla::BloomFilter;

class FilterChecker
{
  public:
    FilterChecker(uint32_t hash) : mHash(hash) { }

    uint32_t hash() const { return mHash; }

  private:
    uint32_t mHash;
};

int
main()
{
  BloomFilter<12, FilterChecker> *filter = new BloomFilter<12, FilterChecker>();
  MOZ_ASSERT(filter);

  FilterChecker one(1);
  FilterChecker two(0x20000);
  FilterChecker many(0x10000);
  FilterChecker multiple(0x20001);

  filter->add(&one);
  MOZ_ASSERT(filter->mightContain(&one),
             "Filter should contain 'one'");

  MOZ_ASSERT(!filter->mightContain(&multiple),
             "Filter claims to contain 'multiple' when it should not");

  MOZ_ASSERT(filter->mightContain(&many),
             "Filter should contain 'many' (false positive)");

  filter->add(&two);
  MOZ_ASSERT(filter->mightContain(&multiple),
             "Filter should contain 'multiple' (false positive)");

  
  filter->remove(&two);
  MOZ_ASSERT(!filter->mightContain(&multiple),
             "Filter claims to contain 'multiple' when it should not after two "
             "was removed");

  
  const size_t FILTER_SIZE = 255;
  for (size_t i = 0; i < FILTER_SIZE - 1; ++i)
    filter->add(&two);

  MOZ_ASSERT(filter->mightContain(&multiple),
             "Filter should contain 'multiple' after 'two' added lots of times "
             "(false positive)");

  for (size_t i = 0; i < FILTER_SIZE - 1; ++i)
    filter->remove(&two);

  MOZ_ASSERT(!filter->mightContain(&multiple),
             "Filter claims to contain 'multiple' when it should not after two "
             "was removed lots of times");

  
  for (size_t i = 0; i < FILTER_SIZE + 1; ++i)
    filter->add(&two);

  MOZ_ASSERT(filter->mightContain(&multiple),
             "Filter should contain 'multiple' after 'two' added lots more "
             "times (false positive)");

  for (size_t i = 0; i < FILTER_SIZE + 1; ++i)
    filter->remove(&two);

  MOZ_ASSERT(filter->mightContain(&multiple),
             "Filter claims to not contain 'multiple' even though we should "
             "have run out of space in the buckets (false positive)");
  MOZ_ASSERT(filter->mightContain(&two),
             "Filter claims to not contain 'two' even though we should have "
             "run out of space in the buckets (false positive)");

  filter->remove(&one);

  MOZ_ASSERT(!filter->mightContain(&one),
             "Filter should not contain 'one', because we didn't overflow its "
             "bucket");

  filter->clear();

  MOZ_ASSERT(!filter->mightContain(&multiple),
             "clear() failed to work");

  return 0;
}
