#pragma once

#include <stdint.h>
#include <cstdlib>
#include <assert.h>

bool findPatternInModule(void *hMod, const char *pattern, bool scanCodeOnly, uintptr_t& virtualAddress);