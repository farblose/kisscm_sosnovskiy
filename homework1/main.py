import zipfile

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