import zipfile

class VShell:
    def __init__(self, archive_path, logfile):
        self.current_dir = "/"
        self.previous_dir = "/"
        self.archive_path = archive_path
        self.file_system = self.load_archive(archive_path)

    def load_archive(self, path):
        if path.endswith(".zip"):
            pass
        else:
            raise ValueError("Unsupported archive format")