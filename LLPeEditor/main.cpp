#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <windows.h>

#include <algorithm>
#include <fstream>
#include <unordered_set>

#include "pdb.h"

#include "../PortableExecutable/pe_bliss.h"
using namespace pe_bliss;
inline void Pause(bool Pause) {
    if (Pause)
        system("pause");
}

int main(int argc, char** argv) {
    if (argc == 0)
        system("mode con cols=49 lines=16");

    SetConsoleCtrlHandler([](DWORD signal) -> BOOL {

        return TRUE;
        },
        TRUE);
    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE,
        MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    std::set<std::string> CliArgs;
    for (int i = 1; i < argc; i++)
        CliArgs.insert(argv[i]);
    if (CliArgs.count("-h")) {
        std::cout << "BDSLiteLoader PE Editor For BDS" << std::endl
            << "Usage: " << argv[0] << " [flags,...]" << std::endl
            << "Flags: -noMod     Do not generate bedrock_server_mod.exe" << std::endl
            << "       -noSymDB   Do not generate bedrock_server.symdb2" << std::endl
            << "       -def       Generate def file for develop propose" << std::endl
            << "       -sym       Generate bedrock_server_SymList.txt for develop propose" << std::endl
            << "       -keepOri   Keep Original bedrock_server.exe" << std::endl
            << "       -noPause   Do not pause before exit" << std::endl;
        return 0;
    }
    bool mGenModBDS = CliArgs.count("-noMod")
        ? false
        : true;
    bool mGenSymDB = CliArgs.count("-symDB")
        ? true
        : false;
    bool mGenDevDef = CliArgs.count("-def")
        ? true
        : false;
    bool mGenSymbolList = CliArgs.count("-sym")
        ? true
        : false;
    bool mKeepOriginal = CliArgs.count("-keepOri")
        ? true
        : false;
    bool mPause = CliArgs.count("-noPause")
        ? false
        : true;
    std::cout << "[Info] LiteLoader ToolChain PEEditor" << std::endl;
    std::cout << "[Info] BuildDate CST " __TIMESTAMP__ << std::endl;
    std::cout << "[Info] Gen bedrock_server_mod.exe              " << std::boolalpha << std::string(mGenModBDS ? " true" : "false") << std::endl;
    std::cout << "[Info] Gen bedrock_server_mod.def        [DEV] " << std::boolalpha << std::string(mGenDevDef ? " true" : "false") << std::endl;
    std::cout << "[Info] Gen bedrock_server_SymList.txt    [DEV] " << std::boolalpha << std::string(mGenSymbolList ? " true" : "false") << std::endl;
    std::cout << "[Info] Keep bedrock_server.exe           [DEV] " << std::boolalpha << std::string(mKeepOriginal ? " true" : "false") << std::endl;

    std::cout << "[Info] Loading bedrock_server.pdb, Please wait" << std::endl;
    auto FunctionList = loadPDB(L"bedrock_server.pdb");
    std::cout << "[Info] Loaded " << FunctionList->size() << " Symbols" << std::endl;
    std::ifstream             OriginalBDS;
    std::ofstream             ModifiedBDS;
    pe_base* OriginalBDS_PE = nullptr;
    export_info                OriginalBDS_ExportInf;
    exported_functions_list         OriginalBDS_ExportFunc;
    uint16_t                  ExportLimit = 0;
    int                       ExportCount = 1;
    int                       ApiDefCount = 1;
    int                       ApiDefFileCount = 1;
    const std::vector<std::string> SkipPerfix = {
        "_",
        "?__",
        "??_",
        "??@",
        "?$TSS"
        "??_C",
        "??3",
        "??2",
        "??_R4",
        "??_E",
        "??_G" };

    const std::vector<std::regex> SkipRegex = {
        std::regex(R"(\?+[a-zA-Z0-9_-]*([a-zA-Z0-9_-]*@)*std@@.*)", std::regex::icase),
        std::regex(R"(.*printf$)", std::regex::icase),
        std::regex(R"(.*no_alloc.*)", std::regex::icase) };
    std::ofstream BDSDef_API;
    std::ofstream BDSDef_VAR;
    std::ofstream BDSSymList;
    if (mGenModBDS) {
        OriginalBDS.open("bedrock_server.exe", std::ios::in | std::ios::binary);

        if (OriginalBDS) {
            ModifiedBDS.open("bedrock_server_mod.exe", std::ios::out | std::ios::binary | std::ios::trunc);
            if (!ModifiedBDS) {
                std::cout << "[Err] Cannot create bedrock_server_mod.exe" << std::endl;
                Pause(mPause);
                return -1;
            }
            if (!OriginalBDS) {
                std::cout << "[Err] Cannot open bedrock_server.exe" << std::endl;
                Pause(mPause);
                return -1;
            }
            OriginalBDS_PE = new pe_base(pe_factory::create_pe(OriginalBDS));
            try {
                OriginalBDS_ExportFunc = get_exported_functions(*OriginalBDS_PE, OriginalBDS_ExportInf);

            }
            catch (const pe_exception& e) {
                std::cout << "[Err] Get Exported Failed: " << e.what() << std::endl;
                Pause(mPause);
                return -1;
            }
            ExportLimit = get_export_ordinal_limits(OriginalBDS_ExportFunc).second;
        }
        else {
            std::cout << "[Err] Failed to Open bedrock_server.exe" << std::endl;
            Pause(mPause);
            return -1;
        }
    }
    if (mGenDevDef) {
        BDSDef_API.open("bedrock_server_api.def", std::ios::ate | std::ios::out);
        if (!BDSDef_API) {
            std::cout << "[Err] Cannot create bedrock_server_api_0.def" << std::endl;
            Pause(mPause);
            return -1;
        }
        BDSDef_API << "LIBRARY bedrock_server.dll\nEXPORTS\n";

        BDSDef_VAR.open("bedrock_server_var.def", std::ios::ate | std::ios::out);
        if (!BDSDef_VAR) {
            std::cout << "[Err] Cannot create bedrock_server_var.def" << std::endl;
            Pause(mPause);
            return -1;
        }
        BDSDef_VAR << "LIBRARY bedrock_server_mod.exe\nEXPORTS\n";
    }
    if (mGenSymbolList) {
        BDSSymList.open("bedrock_server_SymList.txt", std::ios::ate | std::ios::out);
        if (!BDSSymList) {
            std::cout << "[Err] Cannot create bedrock_server_SymList.txt" << std::endl;
            Pause(mPause);
            return -1;
        }
    }
    for (const auto& fn : *FunctionList) {
        
        try {

            if (mGenSymbolList) {
                char tmp[16384];
                sprintf_s(tmp, 16384, "[%08d] %s", fn.Rva, fn.Name.c_str());
                BDSSymList << tmp << std::endl;
            }
            bool skip = false;
            if (fn.Name[0] != '?') {
                skip = true;
                goto Skipped;
            }
            for (const auto& a : SkipPerfix) {
                if (fn.Name.starts_with(a)) {
                    skip = true;
                    goto Skipped;
                }
            }
            for (const auto& reg : SkipRegex) {
                std::smatch result;
                if (std::regex_match(fn.Name, result, reg)) {
                    skip = true;
                    goto Skipped;
                }
            }
        Skipped:
            
            if (mGenDevDef && !skip)
                if (fn.IsFunction) {
                    BDSDef_API << "\t" << fn.Name << "\n";
                }
                else
                    BDSDef_VAR << "\t" << fn.Name << "\n";
            if (mGenModBDS && !fn.IsFunction && !skip) {
                exported_function func;
                func.set_name(fn.Name);
                func.set_rva(fn.Rva);
                func.set_ordinal(ExportLimit + ExportCount);
                ExportCount++;
                if (ExportCount > 65535) {
                    std::cout << "[Err] Too many Symbols are going to insert to ExportTable" << std::endl;
                    Pause(mPause);
                    return 1;
                }
                OriginalBDS_ExportFunc.push_back(func);
            }
        }
        catch (const pe_exception& e) {
            std::cout << "PeEditor : " << e.what() << std::endl;
            Pause(mPause);
            return -1;
        }
        catch (const std::regex_error& e) {
            std::cout << "RegexErr : " << e.what() << std::endl;
            Pause(mPause);
            return -1;
        }
        catch (...) {
            std::cout << "UnkErr " << std::endl;
            Pause(mPause);
            return -1;
        }
    }
    //cout << ExportCount << endl;
    if (mGenModBDS) {
        try {
            section ExportSection;
            ExportSection.get_raw_data().resize(1);
            ExportSection.set_name("ExpFunc");
            ExportSection.readable(true);
            section& attachedExportedSection = OriginalBDS_PE->add_section(ExportSection);
            rebuild_exports(*OriginalBDS_PE, OriginalBDS_ExportInf, OriginalBDS_ExportFunc, attachedExportedSection);


            imported_functions_list imports(get_imported_functions(*OriginalBDS_PE));

            import_library preLoader;
            preLoader.set_name("LLPreLoader.dll");

            imported_function func;
            func.set_name("dlsym_real");
            func.set_iat_va(0x1);

            preLoader.add_import(func);
            imports.push_back(preLoader);

            section ImportSection;
            ImportSection.get_raw_data().resize(1);
            ImportSection.set_name("ImpFunc");
            ImportSection.readable(true).writeable(true);
            section& attachedImportedSection = OriginalBDS_PE->add_section(ImportSection);
            rebuild_imports(*OriginalBDS_PE, imports, attachedImportedSection, import_rebuilder_settings(true,false));



            rebuild_pe(*OriginalBDS_PE, ModifiedBDS);
            ModifiedBDS.close();
            std::cout << "[Info] bedrock_server_mod.exe             Created" << std::endl;

            try {
                if (!mKeepOriginal) {
                    OriginalBDS.close();
                    if (std::filesystem::remove(std::filesystem::path("bedrock_server.exe")))
                        std::cout << "[Info] bedrock_server.exe                 Removed" << std::endl;
                }
            }
            catch (...) {
            }
        }
        catch (pe_exception e) {
            std::cout << "[Error] Failed to rebuild bedrock_server_mod.exe" << std::endl;
            std::cout << "[Error] " << e.what() << std::endl;
            ModifiedBDS.close();
            std::filesystem::remove(std::filesystem::path("bedrock_server_mod.exe"));
        }
        catch (...) {
            std::cout << "[Error] Failed to rebuild bedrock_server_mod.exe with unk err" << std::endl;
            ModifiedBDS.close();
            std::filesystem::remove(std::filesystem::path("bedrock_server_mod.exe"));
        }
    }
    if (mGenDevDef) {
        try {
            BDSDef_API.flush();
            BDSDef_API.close();
            BDSDef_VAR.flush();
            BDSDef_VAR.close();
            std::cout << "[Info] [DEV]bedrock_server_var/api.def    Created" << std::endl;
        }
        catch (...) {
            std::cout << "[Error] Failed to Cerate bedrock_server_var/api.def" << std::endl;
        }
    }
    if (mGenSymbolList) {
        try {
            BDSSymList.flush();
            BDSSymList.close();
            std::cout << "[Info] [DEV]bedrock_server_SymList.txt    Created" << std::endl;
        }
        catch (...) {
            std::cout << "[Error] Failed to Cerate bedrock_server_SymList.txt" << std::endl;
        }
    }

    Pause(mPause);
    return 0;
}