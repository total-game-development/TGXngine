import os
import re
from pathlib import Path

# ANSI Escape Codes for colored terminal output
RED = "\033[91m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
RESET = "\033[0m"

# Structural markers that strongly imply the comment contains leftover code
CODE_INDICATORS = [
    r";\s*$",  # Ends with a semicolon (e.g., // x = 5;)
    r"\b(if|for|while|switch)\s*\(",  # Control flow statements (e.g., // if (condition))
    r"==|!=|<=|>=|=|\+=|-=",  # Assignment or comparison operators
    r"^\s*\}\s*else\s*\{",  # Mid-block brackets (e.g., // } else {)
    r"\b(?:void|int|float|bool)\b.*\([^)]*\)",  # Function definitions/calls with basic types
]

COMPILED_INDICATORS = [re.compile(pattern) for pattern in CODE_INDICATORS]

# Cleaned up: No more asterisk strings here to completely avoid multiple repeat compilation crashes
IGNORED_PATTERNS = re.compile(r"@brief|@param|@return|http://|https://")

# Regex to find and remove valid string literals so we don't treat URLs inside strings as comments
STRING_LITERAL_REGEX = re.compile(
    r'"[^"\\]*(?:\\.[^"\\]*)*"|\'[^\'\\]*(?:\\.[^\'\\]*)*\''
)


def analyze_comment_text(comment_body):
    """
    Evaluates the cleaned text of a comment block. Returns True if the text
    exhibits structural signatures matching C++ source code.
    """
    comment_body = comment_body.strip()

    # If empty or matches explicit documentation/web protocols, skip it
    if not comment_body or IGNORED_PATTERNS.search(comment_body):
        return False

    # Skip ASCII lines (body is made of just 1 unique repeating symbol)
    if len(set(comment_body)) == 1:
        return False

    # Check against our compiled structural C++ indicators
    for indicator in COMPILED_INDICATORS:
        if indicator.search(comment_body):
            return True

    return False


def scan_for_commented_code(target_dirs):
    total_violations = 0
    extensions = {".h", ".hpp", ".cpp"}

    print("=" * 60)
    print(f"{CYAN}Scanning C++ codebase for dead/commented-out code...{RESET}")
    print(f"Current Working Directory: {os.getcwd()}")
    print("Rules: Flags comments containing semicolons, operators, or keywords.")
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
                lines = raw_text.splitlines()

                in_block_comment = False
                block_comment_accumulator = []
                block_start_line = 0

                for line_no, line in enumerate(lines, 1):
                    line_str = line.strip()

                    # Handle Block Comments (/* ... */)
                    if in_block_comment:
                        if "*/" in line_str:
                            parts = line_str.split("*/", 1)
                            block_comment_accumulator.append(parts[0])

                            # Analyze accumulated block comment content
                            full_block_text = "\n".join(block_comment_accumulator)
                            if any(
                                analyze_comment_text(line)
                                for line in full_block_text.splitlines()
                            ):
                                print(
                                    f"{RED}Commented code block found in: {file_path}:{block_start_line}-{line_no}{RESET}"
                                )
                                print(f"   Context:\n{full_block_text}\n")
                                total_violations += 1

                            in_block_comment = False
                            block_comment_accumulator = []
                            line_str = parts[
                                1
                            ].strip()  # Keep scanning remainder of line
                        else:
                            block_comment_accumulator.append(line_str)
                            continue

                    if "/*" in line_str:
                        if "*/" in line_str:
                            # Inline block comment match (e.g. /* int x = 0; */)
                            inline_match = re.search(r"/\*(.*?)\*/", line_str)
                            if inline_match and analyze_comment_text(
                                inline_match.group(1)
                            ):
                                print(
                                    f"{RED}Commented code found in: {file_path}:{line_no}{RESET}"
                                )
                                print(f"   Line: '{line_str}'\n")
                                total_violations += 1
                        else:
                            parts = line_str.split("/*", 1)
                            block_comment_accumulator.append(parts[1])
                            block_start_line = line_no
                            in_block_comment = True
                            continue

                    # Handle Inline Comments (// ...)
                    # Strip out strings completely FIRST so nested URIs aren't parsed as active syntax
                    sanitized_line = STRING_LITERAL_REGEX.sub("", line_str)

                    if "//" in sanitized_line:
                        # Split by the comment marker on the string-stripped variant
                        _, comment_part = sanitized_line.split("//", 1)

                        # Verify the comment payload contents
                        if analyze_comment_text(comment_part):
                            print(
                                f"{RED}Commented code line found in: {file_path}:{line_no}{RESET}"
                            )
                            print(f"   Line: '{line_str}'\n")
                            total_violations += 1

            except Exception as e:
                print(f"{YELLOW}Error reading {file_path}: {e}{RESET}")

    print("-" * 60)
    if total_violations == 0:
        print(f"{GREEN}Clean bill of health! No commented-out code detected.{RESET}")
    else:
        print(
            f"{RED}Scan complete. Found {total_violations} instances of commented code.{RESET}"
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
    scan_for_commented_code(directories_to_scan)
