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

    // SV-only keywords (logic, int, bit, always_ff, ...) shadow what
    // were plain identifiers in Verilog-2001. We default to enabled
    // and disable per-file when the path ends in `.v`, so legacy
    // Verilog-2001 sources that use names like `int` or `logic` as
    // identifiers keep parsing.
    void setSvKeywords(bool on) { _svKeywords = on; }
    bool svKeywords() const { return _svKeywords; }

    void addError(const Parser::location_type& loc,
                  const std::string& msg);

private:
    Parser::location_type _location;
    std::vector<std::string> _errors;
    std::string _filename;
    bool _trace {false};
    bool _svKeywords {true};
    std::unique_ptr<Preprocessor> _pp;

    int parseProcessed(const std::string& processed);
    int doParse();
    void copyPreprocessorErrors();
};

}
