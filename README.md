# DolphinDB Python SDK

A C++ boosted API for Python, built on Pybind11.

## Requirements

To use DolphinDB Python SDK, you'll need:

- Python:
  - CPython: version 3.11 and newer, including 3.14 (standard GIL build)
- DolphinDB Server
- Packages:
  - NumPy: version 2.3.3 and newer, but earlier than 3.0
  - pandas: version 2.3.3 and newer, but earlier than 4.0 (both 2.x and 3.x)
  - future
  - packaging
  - pydantic: version 2.0 and newer
- Extension Packages:
  - PyArrow: version 9.0.0 and newer

## Installation

To install the DolphinDB package, use the following command:

```sh
pip install dolphindb
```

## Build from source (Python 3.14 example)

On macOS, install the Xcode command line tools and Homebrew OpenSSL first.
UUID is provided by the macOS SDK; an external libuuid is not required.

```sh
brew install python@3.14 openssl@3
python3.14 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
export OPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
export CMAKE_BUILD_PARALLEL_LEVEL=8
python -m pip wheel . --no-deps --wheel-dir dist
python -m pip install dist/dolphindb-*-cp314-cp314-*.whl
```

The wheel build installs CMake, pybind11 and the other build dependencies in
an isolated environment. On Linux, install a C++17 compiler, Python 3.14
development headers, OpenSSL development headers and libuuid development
headers before running the same Python commands (omit the Homebrew commands).
These commands build a local wheel; the existing GitLab release jobs and
custom manylinux images require a separate update before publishing 3.14 wheels.

Run the offline compatibility tests against the installed wheel:

```sh
python -m pip install pyarrow "pandas>=2.3.3,<3"
python -I -m unittest discover -s test/compat -v
python -m pip install --upgrade "pandas>=3,<4"
python -I -m unittest discover -s test/compat -v
```

`-I` prevents the source checkout from shadowing the installed package.
`PROTOCOL_PICKLE` remains unavailable on Python 3.14. Arrow timestamp support
is unchanged: use `timestamp[ms]` or `timestamp[ns]` for timestamp columns.

## Example Code

Here's an example of using DolphinDB Python SDK:

```python
import dolphindb as ddb

conn = ddb.Session()
conn.connect("localhost", 8848, "admin", "123456")

conn.run("1+1;")
-----------------------
2
```
