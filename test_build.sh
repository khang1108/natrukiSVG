#!/bin/bash
cd /run/media/phuckhang/HDD/MyWorkspace/ChuongTrinhDaiHoc-HCMUS/NaTruKi_OOP_Project
rm -rf build
echo "=== Configuring ==="
cmake -S . -B build 2>&1 | tail -10
echo ""
echo "=== Building ==="
cmake --build build 2>&1 | tail -20

