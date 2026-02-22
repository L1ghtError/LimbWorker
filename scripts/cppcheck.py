import subprocess
import sys

def run_cppcheck():
    cmd = [
        "cppcheck",
        "--enable=all",
        "--inconclusive",
        "--suppress=unusedFunction",
        "--std=c++17",
        "./include",
        "./src",
        "./processors",
    ]

    print("Running:", " ".join(cmd))
    try:
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        # cppcheck writes most messages to stderr
        if result.stdout:
            print(result.stdout)
        if result.stderr:
            print(result.stderr, file=sys.stderr)

        return result.returncode

    except FileNotFoundError:
        print("Error: cppcheck not found. Make sure it is installed and in PATH.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(run_cppcheck())