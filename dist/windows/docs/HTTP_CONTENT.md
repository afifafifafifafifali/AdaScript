# HTTP and Content Retrieval APIs

This document describes the HTTP client (requests) and the content retrieval helper (content).

- On Windows, HTTP is implemented with WinHTTP.
- On Linux/WSL, HTTP is implemented with libcurl.
- `file://` URLs are supported on both platforms and map to local filesystem reads.

## requests (namespace)

- requests.get(url) -> dict { status, text, headers? }
- requests.post(url, data?, headers?) -> dict
- requests.request(method, url, data?, headers?) -> dict

Examples:
```ad
let r = requests.get("https://example.org/");
print(r.status, len(r.text));
let p = requests.post("https://httpbin.org/post", "x=1", {"Content-Type":"application/x-www-form-urlencoded"});
print(p.status, len(p.text));
```

Notes:
- `status` is a number (HTTP status code).
- `text` is the response body as a string.
- `headers` may contain a limited subset (e.g., Content-Type, Server on Windows). Header availability varies by backend.
- For `file://path`, the body is the file contents and status is 200 on success.
- On Linux/WSL builds without libcurl, HTTP is disabled and any requests.* call throws: "HTTP disabled: libcurl not available in this build".

## content (namespace)

- content.get(source) -> dict with:
  - ok: bool
  - status: number (200 on success)
  - text: content string
  - type: "http" or "file"
  - source: original source string
  - error: string (present on failure)

Examples:
```ad
let r1 = content.get("https://example.org/");
let r2 = content.get("file://C:/Windows/win.ini");
let r3 = content.get("C:/Windows/win.ini");
print(r1.ok, len(r1.text));
print(r2.ok, len(r2.text));
print(r3.ok, len(r3.text));
```

Error handling:
- Network or file errors will return `{ ok: false, status: 500, error: "..." }`.
- Non-existent local paths return `{ ok: false, status: 404, error: "not found" }`.
