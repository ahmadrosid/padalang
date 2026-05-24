#!/usr/bin/env bash
set -euo pipefail

echo "==> Building padalang..."
make clean
make

echo ""
echo "==> Build succeeded! Run with:"
echo "      ./padalang examples/hello.pad"
echo "      ./padalang --repl"
echo ""
echo "==> To install system-wide:"
echo "      sudo make install"
