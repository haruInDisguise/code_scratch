#!/usr/bin/sh

TEXT="${1:?Missing text}"
OUT="${2:?Missing output}"

echo -en 'static const bytes[] = {' > "$OUT"
od --format=x1 --endian=big -A none "$TEXT" | tr -d '\n' | sed -E 's/\s/, 0x/g' >> "$OUT"
echo -e '}' >> "$OUT"

