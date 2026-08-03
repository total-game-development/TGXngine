#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import platform

PROJECT_NAME = "TGXngine"
IS_POSIX = os.name == "posix"
VCPKG_ROOT = os.environ.get("VCPKG_ROOT", "C:/vcpkg")

if not VCPKG_ROOT:
    vcpkg_exe_path = shutil.which("vcpkg")
    if vcpkg_exe_path:
        # Extract the directory containing the executable and normalize backslashes
        VCPKG_ROOT = os.path.dirname(vcpkg_exe_path).replace("\\", "/")
    else:
        # 3. Final safety fallback if it can't be discovered anywhere
        VCPKG_ROOT = "C:/vcpkg"


def build(project_dir: str, release: bool) -> None:
    target = "Release" if release else "Debug"

    print(f"BUILDING TARGET: {target}")

    subprocess.call(
        [
            "cmake",
            "--build",
            ".",
            "--config",
            target,
        ],
        cwd=project_dir,
    )


def generate(
    generator: str,
    project_dir: str,
    release: bool,
    optim: bool,
    docs: bool,
    sanitizers: bool,
) -> None:
    print(
        "GENERATING PROJECT WITH GENERATOR: ",
        "default" if generator == "" else generator,
    )

    target = "Release" if release else "Debug"
    documentation = "ON" if docs else "OFF"
    optimise = "ON" if optim else "OFF"
    use_sanitizers = "ON" if sanitizers else "OFF"

    # reset build dir
    if os.path.exists(project_dir):
        shutil.rmtree(project_dir)
    os.makedirs(project_dir, exist_ok=True)

    # regenerate
    subprocess.call(
        [
            "cmake",
            *([] if generator == "" else ["-G", generator]),
            f"-DGENERATE_DOCUMENTATION={documentation}",
            f"-DOPTIMISE={optimise}",
            f"-DUSE_SANITIZERS={use_sanitizers}",
            f"-DCMAKE_BUILD_TYPE={target}",
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
            *(
                []
                if IS_POSIX
                else [
                    f"-DCMAKE_TOOLCHAIN_FILE={VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
                ]
            ),
            "../..",
        ],
        cwd=project_dir,
    )


def run(project_dir: str, release: bool) -> None:
    print("RUNNING PROJECT")

    target = "Release" if release else "Debug"

    binary_name = PROJECT_NAME
    if not IS_POSIX:
        binary_name += ".exe"

    cmd_prefix = "./" if IS_POSIX else ".\\"
    command = cmd_prefix + binary_name

    # try build/generator/
    binary_path = os.path.join(project_dir, binary_name)
    if os.path.exists(binary_path):
        subprocess.call([command if IS_POSIX else binary_path], cwd=project_dir)
        return

    # try build/generator/Target/
    binary_path = os.path.join(project_dir, target, binary_name)
    if os.path.exists(binary_path):
        subprocess.call(
            [command if IS_POSIX else binary_path], cwd=os.path.dirname(binary_path)
        )
        return

    print(
        f"ERROR: {binary_name} not found in {project_dir} or {os.path.join(project_dir, target)}"
    )


def get_windows_debugger() -> str:
    """
    Locates the Visual Studio IDE executable (devenv.exe) using PATH or vswhere.
    Supports VS 2022 and VS 2026 (64-bit) as well as older 32-bit versions.
    """
    # Check if already in the PATH
    path_check = shutil.which("devenv")
    if path_check:
        return path_check

    # possible locations for vswhere.exe
    potential_vswhere_dirs = [
        os.environ.get("ProgramFiles(x86)", "C:\\Program Files (x86)"),
        os.environ.get("ProgramFiles", "C:\\Program Files"),
    ]

    vswhere_exe = None
    for base_dir in potential_vswhere_dirs:
        test_path = os.path.join(
            base_dir, "Microsoft Visual Studio", "Installer", "vswhere.exe"
        )
        if os.path.exists(test_path):
            vswhere_exe = test_path
            break

    # find the latest install
    if vswhere_exe:
        try:
            result = subprocess.run(
                [vswhere_exe, "-latest", "-property", "productPath"],
                capture_output=True,
                text=True,
                check=True,
            )
            devenv_path = result.stdout.strip()
            if devenv_path and os.path.exists(devenv_path):
                return devenv_path
        except (subprocess.CalledProcessError, OSError):
            pass

    return ""


def examine(project_dir: str, release: bool) -> None:
    """
    Launches the platform-specific debugger (GDB/LLDB/Visual Studio) for the project.
    """
    system = platform.system()
    if system not in ["Linux", "Darwin", "Windows"]:
        print(
            f"ERROR: Examining crashes is only configured for Linux/macOS/Windows (Current: {system})"
        )
        return

    target = "Release" if release else "Debug"

    binary_name = PROJECT_NAME
    if system == "Windows":
        binary_name += ".exe"

    # Locate the binary (check root build dir, then target-specific subfolder)
    binary_path = os.path.join(project_dir, binary_name)
    if not os.path.exists(binary_path):
        binary_path = os.path.join(project_dir, target, binary_name)

    if not os.path.exists(binary_path):
        print(f"ERROR: {binary_name} not found at {binary_path}")
        return

    working_dir = os.path.abspath(os.path.dirname(binary_path))
    env = os.environ.copy()
    module_path = os.path.join(working_dir, "modules")

    # Platform-specific configuration
    if system == "Darwin":
        debugger_name = "LLDB"
        lib_key = "DYLD_LIBRARY_PATH"
        path_sep = ":"
        debugger_cmd = ["lldb", "-o", "run", "--", f"./{binary_name}"]
    elif system == "Linux":
        debugger_name = "GDB"
        lib_key = "LD_LIBRARY_PATH"
        path_sep = ":"
        # fmt: off
        debugger_cmd = [
            "gdb", "-q",
            "-iex", "set debuginfod enabled on",
            "-ex", "set pagination off",
            "-ex", "run", "--args",
            f"./{binary_name}",
        ]
        # fmt: on
    elif system == "Windows":
        devenv_exe = get_windows_debugger()
        if not devenv_exe:
            print("ERROR: 'devenv' not found. Ensure Visual Studio is installed.")
            return

        debugger_name = "Visual Studio"
        lib_key = "PATH"
        path_sep = ";"

        # automatically run once loaded
        debugger_cmd = [devenv_exe, "/debugexe", binary_name, "/command", "Debug.Start"]

    print(f"EXAMINING PROJECT IN {debugger_name}")

    # Inject library paths (so debugger finds modules/DLLs)
    current_lib_path = env.get(lib_key, "")
    new_path = f"{working_dir}{path_sep}{module_path}"
    env[lib_key] = (
        f"{new_path}{path_sep}{current_lib_path}" if current_lib_path else new_path
    )

    try:
        print(f"Starting {debugger_name} in: {working_dir}")
        subprocess.call(debugger_cmd, cwd=working_dir, env=env)
    except KeyboardInterrupt:
        print(f"\nExiting {debugger_name}...")
    except FileNotFoundError as _:
        print(f"ERROR: Failed to launch debugger command: {debugger_cmd[0]}")


def test_project(project_dir: str, generator: str, release: bool) -> None:
    target = "Release" if release else "Debug"

    # regenerate with BUILD_UNIT_TESTS=ON
    subprocess.check_call(
        [
            "cmake",
            *([] if generator == "" else ["-G", generator]),
            "-DBUILD_UNIT_TESTS=ON",
            f"-DCMAKE_BUILD_TYPE={target}",
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
            *(
                []
                if IS_POSIX
                else [
                    f"-DCMAKE_TOOLCHAIN_FILE={VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
                ]
            ),
            "../..",
        ],
        cwd=project_dir,
    )

    # build
    subprocess.check_call(
        [
            "cmake",
            "--build",
            ".",
            "--config",
            target,
        ],
        cwd=project_dir,
    )

    # run UnitTests
    test_binary = "UnitTests"
    if os.name != "posix":
        test_binary += ".exe"

    # try build/project_dir/
    binary_path = os.path.join(project_dir, test_binary)
    if os.path.exists(binary_path):
        subprocess.check_call(
            [binary_path if not IS_POSIX else "./" + test_binary], cwd=project_dir
        )
        return

    # try build/project_dir/Target/
    binary_path = os.path.join(project_dir, target, test_binary)
    if os.path.exists(binary_path):
        subprocess.check_call(
            [binary_path if not IS_POSIX else "./" + test_binary],
            cwd=os.path.dirname(binary_path),
        )
        return

    print(f"ERROR: {test_binary} not found in {project_dir}")


def publish(project_dir: str) -> None:
    print("PUBLISHING PROJECT")

    project_name = PROJECT_NAME
    extensions = [".dll", ".dylib", ".so", ".exe", ".lib", ".pdb"]
    binaries = [project_name]

    # remove last result
    if os.path.exists(project_name):
        shutil.rmtree(project_name)

    # copy binary, modules, and libraries
    files = os.listdir(project_dir)
    files += [f"modules/{file}" for file in os.listdir(f"{project_dir}/modules")]
    for path in files:
        directory, filename = os.path.split(path)
        _, extension = os.path.splitext(filename)

        if extension.lower() in extensions or filename in binaries:
            os.makedirs(os.path.join(project_name, directory), exist_ok=True)
            source = os.path.join(project_dir, path)
            destination = os.path.join(project_name, path)
            shutil.copy2(source, destination)

    # copy Resources
    shutil.copytree("Resources", f"{project_name}/Resources")


def copy_windows_dependencies(project_dir: str, release: bool) -> None:
    """
    Ensures vendor fallback binaries (OpenAL and Zlib) are in the executable
    directory on Windows, supporting both single-config and multi-config layouts.
    """
    if os.name == "posix":
        return

    print("Checking Windows runtime dependencies...")
    target = "Release" if release else "Debug"

    # find binaries
    possible_bin_dirs = [
        project_dir,
        os.path.join(project_dir, target),
    ]

    deps_map = {
        "Vendor/SFML/extlibs/bin/x64/openal32.dll": "OpenAL32.dll",
        "Vendor/zlib/build/Release/z.dll": "z.dll",
    }

    for bin_dir in possible_bin_dirs:
        engine_exe = os.path.join(bin_dir, f"{PROJECT_NAME}.exe")
        unit_test_exe = os.path.join(bin_dir, "UnitTests.exe")

        if os.path.exists(engine_exe) or os.path.exists(unit_test_exe):
            for src_rel, dest_name in deps_map.items():
                src_path = os.path.abspath(src_rel)
                dest_path = os.path.join(bin_dir, dest_name)

                if os.path.exists(src_path):
                    # Check if it needs copying (missing or outdated)
                    if not os.path.exists(dest_path) or os.path.getmtime(
                        src_path
                    ) > os.path.getmtime(dest_path):
                        print(f"-> Copying {dest_name} to {bin_dir}")
                        shutil.copy2(src_path, dest_path)
                else:
                    print(f"Dependency source missing: {src_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description="")

    parser.add_argument(
        "generator", default="", type=str, nargs="?", help="CMake Generator (cmake -h)"
    )

    parser.add_argument(
        "-g", "--generate", action="store_true", help="Regenerate project"
    )
    parser.add_argument("-b", "--build", action="store_true", help="Build target")
    parser.add_argument(
        "-p",
        "--publish",
        action="store_true",
        help=f"Copy build files to /{PROJECT_NAME}",
    )
    parser.add_argument("-r", "--run", action="store_true", help="Run result binary")
    parser.add_argument(
        "-d", "--docs", action="store_true", help="Build documentation with Doxygen"
    )

    parser.add_argument(
        "-R", "--release", action="store_true", help="Build target in Release mode"
    )
    parser.add_argument(
        "-o", "--optimise", action="store_true", help="Build with -O3 optimisation"
    )
    parser.add_argument(
        "-t", "--test", action="store_true", help="Build and run unit tests"
    )
    parser.add_argument(
        "-e",
        "--examine",
        action="store_true",
        help="Examine a crash using GDB/LLDB (Linux/macOS)",
    )
    parser.add_argument(
        "-S",
        "--sanitizers",
        action="store_true",
        help="Generate project with ASan/UBSan (Linux/macOS)",
    )

    args = parser.parse_args()

    project_dir = "build/" + (
        "default" if args.generator == "" else args.generator
    ).replace(" ", "_")

    os.makedirs(project_dir, exist_ok=True)

    if args.sanitizers:
        print("SANITIZER MODE ENABLED")

        args.generate = True
        args.build = True
        args.run = True

        # force safe debug configuration
        args.release = False
        args.optimise = False

    if args.test:
        test_project(project_dir, args.generator, args.release)
        # Force copy right after test compilation finishes
        copy_windows_dependencies(project_dir, args.release)
        return

    if args.generate:
        generate(
            args.generator,
            project_dir,
            args.release,
            args.optimise,
            args.docs,
            args.sanitizers,
        )

    if args.build:
        build(project_dir, args.release)
        # Force copy right after primary engine compilation finishes
        copy_windows_dependencies(project_dir, args.release)

    if args.publish:
        publish(project_dir)

    if args.examine:
        # Fail-safe check: verify assets are in place prior to launching debugger attach
        copy_windows_dependencies(project_dir, args.release)
        examine(project_dir, args.release)
    elif args.run:
        # Fail-safe check: verify assets are in place prior to raw execution
        copy_windows_dependencies(project_dir, args.release)
        run(project_dir, args.release)


if __name__ == "__main__":
    main()
