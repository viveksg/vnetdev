#include "includes/registers.h"

uint32_t get_bytes(uint32_t shift, uint32_t length, uint32_t val)
{
    return ((val >> shift) & (1 << length));
}