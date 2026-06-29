#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
export PATH="/opt/homebrew/bin:$PATH"

PORT="${NSCM_SCREENSHOT_PORT:-8788}"
DURATION="${NSCM_SCREENSHOT_TUNNEL_SECONDS:-900}"
OUT_DIR="${NSCM_SCREENSHOT_HOST_DIR:-/tmp/nscolourmap-screenshot-host}"
SITE_DIR="$OUT_DIR/site"
HTTP_LOG="$OUT_DIR/http.log"
CLOUDFLARED_LOG="$OUT_DIR/cloudflared.log"

mkdir -p "$SITE_DIR"

cmake --build build --target NSColourMap_Screenshot -j 6 >/tmp/nscolourmap-screenshot-build.log 2>&1
./build/NSColourMap_Screenshot "$SITE_DIR" >/tmp/nscolourmap-screenshot-render.log 2>&1

cat >"$SITE_DIR/index.html" <<'HTML'
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>NSColourMap UI screenshots</title>
  <style>
    :root { color-scheme: dark; font-family: -apple-system, BlinkMacSystemFont, "Helvetica Neue", sans-serif; }
    body { margin: 0; background: #0e1012; color: #edf2f6; }
    main { width: min(1280px, calc(100vw - 32px)); margin: 24px auto 48px; }
    h1 { font-size: 20px; font-weight: 500; margin: 0 0 18px; color: #dce6ed; }
    figure { margin: 0 0 28px; }
    img { display: block; width: 100%; height: auto; border-radius: 10px; box-shadow: 0 24px 72px rgba(0,0,0,.52); }
    figcaption { margin-top: 8px; font-size: 13px; color: #96a3ad; }
  </style>
</head>
<body>
  <main>
    <h1>NSColourMap UI screenshots</h1>
    <figure>
      <img src="./screenshot_main.png" alt="NSColourMap main UI screenshot">
      <figcaption>Main</figcaption>
    </figure>
    <figure>
      <img src="./screenshot_about.png" alt="NSColourMap about UI screenshot">
      <figcaption>About</figcaption>
    </figure>
  </main>
</body>
</html>
HTML

cleanup() {
  if [[ -n "${CLOUDFLARED_PID:-}" ]]; then kill "$CLOUDFLARED_PID" >/dev/null 2>&1 || true; fi
  if [[ -n "${HTTP_PID:-}" ]]; then kill "$HTTP_PID" >/dev/null 2>&1 || true; fi
}
trap cleanup EXIT INT TERM

python3 -m http.server "$PORT" --bind 127.0.0.1 --directory "$SITE_DIR" >"$HTTP_LOG" 2>&1 &
HTTP_PID=$!

for _ in $(seq 1 40); do
  if curl -fsS "http://127.0.0.1:$PORT/" >/dev/null 2>&1; then
    break
  fi
  sleep 0.25
done

cloudflared tunnel --url "http://127.0.0.1:$PORT" --no-autoupdate >"$CLOUDFLARED_LOG" 2>&1 &
CLOUDFLARED_PID=$!

URL=""
for _ in $(seq 1 80); do
  URL="$(grep -Eo 'https://[a-zA-Z0-9.-]+\.trycloudflare\.com' "$CLOUDFLARED_LOG" | head -1 || true)"
  if [[ -n "$URL" ]]; then
    break
  fi
  sleep 0.5
done

if [[ -z "$URL" ]]; then
  echo "cloudflared URL was not emitted. Log: $CLOUDFLARED_LOG" >&2
  exit 1
fi

echo "$URL"
echo "Local: http://127.0.0.1:$PORT/"
echo "Serving for ${DURATION}s. Logs: $HTTP_LOG $CLOUDFLARED_LOG"

sleep "$DURATION"
