# Project2 - Multithreaded File Management Server (C)

This project is a RESP-compatible TCP server in C, so you can use redis-cli as the client.

The codebase is modularized into:

- `src/server.c`: socket server, command dispatcher, and client threads.
- `src/resp.c`, `src/resp.h`: RESP parsing and reply encoding.
- `src/file_ops.c`, `src/file_ops.h`: file operation handlers.
- `src/locks.c`, `src/locks.h`: per-file read/write lock registry.
- `src/path_utils.c`, `src/path_utils.h`: path validation and directory helpers.
- `src/logger.c`, `src/logger.h`: thread-safe audit logging.

Implemented commands:

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

Compression format:

- RLE with `RLE1` header.
- If destination is omitted:
- `COMPRESS file.txt` writes to `file.txt.rle`.
- `DECOMPRESS file.txt.rle` writes to `file.txt`.
- If source does not end with `.rle`, decompression default output is `<source>.out`.

## Build

```bash
make
```

## Run

```bash
./filesrv -p 6380 -d ./data
```

Notes:

- Paths are relative to the configured data directory.
- Unsafe paths are rejected (`..` and absolute paths are not allowed).
- Audit log is written to `<data_dir>/operations.log`.
- Read/write locking is per canonical full path.
- Multi-file operations (`RENAMEFILE`, `COPYFILE`, `COMPRESS`, `DECOMPRESS`) lock both paths in sorted order to avoid deadlocks.

## Test using redis-cli

In one terminal:

```bash
./filesrv -p 6380 -d ./data
```

In another terminal:

```bash
redis-cli -p 6380 PING
redis-cli -p 6380 SETFILE notes/hello.txt "hello from redis-cli"
redis-cli -p 6380 GETFILE notes/hello.txt
redis-cli -p 6380 COPYFILE notes/hello.txt notes/hello_copy.txt
redis-cli -p 6380 STATFILE notes/hello_copy.txt
redis-cli -p 6380 COMPRESS notes/hello.txt
redis-cli -p 6380 DECOMPRESS notes/hello.txt.rle notes/hello_restored.txt
redis-cli -p 6380 LOGS 20
```
