import os
import sys
import subprocess
import shutil

def run_command(command, cwd=None):
    """执行系统命令并实时打印输出"""
    print(f"Executing: {' '.join(command)}")
    result = subprocess.run(command, cwd=cwd, shell=True)
    if result.returncode != 0:
        print(f"Error: Command failed with exit code {result.returncode}")
        sys.exit(result.returncode)

def main():
    # 1. 配置参数
    build_dir = "build"
    # 默认 Release，支持通过 python build.py debug 切换
    config = "Release"
    if len(sys.argv) > 1 and sys.argv[1].lower() == "debug":
        config = "Debug"

    # 2. 清理旧的构建目录 (可选)
    # if os.path.exists(build_dir):
    #     shutil.rmtree(build_dir)

    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    # 3. 配置阶段 (Configure)
    # 使用 -A x64 强制 64 位构建，这是 LLVM 开发的标配
    configure_cmd = [
        "cmake",
        "-S", ".",
        "-B", build_dir,
        "-A", "x64",
        f"-DCMAKE_BUILD_TYPE={config}"
    ]
    
    # 如果你想通过脚本开启静态链接 LLVM，可以添加：
    # configure_cmd.append("-DUSE_STATIC_LLVM=ON")

    run_command(configure_cmd)

    # 4. 构建阶段 (Build)
    # --parallel 开启多核编译，显著提升构建速度
    build_cmd = [
        "cmake",
        "--build", build_dir,
        "--config", config,
        "--parallel", "8" 
    ]
    
    run_command(build_cmd)

    print(f"\n[Success] Build finished in {config} mode.")
    print(f"Executable: {os.path.join(build_dir, config, 'chtholly.exe')}")

if __name__ == "__main__":
    main()