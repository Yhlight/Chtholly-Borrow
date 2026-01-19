#ifndef CHTHOLLY_LINKER_H
#define CHTHOLLY_LINKER_H

#include <string>
#include <vector>
#include <filesystem>

namespace chtholly {

class Linker {
public:
    Linker();
    bool invoke(const std::string& objFile, const std::string& exeFile);

private:
    bool loadToolPaths();
    std::string findFile(const std::string& filename);

    std::string m_linkerPath;
    std::vector<std::string> m_libPaths;
};

} // namespace chtholly

#endif // CHTHOLLY_LINKER_H
