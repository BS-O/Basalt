#!/usr/bin/env bash
# Sync the repo READMEs into the Pages embed copies (docs/_includes/readme-embed.md
# and readme-en-embed.md). Repo-relative doc links are rewritten to absolute
# GitHub URLs so they work on the site.
set -euo pipefail
cd /mnt/d/Basalt
mkdir -p docs/_includes

rewrite() {
  sed \
    -e 's|(docs/adr/)|(https://github.com/BS-O/Basalt/tree/main/docs/adr/)|g' \
    -e 's|(docs/|(https://github.com/BS-O/Basalt/blob/main/docs/|g' \
    -e 's|(third_party/|(https://github.com/BS-O/Basalt/blob/main/third_party/|g' \
    -e 's|(README-EN.md)|(https://basalt.bsct.top/readme/en/)|g' \
    -e 's|(README.md)|(https://basalt.bsct.top/readme/)|g' \
    "$1"
}

rewrite README.md > docs/_includes/readme-embed.md
rewrite README-EN.md > docs/_includes/readme-en-embed.md
echo "synced: readme-embed.md ($(wc -c < docs/_includes/readme-embed.md) bytes), readme-en-embed.md ($(wc -c < docs/_includes/readme-en-embed.md) bytes)"
