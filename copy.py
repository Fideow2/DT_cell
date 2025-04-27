import os
import glob
import subprocess
import platform
import argparse
import fnmatch

def copy_to_clipboard(text):
    system = platform.system()
    try:
        if system == 'Darwin':  # macOS
            process = subprocess.Popen('pbcopy', stdin=subprocess.PIPE)
            process.communicate(text.encode('utf-8'))
            return True
        elif system == 'Windows':
            process = subprocess.Popen(['clip'], stdin=subprocess.PIPE)
            process.communicate(text.encode('utf-8', errors='replace'))
            return True
        elif system == 'Linux':
            # Try xclip first, then xsel
            try:
                process = subprocess.Popen(['xclip', '-selection', 'clipboard'], stdin=subprocess.PIPE)
                process.communicate(text.encode('utf-8'))
                return True
            except FileNotFoundError:
                try:
                    process = subprocess.Popen(['xsel', '--clipboard', '--input'], stdin=subprocess.PIPE)
                    process.communicate(text.encode('utf-8'))
                    return True
                except FileNotFoundError:
                    return False
        return False
    except Exception:
        return False

def parse_arguments():
    parser = argparse.ArgumentParser(description='Copy code from .cpp and .h files to clipboard')
    parser.add_argument('-i', '--include', nargs='+', metavar='PATTERN',
                      help='Include only files matching these patterns (e.g., "main.cpp" or "*.h")')
    parser.add_argument('-e', '--exclude', nargs='+', metavar='PATTERN',
                      help='Exclude files matching these patterns (e.g., "test_*.cpp")')
    return parser.parse_args()

def main():
    args = parse_arguments()

    # Find all .cpp and .h files in the current directory
    cpp_files = glob.glob("*.cpp")
    h_files = glob.glob("*.h")
    all_files = sorted(cpp_files + h_files)

    if not all_files:
        print("No .cpp or .h files found in the current directory.")
        return

    # Apply include filter if specified
    if args.include:
        filtered_files = []
        for pattern in args.include:
            for file in all_files:
                if fnmatch.fnmatch(file, pattern):
                    filtered_files.append(file)
        all_files = sorted(list(set(filtered_files)))  # Remove duplicates

    # Apply exclude filter if specified
    if args.exclude:
        for pattern in args.exclude:
            all_files = [f for f in all_files if not fnmatch.fnmatch(f, pattern)]

    if not all_files:
        print("No files match the specified criteria.")
        return

    # Read and format each file's content
    formatted_content = []
    for filename in all_files:
        try:
            with open(filename, 'r', encoding='utf-8') as file:
                content = file.read()
                formatted_content.append(f"{filename}:\n{content}\n")
        except UnicodeDecodeError:
            # Try with a different encoding if UTF-8 fails
            try:
                with open(filename, 'r', encoding='latin1') as file:
                    content = file.read()
                    formatted_content.append(f"{filename}:\n{content}\n")
            except Exception as e:
                formatted_content.append(f"{filename}: Error reading file - {str(e)}\n")
        except Exception as e:
            formatted_content.append(f"{filename}: Error reading file - {str(e)}\n")

    # Join all formatted contents
    all_content = "\n".join(formatted_content)

    # Try to copy to clipboard
    if copy_to_clipboard(all_content):
        print(f"Content of {len(all_files)} files copied to clipboard!")
        print("Files:", ", ".join(all_files))
    else:
        # Fallback: save to file
        output_file = "code_contents.txt"
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(all_content)
        print(f"Could not access clipboard. Content saved to '{output_file}' instead.")
        print("Files:", ", ".join(all_files))

if __name__ == "__main__":
    main()
