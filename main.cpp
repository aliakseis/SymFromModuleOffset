#include <windows.h>
#include "dia2.h"
#include <atlbase.h>
#include <iostream>
#include <string>
#include <sstream>


const CLSID CLSID_DiaSource = __uuidof(DiaSource);


static bool GetSymbolFromOffset(const wchar_t* pdbPath, DWORD64 offset, char* symbolName, size_t symbolSize)
{
    CComPtr<IDiaDataSource> pSource;

    if (FAILED(pSource.CoCreateInstance(CLSID_DiaSource))) {
        std::cerr << "Failed to create DIA Data Source." << std::endl;
        CoUninitialize();
        return false;
    }

    if (FAILED(pSource->loadDataFromPdb(pdbPath))) {
        std::cerr << "Failed to load PDB file." << std::endl;
        CoUninitialize();
        return false;
    }

    CComPtr<IDiaSession> pSession;
    if (FAILED(pSource->openSession(&pSession))) {
        std::cerr << "Failed to open DIA Session." << std::endl;
        CoUninitialize();
        return false;
    }

    CComPtr<IDiaEnumSymbolsByAddr> pEnum;
    if (FAILED(pSession->getSymbolsByAddr(&pEnum))) {
        std::cerr << "Failed to get symbols by address." << std::endl;
        CoUninitialize();
        return false;
    }

    CComPtr<IDiaSymbol> pSymbol;
    if (FAILED(pEnum->symbolByRVA(offset, &pSymbol))) {
        std::cerr << "Failed to get symbol from address." << std::endl;
        CoUninitialize();
        return false;
    }

    BSTR bstrName;
    if (FAILED(pSymbol->get_name(&bstrName))) {
        std::cerr << "Failed to get symbol name." << std::endl;
        CoUninitialize();
        return false;
    }

    WideCharToMultiByte(CP_UTF8, 0, bstrName, -1, symbolName, symbolSize, NULL, NULL);
    SysFreeString(bstrName);

    return true;
}


int wmain(int argc, wchar_t* argv[]) 
{
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

    for (int i = 2; i < argc; ++i)
    {
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

        char symbolName[MAX_PATH];

        if (GetSymbolFromOffset(pdbPath, offset, symbolName, sizeof(symbolName))) {
            std::cout << "Symbol: " << symbolName << std::endl;
        }
        else {
            std::cerr << "Failed to retrieve symbol." << std::endl;
        }
    }

    return 0;
}
