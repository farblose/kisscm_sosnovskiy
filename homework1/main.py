import zipfile
import argparse
import sys
import shutil
import os

def print_tree(directory_dict, root, prefix=""):
    items = directory_dict.get(root, [])
    files = [item for item in items if item not in directory_dict]
    dirs = [item for item in items if item in directory_dict]

    for i, file in enumerate(files):
        connector = "└── " if i == len(files) - 1 and not dirs else "├── "
        print(prefix + connector + file)

    for i, dir in enumerate(dirs):
        connector = "└── " if i == len(dirs) - 1 else "├── "
        print(prefix + connector + dir)
        print_tree(directory_dict, dir, prefix + ("    " if connector == "└── " else "│   "))

class VShell:
    def __init__(self, archive_path):
        self.current_dir = "/"
        self.previous_dir = "/"
        self.archive_path = archive_path
        self.file_system = self.load_archive(archive_path)

    def load_archive(self, path):
        if path.endswith(".zip"):
            with zipfile.ZipFile(path) as zip:
                self.parent_dir = zip.namelist()[0].split("/")[0]
                return ["/" + "/".join(file.split("/")[1:]) for file in zip.namelist()]
        else:
            raise ValueError("Unsupported archive format")
        
    def ls(self, args):
        files = [f.replace(self.current_dir, "", 1) for f in self.file_system if f.startswith(self.current_dir) and f != self.current_dir]
        for file in files:
                if (file.count("/") == 1 and file.endswith("/")) or file.count("/") == 0:
                    print(file)

    def cd(self, args):
        if len(args) == 0:
            self.previous_dir = self.current_dir
            self.current_dir = "/"
        else:
            if not args[0].endswith("/"):
                args[0] += "/"
            if args[0] == "/":
                self.previous_dir = self.current_dir
                self.current_dir = "/"
            elif args[0].count("..") != 0:
                directories = self.current_dir[:-1].split("/")
                if self.current_dir == "/":
                    pass
                elif args[0].count("..") > len(directories) - 1:
                    self.previous_dir = self.current_dir
                    self.current_dir = "/"
                else:
                    self.previous_dir = self.current_dir
                    self.current_dir = "/".join(directories[:-args[0].count("..")]) + "/"
            elif args[0] == "-/":
                self.current_dir, self.previous_dir = self.previous_dir, self.current_dir
            elif any(f.startswith(self.current_dir + args[0]) for f in self.file_system):
                self.previous_dir = self.current_dir
                self.current_dir = self.current_dir + args[0]
            elif any(f.startswith(args[0]) for f in self.file_system):
                self.previous_dir = self.current_dir
                self.current_dir = args[0]
            else:
                print(f"cd: {args[0]}: No such directory")
    
    def tree(self, args):
        files = [f.replace(self.current_dir, "", 1) if not f.replace(self.current_dir, "", 1).endswith("/") else f.replace(self.current_dir, "", 1)[:-1] for f in self.file_system if f.startswith(self.current_dir) and f != self.current_dir]
        files_sorted = {}
        for file in files:
            if len(file.split("/")) != 1:
                file_split = file.split("/")
                if file_split[-2] not in files_sorted.keys():
                    files_sorted[file_split[-2]] = [file_split[-1]]
                else:
                    files_sorted[file_split[-2]].append(file_split[-1])
            else:
                if "/" not in files_sorted.keys():
                    files_sorted["/"] = [file]
                else:
                    files_sorted["/"].append(file)
        print(".")
        print_tree(files_sorted, "/")
        total_dirs = len(files_sorted.keys()) - 1
        total_files = len(files) - total_dirs
        print(f"{total_dirs} directories, {total_files} files")

    def mv(self, args):
        if len(args) != 2:
            print("Usage: mv [source] [destination]")
            return
        source = args[0]
        destination = args[1]
        if not source.startswith("/"):
            source = self.current_dir + source
        if not destination.startswith("/"):
            destination = self.current_dir + destination
        if source == "/":
            print("mv: Cant move / directory")
            return
        if source not in self.file_system and source + "/" not in self.file_system:
            print(f"mv: {source}: No such file or directory")
            return
        if destination not in self.file_system and destination + "/" not in self.file_system:
            print(f"mv: {destination}: No such file or directory")
            return
        if source not in self.file_system:
            source += "/"
        if destination not in self.file_system:
            destination += "/"
        if source == destination:
            return
        if source.endswith("/") and not destination.endswith("/"):
            print("Cant move directory to file")
            return
        if source in destination:
            print(f"Cannot move a directory {source} into itself")
            return
        shutil.unpack_archive(self.archive_path, "buffer")
        source = f"buffer/{self.parent_dir}" + source
        destination = f"buffer/{self.parent_dir}" + destination
        shutil.move(os.path.abspath(source), os.path.abspath(destination))
        os.remove(self.archive_path)
        shutil.make_archive(self.archive_path.replace(".zip", "", 1), format="zip", root_dir="buffer")
        shutil.rmtree("buffer")
        self.file_system = self.load_archive(self.archive_path)
        
    def run_command(self, command):
        parts = command.split()
        if not parts:
            return
        cmd, args = parts[0], parts[1:]

        if cmd == "ls":
            self.ls(args)
        elif cmd == "cd":
            self.cd(args)
        elif cmd == "tree":
            self.tree(args)
        elif cmd == "mv":
            self.mv(args)
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