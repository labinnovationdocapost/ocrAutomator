#pragma once
#include <signal.h>

void segfault_action(int sig, siginfo_t *info, void *secret);

void CatchAllErrorSignals();
void CatchAllExceptions();

#ifdef TEST
void GenerateSIGSEGV();
#else
void GenerateSIGSEGV() __attribute__((error("DO NOT USE OUTSIDE OFF TEST CASE !!!")));
#endif // TEST