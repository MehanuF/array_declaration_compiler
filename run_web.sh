#!/usr/bin/env bash
# ================================================================
# Array Declaration Compiler — Web Interface Launcher
# ================================================================

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
cd "$DIR"

echo "=========================================================="
echo "Array Declaration Compiler — Web Interface"
echo "=========================================================="

# Ensure compiler binary is built
if [ ! -f "array_compiler" ]; then
    echo "[INFO] Building compiler binary..."
    make
fi

# Check Python and Streamlit
if ! python3 -m streamlit version >/dev/null 2>&1; then
    echo "[INFO] Installing Streamlit dependency..."
    python3 -m pip install streamlit
fi

PORT=8501
IP=$(hostname -I 2>/dev/null | awk '{print $1}')
if [ -z "$IP" ]; then
    IP="127.0.0.1"
fi

# Release port 8501 if held by an inactive or previous instance
if command -v fuser >/dev/null 2>&1; then
    fuser -k "${PORT}/tcp" >/dev/null 2>&1 || true
fi

echo ""
echo "Local Address:   http://localhost:${PORT}"
echo "Network (LAN):   http://${IP}:${PORT}"
echo ""

if [ "$1" == "--share" ]; then
    echo "[INFO] Initializing public endpoint via localtunnel..."
    python3 -m streamlit run app.py --server.port "${PORT}" --server.address 0.0.0.0 --server.headless true &
    STREAMLIT_PID=$!
    sleep 2
    echo "[INFO] Public endpoint:"
    npx --yes localtunnel --port "${PORT}"
    kill $STREAMLIT_PID 2>/dev/null || true
else
    echo "[INFO] Use --share to create a public endpoint (e.g. ./run_web.sh --share)"
    echo "[INFO] Launching Streamlit service..."
    exec python3 -m streamlit run app.py --server.port "${PORT}" --server.address 0.0.0.0
fi
