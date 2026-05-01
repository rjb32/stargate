#include "Preprocessor.h"

#include <stdio.h>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace stargate {

Preprocessor::Preprocessor() {
}

Preprocessor::~Preprocessor() {
}

void Preprocessor::addIncludeDir(const std::string& dir) {
    _includeDirs.push_back(dir);
}

void Preprocessor::define(const std::string& name, const std::string& body) {
    Macro m;
    m.body = body;
    m.isFunctionLike = false;
    _macros[name] = m;
}

void Preprocessor::undefine(const std::string& name) {
    _macros.erase(name);
}

bool Preprocessor::isActive() const {
    for (const auto& f : _condStack) {
        if (!f.active) {
            return false;
        }
    }
    return true;
}

bool Preprocessor::parentActive() const {
    if (_condStack.size() < 2) {
        return true;
    }
    for (size_t k = 0; k + 1 < _condStack.size(); ++k) {
        if (!_condStack[k].active) {
            return false;
        }
    }
    return true;
}

void Preprocessor::addError(const std::string& filename,
                            int line,
                            const std::string& msg) {
    std::ostringstream oss;
    oss << filename << ":" << line << ": " << msg;
    _errors.push_back(oss.str());
}

bool Preprocessor::isIdStart(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

bool Preprocessor::isIdCont(char c) {
    return isIdStart(c) ||
           (c >= '0' && c <= '9') ||
            c == '$';
}

std::string Preprocessor::readIdent(const std::string& src, size_t& i) {
    const size_t start = i;
    while (i < src.size() && isIdCont(src[i])) {
        ++i;
    }
    return src.substr(start, i - start);
}

void Preprocessor::skipSpaces(const std::string& src, size_t& i) {
    while (i < src.size() && (src[i] == ' ' || src[i] == '\t')) {
        ++i;
    }
}

int Preprocessor::skipToEol(const std::string& src, size_t& i) {
    int newlines = 0;
    while (i < src.size() && src[i] != '\n') {
        if (src[i] == '\\' && i + 1 < src.size() && src[i + 1] == '\n') {
            i += 2;
            ++newlines;
            continue;
        }
        ++i;
    }
    return newlines;
}

std::string Preprocessor::readDirectiveLine(const std::string& src,
                                            size_t& i,
                                            int* lineDelta) {
    std::string body;
    *lineDelta = 0;
    while (i < src.size() && src[i] != '\n') {
        if (src[i] == '\\' && i + 1 < src.size() && src[i + 1] == '\n') {
            body.push_back('\n');
            i += 2;
            ++*lineDelta;
            continue;
        }
        body.push_back(src[i]);
        ++i;
    }
    return body;
}

void Preprocessor::skipBalancedParens(const std::string& src,
                                      size_t& i,
                                      int& line) {
    if (i >= src.size() || src[i] != '(') {
        return;
    }
    int depth = 0;
    while (i < src.size()) {
        const char c = src[i];
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
            if (depth == 0) {
                ++i;
                return;
            }
        } else if (c == '\n') {
            ++line;
        } else if (c == '"') {
            ++i;
            while (i < src.size() && src[i] != '"' && src[i] != '\n') {
                if (src[i] == '\\' && i + 1 < src.size()) {
                    ++i;
                }
                ++i;
            }
        }
        ++i;
    }
}

bool Preprocessor::readMacroArgs(const std::string& src,
                                 size_t& i,
                                 int& line,
                                 std::vector<std::string>* args) {
    if (i >= src.size() || src[i] != '(') {
        return false;
    }
    ++i;

    int depth = 1;
    std::string current;

    auto pushArg = [&]() {
        size_t a = 0;
        while (a < current.size() &&
               (current[a] == ' ' || current[a] == '\t' || current[a] == '\n')) {
            ++a;
        }
        size_t b = current.size();
        while (b > a &&
               (current[b - 1] == ' ' || current[b - 1] == '\t' ||
                current[b - 1] == '\n')) {
            --b;
        }
        args->push_back(current.substr(a, b - a));
        current.clear();
    };

    while (i < src.size()) {
        const char c = src[i];
        if (c == '(') {
            ++depth;
            current.push_back(c);
            ++i;
        } else if (c == ')') {
            --depth;
            if (depth == 0) {
                pushArg();
                ++i;
                if (args->size() == 1 && args->front().empty()) {
                    args->clear();
                }
                return true;
            }
            current.push_back(c);
            ++i;
        } else if (c == ',' && depth == 1) {
            pushArg();
            ++i;
        } else if (c == '"') {
            current.push_back(c);
            ++i;
            while (i < src.size() && src[i] != '"' && src[i] != '\n') {
                if (src[i] == '\\' && i + 1 < src.size()) {
                    current.push_back(src[i]);
                    ++i;
                }
                current.push_back(src[i]);
                ++i;
            }
            if (i < src.size() && src[i] == '"') {
                current.push_back(src[i]);
                ++i;
            }
        } else if (c == '\n') {
            ++line;
            current.push_back(c);
            ++i;
        } else {
            current.push_back(c);
            ++i;
        }
    }
    return false;
}

std::string Preprocessor::substituteParams(
    const Macro* m,
    const std::vector<std::string>& args) {
    if (!m->isFunctionLike || m->params.empty()) {
        return m->body;
    }

    std::string out;
    const std::string& body = m->body;
    size_t i = 0;

    while (i < body.size()) {
        const char c = body[i];
        if (c == '"') {
            const size_t start = i;
            ++i;
            while (i < body.size() && body[i] != '"' && body[i] != '\n') {
                if (body[i] == '\\' && i + 1 < body.size()) {
                    ++i;
                }
                ++i;
            }
            if (i < body.size() && body[i] == '"') {
                ++i;
            }
            out.append(body, start, i - start);
            continue;
        }
        if (c == '/' && i + 1 < body.size() && body[i + 1] == '/') {
            const size_t start = i;
            while (i < body.size() && body[i] != '\n') {
                ++i;
            }
            out.append(body, start, i - start);
            continue;
        }
        if (isIdStart(c)) {
            const size_t start = i;
            while (i < body.size() && isIdCont(body[i])) {
                ++i;
            }
            std::string token = body.substr(start, i - start);
            ssize_t found = -1;
            for (size_t p = 0; p < m->params.size(); ++p) {
                if (m->params[p] == token) {
                    found = static_cast<ssize_t>(p);
                    break;
                }
            }
            if (found >= 0 && static_cast<size_t>(found) < args.size()) {
                out.append(args[found]);
            } else {
                out.append(token);
            }
            continue;
        }
        out.push_back(c);
        ++i;
    }
    return out;
}

bool Preprocessor::resolveIncludePath(const std::string& referenced,
                                      const std::string& fromFile,
                                      std::string* resolved) {
    namespace fs = std::filesystem;

    fs::path ref(referenced);
    if (ref.is_absolute() && fs::exists(ref)) {
        *resolved = ref.string();
        return true;
    }

    if (!fromFile.empty()) {
        fs::path fromDir = fs::path(fromFile).parent_path();
        fs::path candidate = fromDir / ref;
        if (fs::exists(candidate)) {
            *resolved = candidate.string();
            return true;
        }
    }

    for (const auto& dir : _includeDirs) {
        fs::path candidate = fs::path(dir) / ref;
        if (fs::exists(candidate)) {
            *resolved = candidate.string();
            return true;
        }
    }
    return false;
}

void Preprocessor::handleDefine(const std::string& src,
                                size_t& i,
                                const std::string& filename,
                                int& line) {
    skipSpaces(src, i);
    const std::string name = readIdent(src, i);
    if (name.empty()) {
        addError(filename, line, "`define without macro name");
        skipToEol(src, i);
        return;
    }

    Macro m;

    if (i < src.size() && src[i] == '(') {
        m.isFunctionLike = true;
        ++i;
        skipSpaces(src, i);
        while (i < src.size() && src[i] != ')') {
            const std::string param = readIdent(src, i);
            if (param.empty()) {
                addError(filename, line, "expected parameter name in `define");
                break;
            }
            m.params.push_back(param);
            skipSpaces(src, i);
            if (i < src.size() && src[i] == ',') {
                ++i;
                skipSpaces(src, i);
            } else {
                break;
            }
        }
        if (i < src.size() && src[i] == ')') {
            ++i;
        } else {
            addError(filename, line, "expected ')' in `define parameter list");
        }
    }

    skipSpaces(src, i);

    int lineDelta = 0;
    std::string raw = readDirectiveLine(src, i, &lineDelta);
    line += lineDelta;

    // Strip a trailing line comment from the macro body — per the
    // LRM, "//..." within a `define directive is not part of the body.
    // Be careful not to confuse "//" inside a string literal.
    std::string clean;
    bool inStr = false;
    for (size_t k = 0; k < raw.size(); ++k) {
        const char c = raw[k];
        if (!inStr && c == '/' && k + 1 < raw.size() && raw[k + 1] == '/') {
            break;
        }
        if (c == '"') {
            inStr = !inStr;
        } else if (c == '\\' && inStr && k + 1 < raw.size()) {
            clean.push_back(c);
            clean.push_back(raw[++k]);
            continue;
        }
        clean.push_back(c);
    }

    while (!clean.empty() &&
           (clean.back() == ' ' || clean.back() == '\t' ||
            clean.back() == '\n')) {
        clean.pop_back();
    }
    m.body = clean;

    _macros[name] = m;
}

void Preprocessor::handleInclude(const std::string& src,
                                 size_t& i,
                                 const std::string& filename,
                                 int line,
                                 std::string* out) {
    skipSpaces(src, i);

    std::string requested;
    if (i < src.size() && (src[i] == '"' || src[i] == '<')) {
        const char close = (src[i] == '"') ? '"' : '>';
        ++i;
        while (i < src.size() && src[i] != close && src[i] != '\n') {
            requested.push_back(src[i]);
            ++i;
        }
        if (i < src.size() && src[i] == close) {
            ++i;
        } else {
            addError(filename, line, "unterminated `include path");
        }
    } else {
        addError(filename, line, "`include path must be quoted");
    }
    skipToEol(src, i);

    if (requested.empty()) {
        return;
    }

    std::string resolved;
    if (!resolveIncludePath(requested, filename, &resolved)) {
        addError(filename, line,
            "cannot find included file: " + requested);
        return;
    }

    if (_includeDepth >= MAX_INCLUDE_DEPTH) {
        addError(filename, line, "include depth limit exceeded");
        return;
    }

    std::ifstream f(resolved);
    if (!f) {
        addError(filename, line,
            "cannot open included file: " + resolved);
        return;
    }
    std::ostringstream oss;
    oss << f.rdbuf();

    ++_includeDepth;
    processSource(oss.str(), resolved, out);
    --_includeDepth;
}

void Preprocessor::processSource(const std::string& src,
                                 const std::string& filename,
                                 std::string* out) {
    int line = 1;
    size_t i = 0;

    auto emit = [&](const std::string& s) {
        if (isActive()) {
            out->append(s);
        }
    };
    auto emitChar = [&](char c) {
        if (isActive()) {
            out->push_back(c);
        }
    };

    while (i < src.size()) {
        const char c = src[i];

        if (c == '\n') {
            ++line;
            emitChar(c);
            ++i;
            continue;
        }

        if (c == '"') {
            const size_t start = i;
            ++i;
            while (i < src.size() && src[i] != '"' && src[i] != '\n') {
                if (src[i] == '\\' && i + 1 < src.size()) {
                    ++i;
                }
                ++i;
            }
            if (i < src.size() && src[i] == '"') {
                ++i;
            }
            emit(src.substr(start, i - start));
            continue;
        }

        if (c == '/' && i + 1 < src.size() && src[i + 1] == '/') {
            const size_t start = i;
            while (i < src.size() && src[i] != '\n') {
                ++i;
            }
            emit(src.substr(start, i - start));
            continue;
        }

        if (c == '/' && i + 1 < src.size() && src[i + 1] == '*') {
            const size_t start = i;
            i += 2;
            while (i + 1 < src.size() &&
                   !(src[i] == '*' && src[i + 1] == '/')) {
                if (src[i] == '\n') {
                    ++line;
                }
                ++i;
            }
            if (i + 1 < src.size()) {
                i += 2;
            }
            emit(src.substr(start, i - start));
            continue;
        }

        if (c != '`') {
            emitChar(c);
            ++i;
            continue;
        }

        ++i;
        const std::string name = readIdent(src, i);
        if (name.empty()) {
            emitChar('`');
            continue;
        }

        if (name == "ifdef" || name == "ifndef") {
            skipSpaces(src, i);
            const std::string macroName = readIdent(src, i);
            int delta = 0;
            readDirectiveLine(src, i, &delta);
            line += delta;
            const bool defined = _macros.count(macroName) > 0;
            const bool branchTrue = (name == "ifdef") ? defined : !defined;
            const bool here = parentActive() && isActive() && branchTrue;
            CondFrame f {here, branchTrue, false};
            _condStack.push_back(f);
            continue;
        }

        if (name == "else") {
            int delta = 0;
            readDirectiveLine(src, i, &delta);
            line += delta;
            if (_condStack.empty()) {
                addError(filename, line, "`else without matching `if[n]def");
                continue;
            }
            CondFrame& f = _condStack.back();
            if (f.seenElse) {
                addError(filename, line, "duplicate `else");
                continue;
            }
            f.seenElse = true;
            f.active = parentActive() && !f.seenTrueBranch;
            if (f.active) {
                f.seenTrueBranch = true;
            }
            continue;
        }

        if (name == "elsif") {
            skipSpaces(src, i);
            const std::string macroName = readIdent(src, i);
            int delta = 0;
            readDirectiveLine(src, i, &delta);
            line += delta;
            if (_condStack.empty()) {
                addError(filename, line,
                    "`elsif without matching `if[n]def");
                continue;
            }
            CondFrame& f = _condStack.back();
            if (f.seenElse) {
                addError(filename, line, "`elsif after `else");
                continue;
            }
            const bool defined = _macros.count(macroName) > 0;
            if (f.seenTrueBranch || !defined) {
                f.active = false;
            } else {
                f.active = parentActive();
                if (f.active) {
                    f.seenTrueBranch = true;
                }
            }
            continue;
        }

        if (name == "endif") {
            int delta = 0;
            readDirectiveLine(src, i, &delta);
            line += delta;
            if (_condStack.empty()) {
                addError(filename, line,
                    "`endif without matching `if[n]def");
                continue;
            }
            _condStack.pop_back();
            continue;
        }

        if (!isActive()) {
            if (i < src.size() && src[i] == '(') {
                int dummy = line;
                skipBalancedParens(src, i, dummy);
                line = dummy;
            }
            continue;
        }

        if (name == "define") {
            handleDefine(src, i, filename, line);
            continue;
        }
        if (name == "undef") {
            skipSpaces(src, i);
            const std::string macroName = readIdent(src, i);
            int delta = 0;
            readDirectiveLine(src, i, &delta);
            line += delta;
            _macros.erase(macroName);
            continue;
        }
        if (name == "include") {
            handleInclude(src, i, filename, line, out);
            continue;
        }

        if (name == "timescale" || name == "celldefine" ||
            name == "endcelldefine" || name == "resetall" ||
            name == "default_nettype" ||
            name == "unconnected_drive" ||
            name == "nounconnected_drive" || name == "pragma" ||
            name == "line" || name == "begin_keywords" ||
            name == "end_keywords" || name == "error" ||
            name == "warning" || name == "info" || name == "note" ||
            name == "protect" || name == "endprotect" ||
            name == "protected" || name == "endprotected") {
            int delta = 0;
            readDirectiveLine(src, i, &delta);
            line += delta;
            continue;
        }

        const auto it = _macros.find(name);
        if (it == _macros.end()) {
            // Undefined macro. If it looks function-like (parens
            // follow immediately), strip the args wholesale — many
            // such macros expand to a self-contained statement, and
            // their argument text often contains characters the
            // grammar can't parse standalone (semicolons, blocks).
            // Bare references stay as `<name> for the lexer to
            // surface as MACRO_REF.
            if (i < src.size() && src[i] == '(') {
                int dummy = line;
                skipBalancedParens(src, i, dummy);
                line = dummy;

                // Eat an optional trailing ; — most call-sites of
                // these macros use them as a statement.
                size_t j = i;
                while (j < src.size() &&
                       (src[j] == ' ' || src[j] == '\t')) {
                    ++j;
                }
                if (j < src.size() && src[j] == ';') {
                    i = j + 1;
                }
            } else {
                emitChar('`');
                emit(name);
            }
            continue;
        }

        const Macro& m = it->second;
        std::vector<std::string> args;
        if (m.isFunctionLike) {
            if (i < src.size() && src[i] == '(') {
                if (!readMacroArgs(src, i, line, &args)) {
                    addError(filename, line,
                        "malformed arguments to macro `" + name);
                    continue;
                }
            } else {
                addError(filename, line,
                    "function-like macro `" + name +
                    " requires arguments");
                continue;
            }
        }

        const std::string substituted = substituteParams(&m, args);

        if (_expansionDepth >= MAX_EXPANSION_DEPTH) {
            addError(filename, line,
                "macro expansion depth exceeded for `" + name);
            continue;
        }
        ++_expansionDepth;
        std::string nested;
        processSource(substituted,
                      filename + " (in macro `" + name + ")",
                      &nested);
        --_expansionDepth;
        emit(nested);
    }
}

int Preprocessor::processFile(const std::string& path, std::string* out) {
    std::ifstream f(path);
    if (!f) {
        addError(path, 1, "cannot open file");
        return 1;
    }
    std::ostringstream oss;
    oss << f.rdbuf();
    return processString(oss.str(), path, out);
}

int Preprocessor::processString(const std::string& source,
                                const std::string& sourceName,
                                std::string* out) {
    out->clear();
    _condStack.clear();
    processSource(source, sourceName, out);
    return _errors.empty() ? 0 : 1;
}

}
