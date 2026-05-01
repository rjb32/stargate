#include "VerilogDriver.h"

#include <stdio.h>
#include <sstream>

#include "Parser.h"
#include "Preprocessor.h"

namespace stargate {

void scanBeginFile(VerilogDriver& drv, const std::string& path);
void scanBeginString(VerilogDriver& drv, const std::string& source);
void scanEnd();

VerilogDriver::VerilogDriver()
    : _pp(std::make_unique<Preprocessor>())
{
}

VerilogDriver::~VerilogDriver() {
}

void VerilogDriver::addIncludeDir(const std::string& dir) {
    _pp->addIncludeDir(dir);
}

void VerilogDriver::defineMacro(const std::string& name,
                                const std::string& body) {
    _pp->define(name, body);
}

int VerilogDriver::preprocessFile(const std::string& path,
                                  std::string* out) {
    _filename = path;
    _errors.clear();

    const int rc = _pp->processFile(path, out);
    copyPreprocessorErrors();
    return rc;
}

int VerilogDriver::parseFile(const std::string& path) {
    _filename = path;
    _errors.clear();

    std::string processed;
    const int prc = _pp->processFile(path, &processed);
    copyPreprocessorErrors();
    if (prc != 0) {
        return 1;
    }
    return parseProcessed(processed);
}

int VerilogDriver::parseString(const std::string& source) {
    _filename = "<string>";
    _errors.clear();

    std::string processed;
    const int prc = _pp->processString(source, _filename, &processed);
    copyPreprocessorErrors();
    if (prc != 0) {
        return 1;
    }
    return parseProcessed(processed);
}

int VerilogDriver::parseProcessed(const std::string& processed) {
    _location.initialize(&_filename);

    scanBeginString(*this, processed);
    const int rc = doParse();
    scanEnd();
    return rc;
}

int VerilogDriver::doParse() {
    Parser parser(*this);
    parser.set_debug_level(_trace ? 1 : 0);
    const int rc = parser.parse();
    if (rc != 0 || hasErrors()) {
        return 1;
    }
    return 0;
}

void VerilogDriver::addError(const Parser::location_type& loc,
                             const std::string& msg) {
    std::ostringstream oss;
    oss << loc << ": " << msg;
    _errors.push_back(oss.str());
}

void VerilogDriver::copyPreprocessorErrors() {
    for (const auto& e : _pp->errors()) {
        _errors.push_back(e);
    }
}

}
