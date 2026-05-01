#pragma once

#include <string>

#include "FatalException.h"

namespace stargate {

class ParseException : public FatalException {
public:
    explicit ParseException(const std::string& msg)
        : FatalException(msg)
    {
    }
};

}
