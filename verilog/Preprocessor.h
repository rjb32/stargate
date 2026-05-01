#pragma once

#include <stddef.h>
#include <map>
#include <string>
#include <vector>

namespace stargate {

class Preprocessor {
public:
    Preprocessor();
    ~Preprocessor();

    void addIncludeDir(const std::string& dir);
    void define(const std::string& name, const std::string& body);
    void undefine(const std::string& name);

    int processFile(const std::string& path, std::string* out);
    int processString(const std::string& source,
                      const std::string& sourceName,
                      std::string* out);

    const std::vector<std::string>& errors() const { return _errors; }
    bool hasErrors() const { return !_errors.empty(); }

private:
    struct Macro {
        std::vector<std::string> params;
        std::string body;
        bool isFunctionLike {false};
    };

    struct CondFrame {
        bool active;
        bool seenTrueBranch;
        bool seenElse;
    };

    std::map<std::string, Macro> _macros;
    std::vector<std::string> _includeDirs;
    std::vector<CondFrame> _condStack;
    std::vector<std::string> _errors;
    int _includeDepth {0};
    int _expansionDepth {0};

    static constexpr int MAX_INCLUDE_DEPTH = 64;
    static constexpr int MAX_EXPANSION_DEPTH = 256;

    bool isActive() const;
    bool parentActive() const;

    void addError(const std::string& filename,
                  int line,
                  const std::string& msg);

    void processSource(const std::string& src,
                       const std::string& filename,
                       std::string* out);

    void handleDefine(const std::string& src,
                      size_t& i,
                      const std::string& filename,
                      int& line);
    void handleInclude(const std::string& src,
                       size_t& i,
                       const std::string& filename,
                       int line,
                       std::string* out);
    bool readMacroArgs(const std::string& src,
                       size_t& i,
                       int& line,
                       std::vector<std::string>* args);
    std::string substituteParams(const Macro* m,
                                 const std::vector<std::string>& args);
    void skipBalancedParens(const std::string& src,
                            size_t& i,
                            int& line);

    static bool isIdStart(char c);
    static bool isIdCont(char c);
    static std::string readIdent(const std::string& src, size_t& i);
    static void skipSpaces(const std::string& src, size_t& i);
    static int skipToEol(const std::string& src, size_t& i);
    static std::string readDirectiveLine(const std::string& src, size_t& i,
                                         int* lineDelta);

    bool resolveIncludePath(const std::string& referenced,
                            const std::string& fromFile,
                            std::string* resolved);
};

}
