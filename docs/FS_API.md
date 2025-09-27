# Filesystem API (fs)

AdaScript ships with a small cross-platform filesystem helper module available at the global `fs` namespace. These helpers are thin wrappers over C++17 std::filesystem and standard file I/O. Paths are strings.

Quick example:

```ad
let dir = "./tmp/as";
if (!fs.exists(dir)) { fs.mkdirs(dir); }
let p = dir + "/note.txt";
fs.write_text(p, "hello");
print(fs.read_text(p));           // -> hello
print(fs.listdir(dir));           // -> ["note.txt"]
fs.remove(dir);                   // recursively removes directory
```

## API Reference

- string fs.read_text(path)
  - Reads the entire file into a string.
  - Throws runtime error if the file cannot be opened.

- bool fs.write_text(path, text)
  - Writes the given text to a file, overwriting if it exists. Creates the file if needed.
  - Returns true on success; throws on failure.

- bool fs.exists(path)
  - Returns true if the file or directory exists.

- list fs.listdir(path)
  - Returns a list of names within the directory (filenames only, not full paths).
  - Throws on failure (e.g., path does not exist or not a directory).

- bool fs.mkdirs(path)
  - Creates the directory and all missing parents. Returns true on success (or if directory already exists).

- number fs.remove(path)
  - Removes a file or directory.
  - For directories, removal is recursive and returns the count of filesystem entries removed.
  - For files, returns 1 on success, 0 if nothing removed.

## Notes

- Paths are interpreted by the host OS. On Windows, both `C:/path` and `C:\\path` are acceptable; within AdaScript strings, use `\\` to escape backslashes.
- Errors are surfaced as "Runtime error: ..." with a message.
- For large files, read_text loads the whole file into memory; consider chunking in user code if needed.
- fs.remove uses std::filesystem remove/remove_all semantics; failures throw a runtime error.
