#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo 'usage: sh kathara/stage_submission.sh PATH_TO_EXTRACTED_GROUP_FOLDER' >&2
    exit 2
fi
lab_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
group_dir=$(CDPATH= cd -- "$1" && pwd)
target="$lab_dir/shared/fielddrop"
if [ ! -f "$group_dir/Makefile" ] || [ ! -d "$group_dir/source" ]; then
    echo 'Extract the submitted group ZIP first; its root must contain Makefile and source/' >&2
    exit 2
fi
if [ -e "$target" ]; then
    echo 'Staged files already exist. Use a clean copy of the starter Kathara lab for the assessed run.' >&2
    exit 2
fi
mkdir -p "$target"
cp "$group_dir/Makefile" "$target/Makefile"
cp -R "$group_dir/source" "$target/source"
printf 'Staged submitted Makefile and source in %s\n' "$target"
