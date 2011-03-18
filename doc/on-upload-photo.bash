#!/bin/bash
test -z "$targetroot" && targetroot="$(dirname "$EYEFI_UPLOADED")"

make_vars() {
 echo -n "ttype=$1 "
 sed <<<$etime -e 's,^[^/]\+/,,' -e 's, 0*, hour=,' -e 's,^0*,year=,' -e 's,:0*, month=,' -e 's,:0*, day=,' -e 's,:0*, minute=,' -e 's,:0*, second=,'
}

j_time() {
 local j="$1"
 local etime="$(exiftime -s / -tg "$j" 2>/dev/null)"
 [[ -z "$etime" ]] && etime="$(exiftime -s / -td "$j" 2>/dev/null)"
 [[ -z "$etime" ]] && etime="$(exiftime -s / -tc "$j" 2>/dev/null)"
 [[ -z "$etime" ]] && return 1
 make_vars jpg <<<$etime
}

a_time() {
 local a="$1"
 local etime="$(iii-extract-riff-chunk "$a" '/RIFF.AVI /LIST.ncdt/nctg'|dd bs=1 skip=82 count=19 2>/dev/null)"
 [[ -z "$etime" ]] && return 1
 make_vars avi <<<$etime
}

ul="$EYEFI_UPLOADED"

if ! vars="$(j_time "$ul"||a_time "$ul")" ; then
 report="Timeless $(basename "$ul") uploaded"
else
 eval "$vars"
 stem="$(printf '%04d-%02d-%02d--%02d-%02d-%02d' "$year" "$month" "$day" "$hour" "$minute" "$second")"
 targetdir="$(printf "%s/%04d-%02d" "$targetroot" "$year" "$month")"
 mkdir -p "$targetdir"
 success=false
 for((i=0;i<100;++i)) do
  [[ $i = 0 ]] && tf="$stem.$ttype" || tf="$stem ($i).$ttype"
  tf="$targetdir/$tf"
  if ln -T "$ul" "$tf" &>/dev/null && rm "$ul"  ; then
   success=true
   break
  fi
 done
 if $success ; then
  report="$(basename "$tf") uploaded"
  if [[ -n "$EYEFI_LOG" ]] ; then
   ln -T "$EYEFI_LOG" "${tf}.log" && rm "$EYEFI_LOG" || report="$report, but log..."
  fi
 else
  report="$(basename "$ul") uploaded, but..."
 fi
fi
echo "$report"

type iii_report &>/dev/null && iii_report "$report"
