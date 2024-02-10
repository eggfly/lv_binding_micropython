extern "C" {

#include "py_sh8601.h"

#include "py/runtime.h"
#include "sh8601.hpp"

void sh8601_flush(void *disp_drv, const void *area, void *color_p) {
  SH8601_QSPI display;
  mp_printf(&mp_plat_print, "1234\n\n");
  mp_printf(&mp_plat_print, "5678\n\n");
}
}
