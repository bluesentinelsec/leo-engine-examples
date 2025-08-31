#!/bin/bash

# Bash script for macOS to fetch specified license texts and save them in the current working directory.

echo "Fetching licenses..."

# Base URL for SPDX license texts
spdx_base="https://raw.githubusercontent.com/spdx/license-list-data/main/text"

# LGPL-3.0 from GNU (plain text)
curl -o LICENSE-LGPL-3.0.txt https://www.gnu.org/licenses/lgpl-3.0.txt

# MIT License (standard)
curl -o LICENSE-MIT.txt "${spdx_base}/MIT.txt"

# MIT License (dual: MIT or Public Domain; chose MIT) - using standard MIT as equivalent
curl -o LICENSE-MIT-dual.txt "${spdx_base}/MIT.txt"

# MIT License (Lua) - Lua uses standard MIT; falling back to ubiquitous MIT
curl -o LICENSE-MIT-Lua.txt "${spdx_base}/MIT.txt"

# MIT-0 (MIT No Attribution)
curl -o LICENSE-MIT-0.txt "${spdx_base}/MIT-0.txt"

# zlib
curl -o LICENSE-zlib.txt "${spdx_base}/Zlib.txt"

echo "Licenses fetched and saved in the current directory."
