#! /bin/sh

set -o pipefail

mkdir -p build
exec lua scripts/merge.lua src/lua main.lua | luac -o - - | xxd -i > build/lua_bytecode.h
