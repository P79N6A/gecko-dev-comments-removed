#define MOZ_MUST_USE __attribute__((annotate("moz_must_use")))

class MOZ_MUST_USE MustUse {};
class MayUse {};

MustUse producesMustUse();
MustUse *producesMustUsePointer();
MustUse &producesMustUseRef();

MayUse producesMayUse();
MayUse *producesMayUsePointer();
MayUse &producesMayUseRef();

void use(MustUse*);
void use(MustUse&);
void use(MustUse&&);
void use(MayUse*);
void use(MayUse&);
void use(MayUse&&);
void use(bool);

void foo() {
  producesMustUse(); 
  producesMustUsePointer();
  producesMustUseRef(); 
  producesMayUse();
  producesMayUsePointer();
  producesMayUseRef();
  {
    producesMustUse(); 
    producesMustUsePointer();
    producesMustUseRef(); 
    producesMayUse();
    producesMayUsePointer();
    producesMayUseRef();
  }
  if (true) {
    producesMustUse(); 
    producesMustUsePointer();
    producesMustUseRef(); 
    producesMayUse();
    producesMayUsePointer();
    producesMayUseRef();
  } else {
    producesMustUse(); 
    producesMustUsePointer();
    producesMustUseRef(); 
    producesMayUse();
    producesMayUsePointer();
    producesMayUseRef();
  }

  if(true) producesMustUse(); 
  else producesMustUse(); 
  if(true) producesMustUsePointer();
  else producesMustUsePointer();
  if(true) producesMustUseRef(); 
  else producesMustUseRef(); 
  if(true) producesMayUse();
  else producesMayUse();
  if(true) producesMayUsePointer();
  else producesMayUsePointer();
  if(true) producesMayUseRef();
  else producesMayUseRef();

  while (true) producesMustUse(); 
  while (true) producesMustUsePointer();
  while (true) producesMustUseRef(); 
  while (true) producesMayUse();
  while (true) producesMayUsePointer();
  while (true) producesMayUseRef();

  do producesMustUse(); 
  while (true);
  do producesMustUsePointer();
  while (true);
  do producesMustUseRef(); 
  while (true);
  do producesMayUse();
  while (true);
  do producesMayUsePointer();
  while (true);
  do producesMayUseRef();
  while (true);

  for (;;) producesMustUse(); 
  for (;;) producesMustUsePointer();
  for (;;) producesMustUseRef(); 
  for (;;) producesMayUse();
  for (;;) producesMayUsePointer();
  for (;;) producesMayUseRef();

  for (producesMustUse();;); 
  for (producesMustUsePointer();;);
  for (producesMustUseRef();;); 
  for (producesMayUse();;);
  for (producesMayUsePointer();;);
  for (producesMayUseRef();;);

  for (;;producesMustUse()); 
  for (;;producesMustUsePointer());
  for (;;producesMustUseRef()); 
  for (;;producesMayUse());
  for (;;producesMayUsePointer());
  for (;;producesMayUseRef());

  use((producesMustUse(), false)); 
  use((producesMustUsePointer(), false));
  use((producesMustUseRef(), false)); 
  use((producesMayUse(), false));
  use((producesMayUsePointer(), false));
  use((producesMayUseRef(), false));

  switch (1) {
  case 1:
    producesMustUse(); 
    producesMustUsePointer();
    producesMustUseRef(); 
    producesMayUse();
    producesMayUsePointer();
    producesMayUseRef();
  case 2:
    producesMustUse(); 
  case 3:
    producesMustUsePointer();
  case 4:
    producesMustUseRef(); 
  case 5:
    producesMayUse();
  case 6:
    producesMayUsePointer();
  case 7:
    producesMayUseRef();
  default:
    producesMustUse(); 
    producesMustUsePointer();
    producesMustUseRef(); 
    producesMayUse();
    producesMayUsePointer();
    producesMayUseRef();
  }

  use(producesMustUse());
  use(producesMustUsePointer());
  use(producesMustUseRef());
  use(producesMayUse());
  use(producesMayUsePointer());
  use(producesMayUseRef());

  MustUse a = producesMustUse();
  MustUse *b = producesMustUsePointer();
  MustUse &c = producesMustUseRef();
  MayUse d = producesMayUse();
  MayUse *e = producesMayUsePointer();
  MayUse &f = producesMayUseRef();
}
