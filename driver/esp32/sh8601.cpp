#include "sh8601.hpp"

#include "py/runtime.h"

void SH8601_QSPI::printSelf() {
  mp_printf(&mp_plat_print, "12345\n\n");
  mp_printf(&mp_plat_print, "56789\n\n");
}
