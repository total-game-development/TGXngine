import os
import re
from pathlib import Path

# ANSI Escape Codes for colored terminal output
RED = "\033[91m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
RESET = "\033[0m"

# Match independent words or class-qualified identifiers followed by '('
# Group 1: Optional class scope (e.g., 'Mouse::', 'sf::')
# Group 2: The actual function name (e.g., 'drawCursor')
# Group 3: The parameters
FUNCTION_REGEX = re.compile(
    r"\b(?:([a-zA-Z_][a-zA-Z0-9_]*::))?([a-zA-Z_][a-zA-Z0-9_]*)\b\s*\(([^)]*)\)"
)

# Regex to catch lambda or variable assignments: identifier = ...
ASSIGNMENT_REGEX = re.compile(r"\b([a-zA-Z_][a-zA-Z0-9_]*)\s*=")

# Valid C++ Type identifier shape (PascalCase, all caps macros, or ending in _t)
TYPE_IDENTIFIER_REGEX = re.compile(r"^[A-Z][a-zA-Z0-9]*$|^[A-Z0-9_]+$|_t$")

# fmt: off

# Block specific external or library namespaces from tracking as local custom functions
IGNORED_NAMESPACES = {
    'std::', 'ranges::', 'this_thread::', 'Random::', 'sf::', 'json::', 'filesystem::', 'Joystick::', 'VideoMode::'
}

# Core C++ keywords, types, and compiler utilities that mimic custom function shapes
IGNORED_TOKENS = {
    # Control Flow / Core Language Keywords
    'if', 'while', 'for', 'switch', 'catch', 'return', 'requires', 'static_assert', 'sizeof', 'case', 'typeid', 'operator',
    # Primitive Types / Common Type prefixes that instantiate objects locally
    'void', 'int', 'float', 'double', 'char', 'bool', 'auto', 'long', 'short', 'unsigned', 'virtual', 'explicit', 'friend',
    'ifstream', 'ofstream', 'fstream', 'lock_guard', 'unique_lock', 'shared_lock', 'vector', 'string', 'json',
    # Preprocessor / Compiler macros / Standard math & collections entries
    'defined', '__declspec', 'alignas', 'decltype', 'abs', 'labs', 'fabs', 'pow', 'round', 'rand', 'back', 'sqrt', 'assert', 'infinity', 'min', 'max',
    # Common Standard Library elements leaking at root header scopes
    'move', 'visit', 'reverse', 'to_string', 'all_of', 'as_integer', 'main', 'pos',
    # Common system macros or explicit POSIX symbols
    'dlopen', 'dlsym', 'dlclose', 'LOAD_LIBRARY', 'GET_PROC', 'FREE_LIBRARY',

    # Explicitly Excluded Engine Lifecycle / Framework Methods
    'awake', 'create', 'start', 'update', 'draw', 'render', 'clear', 'init', 'destroy', '_delete', 
    'processOrders', 'sendOrders'
}

CONTROL_FLOW_KEYWORDS = {'if', 'while', 'for', 'switch', 'catch', 'return', 'case'}

METHOD_PREFIXES = {'void', 'int', 'float', 'bool', 'auto', 'virtual', 'inline'}

# fmt: on


def is_valid_parameter_list(param_text):
    """
    Differentiates a function signature/call from an object instantiation by checking
    if the parameter list contains literal raw scalar values instead of types/variables.
    """
    param_text = param_text.strip()
    if not param_text:
        return True

    # High certainty object construction metrics (strings/chars)
    if '""' in param_text or "''" in param_text:
        return False

    # Match standalone numeric assignments or constructor scales (e.g., Vec2(0, 0) or Int(10))
    if re.match(r"^\s*-?\d+(?:\.\d+)?(?:f)?\s*$", param_text) or re.match(
        r"^\s*-?\d+\s*,\s*-?\d+\s*$", param_text
    ):
        return False

    return True


def check_naming_violation(
    prefix, class_scope, func_name, param_text, active_assignments
):
    """
    Validates function naming trends across all code scopes by verifying layout prefixes.
    """
    # Skip library, platform, and helper utility namespaces entirely
    if class_scope in IGNORED_NAMESPACES:
        return False

    # Skip standard tokens, lambda definitions, and variables found on the LHS of assignments
    if not func_name or func_name in IGNORED_TOKENS or func_name in active_assignments:
        return False

    # If multiple statements share a single line, isolate the prefix of the current statement
    if prefix and ";" in prefix:
        prefix = prefix.split(";")[-1]

    # Skip member access actions (e.g., window.Draw, pointer->Draw)
    if prefix.endswith(".") or prefix.endswith("->"):
        return False

    # Skip constructor initializer list lines
    if prefix.endswith(",") or (":" in prefix and not prefix.endswith("::")):
        return False

    # Skip class method declarations/definitions and explicit type instantiations
    if prefix:
        words = prefix.split()
        if words:
            last_word = words[-1].strip("&").strip("*")

            # Only skip if the last token is a core control-flow keyword
            if last_word in CONTROL_FLOW_KEYWORDS or last_word.endswith(">"):
                return False

            # Variable instantiation check: e.g., 'Vector2D acc('
            if TYPE_IDENTIFIER_REGEX.match(last_word):
                return False

    # If an explicit scope was passed (e.g. Joystick::getAxisPosition) but there is no
    # clear type signature preceding it on the left, it's a runtime call, not a definition.
    if class_scope and class_scope.endswith("::"):
        if not prefix or not any(w in prefix for w in METHOD_PREFIXES):
            # Verify it isn't an out-of-line class definition line by ensuring it doesn't end a statement block setup
            if not prefix.strip() or prefix.strip()[-1] not in {"}", ";", ":"}:
                return False

    # Filter out object instantiations with literal parameters
    if not is_valid_parameter_list(param_text):
        return False

    # Rule validation: Must be PascalCase
    if func_name[0].islower() or "_" in func_name:
        return True

    return False


def scan_cpp_files(target_dirs):
    total_violations = 0
    extensions = {".h", ".hpp", ".cpp"}

    print("=" * 60)
    print(f"{CYAN}Scanning C++ codebase for naming convention violations...{RESET}")
    print(f"Current Working Directory: {os.getcwd()}")
    print("Rules: Custom functions MUST NOT start with lowercase or contain '_'")
    print("=" * 60 + "\n")

    for target in target_dirs:
        target_path = Path(target)
        if not target_path.exists():
            print(
                f"{YELLOW}Warning: Target directory not found: {target_path.absolute()}{RESET}"
            )
            continue

        for file_path in target_path.rglob("*"):
            if file_path.suffix not in extensions:
                continue

            try:
                raw_text = file_path.read_text(encoding="utf-8", errors="ignore")

                # Extract all identifiers that appear on the left side of an assignment (=)
                active_assignments = set(ASSIGNMENT_REGEX.findall(raw_text))

                in_block_comment = False

                # Loop through line by line directly from the raw data to preserve exact line metrics
                for line_no, line in enumerate(raw_text.splitlines(), 1):
                    line_clean = line.strip()

                    # Handle block comment transitions manually line-by-line
                    if in_block_comment:
                        if "*/" in line_clean:
                            line_clean = line_clean.split("*/", 1)[1]
                            in_block_comment = False
                        else:
                            continue

                    if "/*" in line_clean:
                        if "*/" in line_clean:
                            line_clean = re.sub(r"/\*.*?\*/", "", line_clean)
                        else:
                            line_clean = line_clean.split("/*", 1)[0]
                            in_block_comment = True

                    # Sanitize line contents by stripping inline comments and string literals
                    line_clean = re.sub(r"//.*", "", line_clean)
                    line_clean = re.sub(r'"[^"\\]*(?:\\.[^"\\]*)*"', '""', line_clean)
                    line_clean = re.sub(r"'.'", "''", line_clean)

                    if not line_clean:
                        continue

                    # Evaluate structural signatures globally across all source scopes
                    for match in FUNCTION_REGEX.finditer(line_clean):
                        class_scope = match.group(1) or ""
                        func_name = match.group(2)
                        param_text = match.group(3)

                        # Calculate full index offset including class scope if it exists
                        name_start_idx = (
                            match.start(1) if match.group(1) else match.start(2)
                        )
                        calculated_prefix = line_clean[:name_start_idx].rstrip()

                        if check_naming_violation(
                            calculated_prefix,
                            class_scope,
                            func_name,
                            param_text,
                            active_assignments,
                        ):
                            # Format output to include the class scope name if found
                            full_display_name = f"{class_scope}{func_name}"
                            print(
                                f"{RED}Violation found in: {file_path}:{line_no}{RESET}"
                            )
                            print(f"   Function: '{full_display_name}()'\n")
                            total_violations += 1

            except Exception as e:
                print(f"{YELLOW}Error reading {file_path}: {e}{RESET}")

    print("-" * 60)
    if total_violations == 0:
        print(
            f"{GREEN}Clean bill of health! All custom functions follow PascalCase.{RESET}"
        )
    else:
        print(
            f"{RED}Scan complete. Found {total_violations} naming violation(s).{RESET}"
        )
    print("-" * 60)


if __name__ == "__main__":
    script_dir = Path(__file__).resolve().parent
    source_dir = script_dir / "Source"

    if source_dir.exists() and source_dir.is_dir():
        os.chdir(source_dir)
    else:
        print(
            f"{RED}Fatal Error: Could not find 'Source' folder at {source_dir}{RESET}"
        )
        exit(1)

    directories_to_scan = ["include", "src", "modules"]
    scan_cpp_files(directories_to_scan)
