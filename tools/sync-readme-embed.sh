#!/usr/bin/env bash
# Sync the repo README into the Pages embed copy (docs/_includes/readme-embed.md).
# Repo-relative doc links are rewritten to absolute GitHub URLs so they work on the site.
set -euo pipefail
cd /mnt/d/Basalt
mkdir -p docs/_includes
sed \
  -e 's|(docs/adr/)|(https://github.com/BS-O/Basalt/tree/main/docs/adr/)|g' \
  -e 's|(docs/|(https://github.com/BS-O/Basalt/blob/main/docs/|g' \
  -e 's|(third_party/|(https://github.com/BS-O/Basalt/blob/main/third_party/|g' \
  README.md > docs/_includes/readme-embed.md
echo "synced: docs/_includes/readme-embed.md ($(wc -c < docs/_includes/readme-embed.md) bytes)"
