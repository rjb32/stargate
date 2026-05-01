#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Parser.h"

namespace stargate {

class Preprocessor;

class VerilogDriver {
public:
    VerilogDriver();
    ~VerilogDriver();

    void addIncludeDir(const std::string& dir);
    void defineMacro(const std::string& name, const std::string& body);

    int parseFile(const std::string& path);
    int parseString(const std::string& source);

    int preprocessFile(const std::string& path, std::string* out);

    const std::vector<std::string>& errors() const { return _errors; }
    bool hasErrors() const { return !_errors.empty(); }

    Parser::location_type& location() { return _location; }
    const std::string& filename() const { return _filename; }

    void setTrace(bool on) { _trace = on; }
    bool trace() const { return _trace; }

    void addError(const Parser::location_type& loc,
                  const std::string& msg);

private:
    Parser::location_type _location;
    std::vector<std::string> _errors;
    std::string _filename;
    bool _trace {false};
    std::unique_ptr<Preprocessor> _pp;

    int parseProcessed(const std::string& processed);
    int doParse();
    void copyPreprocessorErrors();
};

}
