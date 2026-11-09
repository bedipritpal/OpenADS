#!/usr/bin/env bash
# Actual pinned Harbour RDD error-chain test. Test-only call seams do not enter kit.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HB="$(cd "$1" && pwd)"
BUILD="$(cd "$2" && pwd)"
OUT="$ROOT/output/harbour-batch"
mkdir -p "$OUT"
HBINC="$HB/include"
RDD="$HB/contrib/rddads"
HBBIN="$(find "$HB/bin" -type f \( -name hbmk2 -o -name hbmk2.exe \) | head -1)"
test -n "$HBBIN"
# No change to real DLL functions: only RDD test objects use these named seams.
for unit in ads1 adsfunc adsmgmnt adsx;do
 source="$RDD/$unit.c"
 [ "$unit" != ads1 ] || source="$ROOT/tests/harbour_batch/ads1-test.c"
 gcc -c "$source" -DDOUBLE=double -DWCHAR=UNSIGNED16 -DVOID=void \
  -DAdsAppendRecord=test_AdsAppendRecord -DAdsGotoRecord=test_AdsGotoRecord \
  -DAdsExecuteSQLDirect=test_AdsExecuteSQLDirect -DAdsExecuteSQL=test_AdsExecuteSQL \
  -I"$HBINC" -I"$RDD" -I"$ROOT/include/openads" -o "$OUT/$unit.o"
done
ar rcs "$OUT/librddads.a" "$OUT/ads1.o" "$OUT/adsfunc.o" "$OUT/adsmgmnt.o" "$OUT/adsx.o"
EXTRA=()
if [ -f "$ROOT/tests/legacy-native-temp.h" ];then EXTRA=(-include "$ROOT/tests/legacy-native-temp.h");fi
g++ -std=c++17 "${EXTRA[@]}" -c "$ROOT/tests/harbour_batch/integration.cpp" \
 -I"$HBINC" -I"$RDD" -I"$ROOT/src" -I"$ROOT/include" -I"$ROOT/include/openads" -o "$OUT/integration.o"
mapfile -t LIBS < <(find "$BUILD" -name '*.a' ! -name '*dll.a' ! -path '*/CMakeFiles/*' | sort)
LINKS=(-ldflag=-Wl,--start-group)
for lib in "${LIBS[@]}";do LINKS+=("-ldflag=$lib");done
LINKS+=(-ldflag=-Wl,--end-group -lstdc++ -lpthread)
if [[ "${OS:-}" == Windows_NT ]];then LINKS+=(-lws2_32 -lbcrypt -lpsapi);else LINKS+=(-ldl);fi
"$HBBIN" "$ROOT/tests/harbour_batch/integration.prg" "$OUT/integration.o" -L"$OUT" -lrddads "${LINKS[@]}" -mt -gtstd -o"$OUT/harbour-batch"
"$OUT/harbour-batch" > "$OUT/runtime.log" 2>&1
cat "$OUT/runtime.log"
grep -q HARBOUR-BATCH-ERROR-PASS "$OUT/runtime.log"
grep -q 'DEFERRED-BEFORE-APPEND-PASS command' "$OUT/runtime.log"
grep -q 'SQL-CLOSE-FAILURE-RETAINED-PASS sql-prepared' "$OUT/runtime.log"
