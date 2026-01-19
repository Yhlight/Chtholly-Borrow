import os
import sys
import subprocess
import shutil
import platform

def run_command(command, cwd=None):
    """Executes system command and prints output"""
    print(f"Executing: {' '.join(command)}")
    # On Windows shell=True is often needed for cmake to be found if not in direct path or for some shell features.
    # On Linux it's better to avoid shell=True if passing a list.
    use_shell = (sys.platform == 'win32')
    result = subprocess.run(command, cwd=cwd, shell=use_shell)
    if result.returncode != 0:
        print(f"Error: Command failed with exit code {result.returncode}")
        sys.exit(result.returncode)

def main():
    # 1. Configuration
    build_dir = "build"
    # Default Release
    config = "Release"
    if len(sys.argv) > 1 and sys.argv[1].lower() == "debug":
        config = "Debug"

    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    # 3. Configure
    configure_cmd = [
        "cmake",
        "-S", ".",
        "-B", build_dir,
        f"-DCMAKE_BUILD_TYPE={config}"
    ]

    # Only add -A x64 on Windows (Visual Studio generator)
    if sys.platform == "win32":
        configure_cmd.extend(["-A", "x64"])

    run_command(configure_cmd)

    # 4. Build
    build_cmd = [
        "cmake",
        "--build", build_dir,
        "--config", config,
        "--parallel", "8" 
    ]
    
    run_command(build_cmd)

    print(f"\n[Success] Build finished in {config} mode.")
    exe_name = "chtholly.exe" if sys.platform == "win32" else "chtholly"

    # Check possible locations
    exe_path = os.path.join(build_dir, config, exe_name)
    if not os.path.exists(exe_path):
        exe_path = os.path.join(build_dir, exe_name)

    print(f"Executable: {exe_path}")

if __name__ == "__main__":
    main()
