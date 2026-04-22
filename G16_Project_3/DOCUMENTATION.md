# G16 Project 3 Documentation

## 1. Project Summary
G16 Project 3 is a multithreaded file management server written in C. It exposes a RESP-compatible TCP protocol, so `redis-cli` can be used directly as a client.

The server supports core file operations, metadata queries, audit log retrieval, and simple RLE-based compression/decompression, while enforcing path safety and per-file synchronization.

## 2. Architecture
Core modules:
- `src/server.c`: socket setup, client session handling, command dispatch.
- `src/resp.c`, `src/resp.h`: RESP request parsing and response serialization.
- `src/file_ops.c`, `src/file_ops.h`: command handlers for file operations.
- `src/locks.c`, `src/locks.h`: lock registry for per-path read/write synchronization.
- `src/path_utils.c`, `src/path_utils.h`: canonicalization and safe path checks.
- `src/logger.c`, `src/logger.h`: thread-safe operation logging.

## 3. Supported Commands
- `PING [msg]`
- `GETFILE <path>`
- `SETFILE <path> <content>`
- `DELFILE <path>`
- `RENAMEFILE <old_path> <new_path>`
- `COPYFILE <src_path> <dst_path>`
- `STATFILE <path>`
- `LOGS [line_count]`
- `COMPRESS <src_path> [dst_path]`
- `DECOMPRESS <src_path> [dst_path]`

## 4. Build and Run
Build:
```bash
make
```

Run:
```bash
./filesrv -p 6380 -d ./data
```

Sample client usage:
```bash
redis-cli -p 6380 PING
redis-cli -p 6380 SETFILE notes/hello.txt "hello"
redis-cli -p 6380 GETFILE notes/hello.txt
redis-cli -p 6380 LOGS 20
```

## 5. Concurrency and Safety Notes
- Locking is done per canonical file path.
- Multi-path commands lock both paths in sorted order to avoid deadlocks.
- Unsafe paths are rejected (`..` and absolute paths are not allowed).
- Audit events are written to `<data_dir>/operations.log`.

## 6. Compression Format
- Uses run-length encoding with `RLE1` header.
- Default outputs:
  - `COMPRESS file.txt` -> `file.txt.rle`
  - `DECOMPRESS file.txt.rle` -> `file.txt`
  - Non-`.rle` decompression source defaults to `<source>.out`

## 7. Contribution Breakdown (Who Contributed What)
Attribution below is based on git history for files inside this project.

- `armoredvortex`
  - `src/server.c`
  - `src/file_ops.h`
  - `Makefile`
  - Core server bootstrap/integration and build wiring.

- `crypticsaiyan`
  - `src/file_ops.c`
  - `README.md`
  - File-operation implementation and project-level usage documentation.

- `balaji7416`
  - `src/resp.c`
  - `src/logger.c`
  - RESP processing and logging implementation.

- `Ranish-Garg`
  - `src/resp.h`
  - `src/logger.h`
  - RESP and logger interface design (header-level contracts).

- `hawk_wild`
  - `src/path_utils.c`
  - `src/locks.c`
  - Path validation/canonicalization logic and lock-system implementation.

- `Rajarshi Chakraborty`
  - `src/path_utils.h`
  - `src/locks.h`
  - Header/interface definitions for path and lock modules.

## 8. Screenshot Placeholders
Use the paths below to add screenshots for report/demo submission.

- Build output screenshot:
  ![Project 3 Build Output](screenshots/build_output.png)
- Server startup screenshot:
  ![Project 3 Server Startup](screenshots/server_start.png)
- RESP command demo screenshot:
  ![Project 3 Command Demo](screenshots/command_demo.png)
- Logs/operations screenshot:
  ![Project 3 Logs Demo](screenshots/logs_demo.png)

Create the `screenshots/` folder in this project and place images using the same filenames (or update the links if you choose different names).

## 9. Known Limitations
- Compression is simple RLE and may not reduce size for high-entropy data.
- Functionality is scoped to a configured data directory (by design).
- Access control/authentication is not part of the current protocol scope.
