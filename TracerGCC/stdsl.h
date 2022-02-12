#pragma once

#ifndef __clang__
#include <source_location>
#else
#include <experimental/source_location>

namespace std {

using experimental::source_location;

}
#endif

