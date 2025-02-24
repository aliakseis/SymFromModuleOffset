#include <windows.h>
#include "dia2.h"
#include <atlbase.h>
#include <comutil.h>
#include <iostream>
#include <string>
#include <sstream>
#include <optional>

#pragma comment(lib, "comsuppw.lib")

const CLSID CLSID_DiaSource = __uuidof(DiaSource);

struct SymbolInfo {
    std::string symbolName;
    std::string sourceFile;
    DWORD lineNumber = 0;
};

std::optional<SymbolInfo> GetSymbolFromOffset(const wchar_t* pdbPath, DWORD64 offset) {
    CComPtr<IDiaDataSource> pSource;

    if (FAILED(pSource.CoCreateInstance(CLSID_DiaSource))) {
        std::cerr << "Failed to create DIA Data Source." << std::endl;
        return std::nullopt;
    }

    if (FAILED(pSource->loadDataFromPdb(pdbPath))) {
        std::cerr << "Failed to load PDB file." << std::endl;
        return std::nullopt;
    }

    CComPtr<IDiaSession> pSession;
    if (FAILED(pSource->openSession(&pSession))) {
        std::cerr << "Failed to open DIA Session." << std::endl;
        return std::nullopt;
    }

    CComPtr<IDiaEnumSymbolsByAddr> pEnum;
    if (FAILED(pSession->getSymbolsByAddr(&pEnum))) {
        std::cerr << "Failed to get symbols by address." << std::endl;
        return std::nullopt;
    }

    CComPtr<IDiaSymbol> pSymbol;
    if (FAILED(pEnum->symbolByRVA(offset, &pSymbol))) {
        std::cerr << "Failed to get symbol from address." << std::endl;
        return std::nullopt;
    }

    SymbolInfo symbolInfo;

    _bstr_t bstrName;
    if (FAILED(pSymbol->get_name(bstrName.GetAddress()))) {
        std::cerr << "Failed to get symbol name." << std::endl;
        return std::nullopt;
    }

    symbolInfo.symbolName = static_cast<const char*>(bstrName);

    // Get source file and line number
    CComPtr<IDiaEnumLineNumbers> pLines;
    if (SUCCEEDED(pSession->findLinesByRVA(offset, 1, &pLines))) {
        CComPtr<IDiaLineNumber> pLine;
        ULONG celt = 0;
        if (SUCCEEDED(pLines->Next(1, &pLine, &celt)) && celt == 1) {
            DWORD line;
            if (SUCCEEDED(pLine->get_lineNumber(&line))) {
                symbolInfo.lineNumber = line;

                CComPtr<IDiaSourceFile> pSourceFile;
                if (SUCCEEDED(pLine->get_sourceFile(&pSourceFile))) {
                    _bstr_t bstrSourceFile;
                    if (SUCCEEDED(pSourceFile->get_fileName(bstrSourceFile.GetAddress()))) {
                        symbolInfo.sourceFile = static_cast<const char*>(bstrSourceFile);
                    }
                }
            }
        }
    }

    return symbolInfo;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) {
        std::wcerr << L"Usage: " << argv[0] << L" <pdbPath> <offset>" << std::endl;
        return 1;
    }

    if (FAILED(CoInitialize(NULL))) {
        std::cerr << "Failed to initialize COM library." << std::endl;
        return 1;
    }
    atexit(CoUninitialize);

    const wchar_t* pdbPath = argv[1];

    for (int i = 2; i < argc; ++i) {
        DWORD64 offset;
        std::wstringstream ss;
        ss << argv[i];

        if (argv[i][0] == '0' && (argv[i][1] == 'x' || argv[i][1] == 'X')) {
            ss >> std::hex >> offset; // Hexadecimal
        }
        else {
            ss >> offset; // Decimal
        }

        if (ss.fail()) {
            std::wcerr << L"Invalid offset value." << std::endl;
            return 1;
        }

        auto symbolInfo = GetSymbolFromOffset(pdbPath, offset);

        if (symbolInfo) {
            std::cout << "Symbol: " << symbolInfo->symbolName << std::endl;
            if (!symbolInfo->sourceFile.empty()) {
                std::cout << "Source File: " << symbolInfo->sourceFile << std::endl;
            }
            if (symbolInfo->lineNumber != 0) {
                std::cout << "Line: " << symbolInfo->lineNumber << std::endl;
            }
        }
        else {
            std::cerr << "Failed to retrieve symbol." << std::endl;
        }
    }

    return 0;
}
