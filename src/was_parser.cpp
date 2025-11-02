#include "wasm_parser.hpp"
#include "wasm_stack.hpp"
#include "wasm_memory.hpp"
#include <iostream>
#include <sstream>
#include <algorithm> 
#include <regex>

void WasmParser::parseModule(WasmStack& stack, std::unordered_map<std::string, int32_t>& globals) {
    stack.clear();
    globals.clear();

    std::cout << "\033[1;32m[parser:parseModule]\033[0m New module initialized. Stack and globals cleared.\n";
}

void WasmParser::parseGlobal(const std::string& line, std::unordered_map<std::string, int32_t>& globals) {
    std::cout << "\033[1;32m[parser:parseGlobal]\033[0m Global definition found: " << line << "\n";

    std::istringstream iss(line);
    std::string temp, name, token;
    int32_t value = 0;

    iss >> temp;
    iss >> name;

    while (iss >> token) {
        if (token.find("i32.const") != std::string::npos) {
            iss >> value;
            break;
        }
    }

    globals[name] = value;
    std::cout << "\033[1;32m[parser:parseGlobal]\033[0m Global init → " << name << " = " << value << "\n";
}

void WasmParser::print_globals(const std::unordered_map<std::string, int32_t>& globals) const {
    std::cout << "\033[1;32m[parser:print_globals]\033[0m Global variables state:\n";
    if (globals.empty()) {
        std::cout << "  (empty)\n";
        return;
    }

    for (const auto& [name, value] : globals) {
        std::cout << "  " << name << " = " << value << "\n";
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

    while (iss >> token) {
        if (token == "(func") continue;

        if (token.rfind("(;", 0) == 0 && token.find(";") != std::string::npos) {
            try {
                size_t start = token.find(';') + 1;
                size_t end = token.find(';', start);
                func.index = std::stoi(token.substr(start, end - start));
            } catch (...) {
                func.index = -1;
            }
        }

        else if (token[0] == '$') {
            func.name = token;
        }

        else if (token == "(type") {
            std::string typeIndex;
            if (iss >> typeIndex) {
                typeIndex.erase(remove(typeIndex.begin(), typeIndex.end(), ')'), typeIndex.end());
                int t = std::stoi(typeIndex);

                auto it = funcTypes.find(t);
                if (it != funcTypes.end()) {

                    func.paramNames.clear();
                    for (const auto& paramType : it->second.params)
                        func.paramNames.push_back({ "", paramType });
                    func.result = { "", it->second.resultType };
                }
            }
        }

        else if (token == "(param") {
            std::string maybeName, maybeType;
            if (iss >> maybeName) {
                if (maybeName[0] == '$') {
                    iss >> maybeType;
                    maybeType.erase(remove(maybeType.begin(), maybeType.end(), ')'), maybeType.end());

                    for (auto& p : func.paramNames) {
                        if (p.name.empty() && p.type == maybeType) {
                            p.name = maybeName;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (func.index >= 0)
        functionsByID[func.index] = func;
    if (!func.name.empty())
        functionByName[func.name] = func;

    std::cout << "\033[1;32m[parser:parseFunction]\033[0m Parsed function "
              << (func.name.empty() ? "[anon]" : func.name)
              << " (index " << func.index << ") "
              << "params=" << func.paramNames.size()
              << " result=" << func.result.type << "\n";

    std::cout << "\033[1;32m[parser:parseFunction]\033[0m Params: ";
    for (const auto& p : func.paramNames)
        std::cout << (p.name.empty() ? "_" : p.name) << ":" << p.type << " ";
    std::cout << "| result: " << (func.result.name.empty() ? "_" : func.result.name)
              << ":" << func.result.type << "\n";
    return func;
}

void WasmParser::print_functions(
    const std::unordered_map<std::string, FuncDef>& functionByName,
    const std::unordered_map<int, FuncDef>& functionsByID) const
{
    std::cout << "\033[1;32m[parser:print_functions]\033[0m Functions by Name:\n";
    if (functionByName.empty()) {
        std::cout << "  (empty)\n";
    } else {
        for (const auto& [name, func] : functionByName) {
            std::cout << "  " << name
                      << " (index " << func.index << ")\n"
                      << "    params=" << func.paramNames.size()
                      << " result=" << func.result.type << "\n";

            if (!func.paramNames.empty()) {
                std::cout << "    Params:\n";
                for (const auto& p : func.paramNames)
                    std::cout << "      " << (p.name.empty() ? "[anon]" : p.name)
                              << ": " << p.type << "\n";
            }

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
                      << "    params=" << func.paramNames.size()
                      << " result=" << func.result.type << "\n";

            if (!func.paramNames.empty()) {
                std::cout << "    Params:\n";
                for (const auto& p : func.paramNames)
                    std::cout << "      " << (p.name.empty() ? "[anon]" : p.name)
                              << ": " << p.type << "\n";
            }

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
    while (!cleaned.empty() && std::isspace(cleaned.back()))
        cleaned.pop_back();
    if (toRemove) {
        if (!cleaned.empty() && cleaned.back() == ')') {
            cleaned.pop_back();
            while (!cleaned.empty() && std::isspace(cleaned.back()))
                cleaned.pop_back();
        }
    }

    if (!cleaned.empty()) {
        func->body.push_back(cleaned);
        std::cout << "\033[1;32m[parser:parseBody]\033[0m Added line to function "
                  << (func->name.empty() ? "[anon]" : func->name)
                  << ": " << cleaned << "\n";
    } else {
        std::cout << "\033[1;33m[parser:parseBody]\033[0m Skipped empty line after cleaning.\n";
    }
}

void WasmParser::parseMemory(const std::string& line, std::unordered_map<int, WasmMemory>& memories) {
    std::cout << "\033[1;32m[parser:parseMemory]\033[0m Parsing memory line: " << line << "\n";

    std::istringstream iss(line);
    std::string token;
    int memoryIndex = 0;
    size_t initialPages = 1;

    while (iss >> token) {
        if (token.rfind("(;", 0) == 0 && token.find(";") != std::string::npos) {
            try {
                size_t start = token.find(';') + 1;
                size_t end = token.find(';', start);
                memoryIndex = std::stoi(token.substr(start, end - start));
            } catch (...) {
                memoryIndex = 0;
            }
        }
        else if (std::isdigit(token[0])) {
            try {
                initialPages = std::stoul(token);
            } catch (...) {
                initialPages = 1;
            }
        }
    }

    WasmMemory mem(initialPages);
    mem.setIndex(memoryIndex);

    memories[memoryIndex] = mem;

    std::cout << "\033[1;32m[parser:parseMemory]\033[0m Created memory index "
              << memoryIndex << " with " << initialPages << " page(s) = "
              << (initialPages * WasmMemory::PAGE_SIZE) << " bytes.\n";
}

