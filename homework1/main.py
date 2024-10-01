import zipfile
import argparse
import sys

class VShell:
    def __init__(self, archive_path):
        self.current_dir = "/"
        self.previous_dir = "/"
        self.archive_path = archive_path
        self.file_system = self.load_archive(archive_path)

    def load_archive(self, path):
        if path.endswith(".zip"):
            with zipfile.ZipFile(path) as zip:
                return ["/".join(file.split("/")[1:]) for file in zip.namelist() if "/".join(file.split("/")[1:]) != ""]
        else:
            raise ValueError("Unsupported archive format")
        
    def ls(self, args):
        for file in self.file_system:
            if (file.endswith("/") and file.count("/") == 1) or file.count("/") == 0:
                print(file)
        
    def run_command(self, command):
        parts = command.split()
        if not parts:
            return
        cmd, args = parts[0], parts[1:]

        if cmd == "ls":
            self.ls(args)
        elif cmd == "exit":
            sys.exit(0)
        else:
            print(f"vshell: {cmd}: Command not found")
        

def main():
    parser = argparse.ArgumentParser(description="Virtual Shell")
    parser.add_argument("user", help="User's name")
    parser.add_argument("pcname", help="PC's name")
    parser.add_argument("archive", help="archive path")
    args = parser.parse_args()

    shell = VShell(args.archive)

    while True:
        try:
            command = input(f"{args.user}@{args.pcname}:{shell.current_dir}$ ")
            shell.run_command(command)
        except EOFError:
            break


if __name__ == "__main__":
    main()