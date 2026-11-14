#!/bin/sh
# Runs every Checkpoint 1 case with the course script and copies the logs into
# evidence/cp1/. This script only orchestrates commands and records results;
# all packet and protocol logic is in the C++ programs.
#
# Usage, from the project root on a course Linux PC:
#   sh tests/run_cp1.sh PATH_TO_FieldDrop_Course_Tools
#
# The course script needs the unmodified FieldDrop_Student_Starter folder next
# to the course tools folder (it reads its fixtures from there).

set -u

tools=${1:?usage: sh tests/run_cp1.sh PATH_TO_FieldDrop_Course_Tools}
tools=$(CDPATH= cd -- "$tools" && pwd) || exit 2
here=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
fixtures="$tools/../FieldDrop_Student_Starter/tests"
out="$here/evidence/cp1"

if [ ! -x "$tools/scripts/run_case.sh" ] || [ ! -x "$tools/fielddrop-recv" ]; then
    echo "Course tools not found in $tools" >&2; exit 2
fi
if [ ! -d "$fixtures" ]; then
    echo "Put the unmodified FieldDrop_Student_Starter folder next to $tools" >&2; exit 2
fi
if [ -e "$out" ]; then
    echo "$out already exists; move it aside so old evidence is not mixed in" >&2; exit 2
fi
mkdir -p "$out"

cd "$here" || exit 2
{
    echo "Date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
    echo "Host: $(uname -a)"
    echo "Course tools: $(cat "$tools/VERSION" | tr '\n' ' ')"
    echo "Git commit: $(git rev-parse HEAD 2>/dev/null || echo 'not a git checkout')"
    echo "g++: $(g++ --version | head -n 1)"
} > "$out/environment.txt"

make clean > /dev/null && make > "$out/build.txt" 2>&1 || { echo "build failed, see $out/build.txt" >&2; exit 1; }
make check-vectors > "$out/packet-vector-check-linux.txt" 2>&1
echo "packet vectors: exit $?" | tee -a "$out/summary.txt"

send="$here/fielddrop-send"
recv="$here/fielddrop-recv"
ref_send="$tools/fielddrop-send"
ref_recv="$tools/fielddrop-recv"

# run_one NAME CASE SENDER RECEIVER INPUT_FIXTURE
run_one() {
    name=$1; n=$2; s=$3; r=$4; input=$5
    dest="$out/$name"
    mkdir -p "$dest"
    echo "== $name: run_case.sh $n $s $r" | tee -a "$out/summary.txt"
    echo "\"$tools/scripts/run_case.sh\" $n \"$s\" \"$r\"" > "$dest/command.txt"
    "$tools/scripts/run_case.sh" "$n" "$s" "$r" > "$dest/run_case_output.txt" 2>&1
    echo "run_case.sh exit: $?" >> "$dest/run_case_output.txt"
    cat "$dest/run_case_output.txt" | tee -a "$out/summary.txt"

    folder=$(sed -n 's/.*folder=\([^ ]*\).*/\1/p' "$dest/run_case_output.txt")
    if [ -n "$folder" ] && [ -d "$folder" ]; then
        cp -R "$folder"/. "$dest/results-folder/"
        {
            echo "Results folder: $folder"
            echo "Listing:"
            ls -la "$folder"
            if [ -n "$input" ]; then
                echo "Input SHA-256:"
                sha256sum "$input"
            fi
            if [ -e "$folder/output.bin" ]; then
                echo "Output SHA-256:"
                sha256sum "$folder/output.bin"
                echo "Output bytes (hex):"
                od -A d -t x1 "$folder/output.bin" | head -n 20
            else
                echo "No output.bin present."
            fi
        } > "$dest/hashes.txt" 2>&1
    fi
}

run_one case01-empty        1  "$send" "$recv" "$fixtures/empty.bin"
run_one case02-abc          2  "$send" "$recv" "$fixtures/abc.txt"
run_one case03-allbytes     3  "$send" "$recv" "$fixtures/all_bytes.bin"
run_one case04-8k-delay     4  "$send" "$recv" "$fixtures/kathara_8k.bin"
run_one case15-oversize     15 "$send" "$recv" ""
run_one case16-existing     16 "$send" "$recv" ""
run_one case02-oursend-refrecv 2 "$send" "$ref_recv" "$fixtures/abc.txt"
run_one case02-refsend-ourrecv 2 "$ref_send" "$recv" "$fixtures/abc.txt"

echo
echo "Evidence saved in $out"
grep -E 'PASS|FAIL' "$out/summary.txt"
