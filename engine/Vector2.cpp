//
// Created by alex on 6/27/15.
//

#include "Vector2.h"

sincos_cached<float,  FLOAT_PREC>&   inst1 = Vector2<float,FLOAT_PREC>::sincos;
sincos_cached<double, DOUBLE_PREC>& inst2  = Vector2<double,DOUBLE_PREC>::sincos;