#include "usim/common.h"
#include <assert.h>

void usim_etu(uint32_t *const etu, uint16_t const fi, uint8_t const di,
              uint32_t const fmax)
{
    assert(fmax != 0U);
    assert(di != 0U);
    *etu = fi / (di * fmax);
}
