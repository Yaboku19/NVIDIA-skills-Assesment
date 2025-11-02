#include "wasm_parser.hpp"
#include "wasm_memory.hpp"
#include <iostream>
#include <sstream>
#include <algorithm> 
#include <regex>

void WasmParser::parseModule(std::unordered_map<std::string, WasmGlobal>& globals) {
    globals.clear();

    std::cout << "\033[1;32m[parser:parseModule]\033[0m New module initialized. Globals cleared.\n";
}

void WasmParser::parseGlobal(const std::string& line, std::unordered_map<std::string, WasmGlobal>& globals) {
    std::cout << "\033[1;32m[parser:parseGlobal]\033[0m Global definition found: " << line << "\n";

    std::istringstream iss(line);
    std::string token, name, typeStr, mutStr;
    bool isMutable = false;
    ValueType type = ValueType::I32;
    WasmValue initValue;

    iss >> token;
    iss >> name;
    iss >> mutStr;

    if (mutStr == "(mut") {
        isMutable = true;
        iss >> typeStr;
        if (!typeStr.empty() && typeStr.back() == ')')
            typeStr.pop_back();
    } else {
        typeStr = mutStr;
    }

    if (typeStr == "i32") type = ValueType::I32;
    else if (typeStr == "i64") type = ValueType::I64;
    else if (typeStr == "f32") type = ValueType::F32;
    else if (typeStr == "f64") type = ValueType::F64;
    else {
        std::cerr << "\033[1;31m[parser:parseGlobal]\033[0m Unknown type " << typeStr << "\n";
        return;
    }

    std::string constType;
    iss >> constType;

    if (constType.find("i32.const") != std::string::npos) {
        int32_t val; iss >> val;
        initValue = WasmValue(val);
    } else if (constType.find("i64.const") != std::string::npos) {
        int64_t val; iss >> val;
        initValue = WasmValue(val);
    } else if (constType.find("f32.const") != std::string::npos) {
        float val; iss >> val;
        initValue = WasmValue(val);
    } else if (constType.find("f64.const") != std::string::npos) {
        double val; iss >> val;
        initValue = WasmValue(val);
    } else {
        std::cerr << "\033[1;31m[parser:parseGlobal]\033[0m Unknown init constant type: " << constType << "\n";
        return;
    }

    WasmGlobal g{name, type, isMutable, initValue};
    globals[name] = g;

    std::cout << "\033[1;32m[parser:parseGlobal]\033[0m Global "
              << name << " (type=" << typeStr
              << ", mutable=" << (isMutable ? "true" : "false")
              << ") = ";

    switch (type) {
        case ValueType::I32: std::cout << initValue.i32; break;
        case ValueType::I64: std::cout << initValue.i64; break;
        case ValueType::F32: std::cout << initValue.f32; break;
        case ValueType::F64: std::cout << initValue.f64; break;
    }
    std::cout << "\n";
}

void WasmParser::print_globals(const std::unordered_map<std::string, WasmGlobal>& globals) const {
    std::cout << "\033[1;32m[parser:print_globals]\033[0m Global variables state:\n";

    if (globals.empty()) {
        std::cout << "  (empty)\n";
        return;
    }

    for (const auto& [name, g] : globals) {
        std::cout << "  " << name << " (type=";

        switch (g.type) {
            case ValueType::I32: std::cout << "i32"; break;
            case ValueType::I64: std::cout << "i64"; break;
            case ValueType::F32: std::cout << "f32"; break;
            case ValueType::F64: std::cout << "f64"; break;
        }

        std::cout << ", mutable=" << (g.mutableFlag ? "true" : "false")
                  << ") = ";

        switch (g.type) {
            case ValueType::I32: std::cout << g.value.i32; break;
            case ValueType::I64: std::cout << g.value.i64; break;
            case ValueType::F32: std::cout << g.value.f32; break;
            case ValueType::F64: std::cout << g.value.f64; break;
        }

        std::cout << "\n";
    }
}

void WasmParser::parseType(const std::string& line, std::unordered_map<int, FuncType>& funcTypes) {
    std::cout << "\033[1;32m[parser:parseType]\033[0m Type definition found: " << line << "\n";

    std::istringstream iss(line);
    std::string temp, typeIndexStr, token;
    FuncType funcType;

    iss >> temp;
    iss >> typeIndexStr;

    std::string digitsOnly;
    for (char c : typeIndexStr) {
        if (std::isdigit(c) || c == '-' || c == '+')
            digitsOnly += c;
    }

    int typeIndex = -1;
    if (!digitsOnly.empty()) {
        try {
            typeIndex = std::stoi(digitsOnly);
        } catch (...) {
            typeIndex = -1;
        }
    }
    size_t paramStart = line.find("(param");
    if (paramStart != std::string::npos) {
        size_t paramEnd = line.find(")", paramStart);
        if (paramEnd != std::string::npos) {
            std::string paramsPart = line.substr(paramStart + 6, paramEnd - (paramStart + 6));
            std::istringstream pss(paramsPart);
            std::string paramType;
            while (pss >> paramType) {
                funcType.params.push_back(paramType);
            }
        }
    }

    size_t resultStart = line.find("(result");
    if (resultStart != std::string::npos) {
        size_t resultEnd = line.find(")", resultStart);
        if (resultEnd != std::string::npos) {
            std::string resultPart = line.substr(resultStart + 7, resultEnd - (resultStart + 7));
            resultPart.erase(0, resultPart.find_first_not_of(" \t"));
            resultPart.erase(resultPart.find_last_not_of(" \t") + 1);
            funcType.resultType = resultPart;
        }
    }
    if (funcType.resultType.empty()) {
        funcType.resultType = "void"; 
    }
    funcTypes[typeIndex] = funcType;
    std::cout << "\033[1;32m[parser:parseType]\033[0m Type parsed → Index: " << typeIndex << ", Params: [";
    for (const auto& p : funcType.params) {
        std::cout << p << " ";
    }
    std::cout << "], Results: " << funcType.resultType << "\n";
}

FuncDef WasmParser::parseFunction(
    const std::string& line,
    std::unordered_map<int, FuncDef>& functionsByID,
    std::unordered_map<std::string, FuncDef>& functionByName,
    const std::unordered_map<int, FuncType>& funcTypes)
{
    std::cout << "\033[1;32m[parser:parseFunction]\033[0m Function definition found: " << line << "\n";
    FuncDef func;
    std::string token;
    std::istringstream iss(line);
    int anonCounter = 1; 

    // ✅ PRE-SCAN: se la riga contiene (param ...), NON importiamo i param dal (type N)
    const bool hasExplicitParams = (line.find("(param") != std::string::npos);

    while (iss >> token) {
        if (token == "(func") continue;

        if (token.rfind("(;", 0) == 0 && token.find(';') != std::string::npos) {
            try {
                size_t start = token.find(';') + 1;
                size_t end = token.find(';', start);
                func.index = std::stoi(token.substr(start, end - start));
            } catch (...) {
                func.index = -1;
            }
        }

        else if (!token.empty() && token[0] == '$') {
            func.name = token;
        }

        else if (token == "(type") {
            std::string typeIndex;
            if (iss >> typeIndex) {
                typeIndex.erase(remove(typeIndex.begin(), typeIndex.end(), ')'), typeIndex.end());
                int t = std::stoi(typeIndex);

                auto it = funcTypes.find(t);
                if (it != funcTypes.end()) {
                    const FuncType& ftype = it->second;

                    // ⛔️ PRIMA importavi sempre: ora lo fai SOLO se non ci sono (param ...)
                    if (!hasExplicitParams && func.params.empty()) {
                        for (const auto& ptype : ftype.params) {
                            WasmValue val;
                            if (ptype == "i32") val = WasmValue(int32_t(0));
                            else if (ptype == "i64") val = WasmValue(int64_t(0));
                            else if (ptype == "f32") val = WasmValue(float(0));
                            else if (ptype == "f64") val = WasmValue(double(0));

                            std::string anonName = "param_" + std::to_string(anonCounter++);
                            func.params[anonName] = val;
                        }
                    }

                    // Il result del type lo importiamo comunque se presente
                    if (!ftype.resultType.empty() && ftype.resultType != "void") {
                        if (ftype.resultType == "i32") func.result = WasmValue(int32_t(0));
                        else if (ftype.resultType == "i64") func.result = WasmValue(int64_t(0));
                        else if (ftype.resultType == "f32") func.result = WasmValue(float(0));
                        else if (ftype.resultType == "f64") func.result = WasmValue(double(0));
                    }
                }
            }
        }

        else if (token == "(param") {
            std::string maybeName, maybeType;
            if (iss >> maybeName) {
                if (!maybeName.empty() && maybeName[0] == '$') {
                    iss >> maybeType;        // (param $a i32)
                } else {
                    maybeType = maybeName;   // (param i32)
                    maybeName = "";
                }

                maybeType.erase(remove(maybeType.begin(), maybeType.end(), ')'), maybeType.end());

                WasmValue val;
                if (maybeType == "i32") val = WasmValue(int32_t(0));
                else if (maybeType == "i64") val = WasmValue(int64_t(0));
                else if (maybeType == "f32") val = WasmValue(float(0));
                else if (maybeType == "f64") val = WasmValue(double(0));

                if (maybeName.empty()) {
                    maybeName = "param_" + std::to_string(anonCounter++);
                }

                func.params[maybeName] = val;
            }
        }
    }

    if (func.index >= 0) functionsByID[func.index] = func;
    if (!func.name.empty()) functionByName[func.name] = func;

    std::cout << "\033[1;32m[parser:parseFunction]\033[0m Parsed function "
              << (func.name.empty() ? "[anon]" : func.name)
              << " (index " << func.index << ") "
              << "params=" << func.params.size()
              << " result=";
    switch (func.result.type) {
        case ValueType::I32: std::cout << "i32"; break;
        case ValueType::I64: std::cout << "i64"; break;
        case ValueType::F32: std::cout << "f32"; break;
        case ValueType::F64: std::cout << "f64"; break;
        default: std::cout << "(none)"; break;
    }
    std::cout << "\n";

    if (!func.params.empty()) {
        std::cout << "\033[1;32m[parser:parseFunction]\033[0m Params: ";
        for (const auto& [name, val] : func.params) {
            const char* t = (val.type == ValueType::I32) ? "i32" :
                            (val.type == ValueType::I64) ? "i64" :
                            (val.type == ValueType::F32) ? "f32" : "f64";
            std::cout << name << ":" << t << " ";
        }
        std::cout << "\n";
    }

    return func;
}

void WasmParser::print_functions(
    const std::unordered_map<std::string, FuncDef>& functionByName,
    const std::unordered_map<int, FuncDef>& functionsByID) const
{
    auto printParams = [](const std::unordered_map<std::string, WasmValue>& params) {
        if (params.empty()) {
            std::cout << "    Params: (none)\n";
            return;
        }

        std::cout << "    Params:\n";
        for (const auto& [name, val] : params) {
            std::string typeStr;
            switch (val.type) {
                case ValueType::I32: typeStr = "i32"; break;
                case ValueType::I64: typeStr = "i64"; break;
                case ValueType::F32: typeStr = "f32"; break;
                case ValueType::F64: typeStr = "f64"; break;
            }

            std::cout << "      " << (name.empty() ? "[anon]" : name)
                      << ": " << typeStr << " = ";

            switch (val.type) {
                case ValueType::I32: std::cout << val.i32; break;
                case ValueType::I64: std::cout << val.i64; break;
                case ValueType::F32: std::cout << val.f32; break;
                case ValueType::F64: std::cout << val.f64; break;
            }
            std::cout << "\n";
        }
    };

    std::cout << "\033[1;32m[parser:print_functions]\033[0m Functions by Name:\n";
    if (functionByName.empty()) {
        std::cout << "  (empty)\n";
    } else {
        for (const auto& [name, func] : functionByName) {
            std::cout << "\033[1;32m[parser:parseFunction]\033[0m Parsed function "
              << (func.name.empty() ? "[anon]" : func.name)
              << " (index " << func.index << ") "
              << "params=" << func.params.size()
              << " result="; 
            switch (func.result.type) {
                case ValueType::I32: std::cout << "i32"; break;
                case ValueType::I64: std::cout << "i64"; break;
                case ValueType::F32: std::cout << "f32"; break;
                case ValueType::F64: std::cout << "f64"; break;
                default: std::cout << "(none)"; break;
            }
            std::cout << "\n";

            printParams(func.params);

            if (!func.body.empty()) {
                std::cout << "    Body:\n";
                for (const auto& line : func.body)
                    std::cout << "      " << line << "\n";
            } else {
                std::cout << "    Body: (empty)\n";
            }

            std::cout << "\n";
        }
    }

    std::cout << "\033[1;32m[parser:print_functions]\033[0m Functions by ID:\n";
    if (functionsByID.empty()) {
        std::cout << "  (empty)\n";
    } else {
        for (const auto& [id, func] : functionsByID) {
            std::cout << "  Index " << id
                      << " " << (func.name.empty() ? "[anon]" : func.name) << "\n"
                      << "    params=" << func.params.size()
                      << " result=";
            switch (func.result.type) {
                case ValueType::I32: std::cout << "i32"; break;
                case ValueType::I64: std::cout << "i64"; break;
                case ValueType::F32: std::cout << "f32"; break;
                case ValueType::F64: std::cout << "f64"; break;
                default: std::cout << "(none)"; break;
            }
            std::cout << "\n";

            printParams(func.params);

            if (!func.body.empty()) {
                std::cout << "    Body:\n";
                for (const auto& line : func.body)
                    std::cout << "      " << line << "\n";
            } else {
                std::cout << "    Body: (empty)\n";
            }

            std::cout << "\n";
        }
    }
}

void WasmParser::parseBody(const std::string& line, FuncDef* func, bool toRemove) {
    if (!func) {
        std::cout << "\033[1;31m[parser:parseBody]\033[0m Error: Function definition is null.\n";
        return;
    }
    std::string cleaned = line;
    size_t commentPos = cleaned.find(";;");
    if (commentPos != std::string::npos)
        cleaned = cleaned.substr(0, commentPos);
    while (!cleaned.empty() && std::isspace(static_cast<unsigned char>(cleaned.back())))
        cleaned.pop_back();
    if (toRemove) {
        if (!cleaned.empty() && cleaned.back() == ')') {
            cleaned.pop_back();
            while (!cleaned.empty() && std::isspace(static_cast<unsigned char>(cleaned.back())))
                cleaned.pop_back();
        }
    }
    if (!cleaned.empty()) {
        func->body.push_back(cleaned);
        std::cout << "\033[1;32m[parser:parseBody]\033[0m Added line to function "
                  << (func->name.empty() ? "[anon]" : func->name)
                  << ": " << cleaned << "\n";
    } else {
        std::cout << "\033[1;33m[parser:parseBody]\033[0m Skipped empty/comment-only line.\n";
    }
}

void WasmParser::parseMemory(const std::string& line, WasmMemory& memory) {
    std::cout << "\033[1;32m[parser:parseMemory]\033[0m Parsing memory line: " << line << "\n";

    std::istringstream iss(line);
    std::string token;
    size_t initialPages = 1;

    while (iss >> token) {
        if (token.rfind("(;", 0) == 0)
            continue;

        if (std::isdigit(token[0])) {
            try {
                initialPages = std::stoul(token);
            } catch (...) {
                initialPages = 1;
            }
        }
    }
    memory = WasmMemory(initialPages);

    std::cout << "\033[1;32m[parser:parseMemory]\033[0m Initialized single memory with "
              << initialPages << " page(s) = "
              << (initialPages * WasmMemory::PAGE_SIZE) << " bytes.\n";
}


void WasmParser::parseExport(const std::string& line,
                             std::unordered_map<std::string, WasmExport>& exports)
{
    std::istringstream iss(line);
    std::string token;
    WasmExport exp;

    iss >> token;
    iss >> exp.name;

    if (!exp.name.empty() && exp.name.front() == '"')
        exp.name = exp.name.substr(1, exp.name.size() - 2);

    std::string kind, indexStr;
    iss >> token;
    if (token[0] == '(') token.erase(0, 1);
    kind = token;

    iss >> indexStr;
    try {
        exp.index = std::stoi(indexStr);
    } catch (...) {
        exp.index = -1;
    }

    exp.kind = kind;
    exports[exp.name] = exp;

    std::cout << "\033[1;32m[parser:parseExport]\033[0m Exported "
              << exp.kind << " '" << exp.name
              << "' (index " << exp.index << ")\n";
}

void WasmParser::print_exports(const std::unordered_map<std::string, WasmExport>& exports) const {
    std::cout << "\033[1;32m[parser:print_exports]\033[0m Exported items:\n";
    if (exports.empty()) {
        std::cout << "  (empty)\n";
        return;
    }

    for (const auto& [name, exp] : exports) {
        std::cout << "  " << exp.kind << " '" << name
                  << "' (index " << exp.index << ")\n";
    }
}