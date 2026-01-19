#include "Backend/Linker.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace chtholly {

Linker::Linker() {
    loadToolPaths();
}

bool Linker::loadToolPaths() {

    std::ifstream file("ToolPath.md");

    if (!file.is_open()) {

        std::cerr << "Linker: Could not open ToolPath.md" << std::endl;

        return false;

    }



    std::string line;

    std::string section;

    while (std::getline(file, line)) {

        if (line.empty()) continue;

        if (line.find("## llc") != std::string::npos) {

            section = "llc";

            continue;

        } else if (line.find("## Windows Kits") != std::string::npos) {

            section = "kits";

            continue;

        } else if (line.find("##") == 0) {

            section = "other";

            continue;

        }



        if (section == "other") continue;



        // Clean up line (handle double backslashes and quotes)

        size_t start = line.find_first_not_of(" \t\r\n");

        if (start == std::string::npos) continue;

        std::string path = line.substr(start);

        

        // Basic heuristic to skip lines that are likely comments or instructions

        if (path.find("llvm") == 0 || (unsigned char)path[0] > 127) continue;



        // Remove trailing quotes and spaces

        while (!path.empty() && (path.back() == '"' || isspace(path.back()))) {

            path.pop_back();

        }



        // Handle double backslashes (Markdown escaping)

        std::string cleanPath;

        for (size_t i = 0; i < path.size(); ++i) {

            if (path[i] == '\\' && i + 1 < path.size() && path[i+1] == '\\') {

                cleanPath += '\\';

                i++;

            } else {

                cleanPath += path[i];

            }

        }



        if (cleanPath.empty() || cleanPath[0] == '#') continue;



        if (section == "llc") {

            if (m_linkerPath.empty()) {

                m_linkerPath = cleanPath;

                if (!m_linkerPath.empty() && m_linkerPath.back() != '\\') m_linkerPath += '\\';

                m_linkerPath += "link.exe";

            } else {

                m_libPaths.push_back(cleanPath);

            }

        }

        else if (section == "kits") {

            m_libPaths.push_back(cleanPath);

        }

    }



    return !m_linkerPath.empty();

}



bool Linker::invoke(const std::string& objFile, const std::string& exeFile) {

    if (m_linkerPath.empty()) {

        std::cerr << "Linker: link.exe path not found. Check ToolPath.md" << std::endl;

        return false;

    }



    std::stringstream cmd;

    cmd << "\"" << m_linkerPath << "\" ";

    cmd << objFile << " ";

    cmd << "/OUT:\"" << exeFile << "\" ";

    

    for (const auto& path : m_libPaths) {

        cmd << "/LIBPATH:\"" << path << "\" ";

    }



    cmd << "/SUBSYSTEM:CONSOLE ";

    cmd << "/DEFAULTLIB:libcmt.lib /DEFAULTLIB:oldnames.lib /DEFAULTLIB:legacy_stdio_definitions.lib kernel32.lib";



    // On Windows, system() with a command containing quotes needs the whole thing quoted

    std::string command = "\"" + cmd.str() + "\"";

    std::cout << "Executing linker: " << command << std::endl;



    int result = std::system(command.c_str());

    return result == 0;

}

} // namespace chtholly
