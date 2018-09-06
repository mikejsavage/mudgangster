all: debug
.PHONY: debug asan release clean

build/lua_combined.h: src/lua/action.lua src/lua/alias.lua src/lua/chat.lua src/lua/mud.lua src/lua/event.lua src/lua/gag.lua src/lua/handlers.lua src/lua/intercept.lua src/lua/interval.lua src/lua/macro.lua src/lua/main.lua src/lua/script.lua src/lua/serialize.lua src/lua/status.lua src/lua/sub.lua src/lua/utils.lua src/lua/socket.lua
	@printf "\033[1;33mbuilding $@\033[0m\n"
	@scripts/pack_lua.sh

debug: build/lua_combined.h
	@lua make.lua > gen.mk
	@$(MAKE) -f gen.mk

asan: build/lua_combined.h
	@lua make.lua asan > gen.mk
	@$(MAKE) -f gen.mk

release: build/lua_combined.h
	@lua make.lua release > gen.mk
	@$(MAKE) -f gen.mk

clean:
	@lua make.lua debug > gen.mk
	@$(MAKE) -f gen.mk clean
	@lua make.lua asan > gen.mk
	@$(MAKE) -f gen.mk clean
	@rm -f gen.mk

