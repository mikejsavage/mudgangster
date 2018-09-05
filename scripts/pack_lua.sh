#! /bin/sh

set -o pipefail

mkdir -p build
lua scripts/merge.lua src/lua main.lua > build/lua_combined.lua
#exec lua scripts/merge.lua src/lua main.lua | luac -o - - | lua scripts/bin2arr.lua > build/lua_bytecode.h
exec lua scripts/merge.lua src/lua main.lua | lua scripts/bin2arr.lua > build/lua_bytecode.h
