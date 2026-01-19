#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "Parser.h"
#include "Sema/Sema.h"
#include "MIR/MIRBuilder.h"
#include "Backend/CodeGenerator.h"
#include "Backend/Linker.h"

using namespace chtholly;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <source_file> [-o <out_file>] [-run]" << std::endl;
        return 1;
    }

    std::string sourcePath = argv[1];
    std::string outPath;
    bool shouldRun = false;

    for (int i = 2; i < argc; ++i)
    {
        if (std::string(argv[i]) == "-o" && i + 1 < argc)
        {
            outPath = argv[++i];
        }
        else if (std::string(argv[i]) == "-run")
        {
            shouldRun = true;
        }
    }

    if (outPath.empty()) {
        std::filesystem::path p(sourcePath);
        outPath = p.stem().string() + ".exe";
    }

    std::string objPath = outPath + ".obj";
    if (outPath.ends_with(".obj")) {
        objPath = outPath;
    }

    std::ifstream file(sourcePath);
    if (!file.is_open())
    {
        std::cerr << "Could not open file: " << sourcePath << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    try
    {
        Parser parser(source);
        auto program = parser.parseProgram();

        Sema sema;
        for (auto const &node : program)
        {
            sema.analyze(node.get());
        }
        std::cout << "Semantic analysis passed!" << std::endl;

        MIRModule module;
        MIRBuilder mirBuilder(module);

        for (auto const &[name, table] : sema.getModules())
        {
            mirBuilder.addModuleName(name);
        }

        // Lower nodes from specialized generics first (analyzedNodes)
        for (auto const &node : sema.getAnalyzedNodes())
        {
            mirBuilder.lower(node.get());
        }

        for (auto const &node : program)
        {
            mirBuilder.lower(node.get());
        }
        std::cout << "MIR lowering successful!" << std::endl;

        CodeGenerator codegen(module);
        codegen.generate();
        std::cout << "LLVM IR generation successful!" << std::endl;

        codegen.emitObjectFile(objPath);
        if (std::filesystem::exists(objPath))
        {
            std::cout << "Successfully emitted " << objPath << std::endl;
        }

        // 4. Automated Linking
        if (outPath.ends_with(".exe") || !outPath.ends_with(".obj")) {
            std::string exePath = outPath;
            if (!exePath.ends_with(".exe")) exePath += ".exe";

            Linker linker;
            if (linker.invoke(objPath, exePath)) {
                std::cout << "Successfully linked " << exePath << std::endl;
                
                if (shouldRun) {
                    std::cout << "Running " << exePath << "..." << std::endl;
                    std::string runCmd = ".\\" + exePath;
                    std::system(runCmd.c_str());
                }
            } else {
                std::cerr << "Linking failed." << std::endl;
                return 1;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}