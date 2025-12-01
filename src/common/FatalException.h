#pragma once

#include <exception>
#include <string>

namespace stargate {

class FatalException : public std::exception {
public:
    explicit FatalException(const std::string& msg)
        : _msg(msg)
    {
    }

    explicit FatalException(std::string&& msg)
        : _msg(msg)
    {
    }

    const char* what() const noexcept override { return _msg.c_str(); }

private:
    std::string _msg;
};

}
