#! /bin/sh

set -o pipefail

mkdir -p build
exec lua scripts/merge.lua src/lua main.lua | luac -o - - | lua scripts/bin2arr.lua > build/lua_bytecode.h
