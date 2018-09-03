all: debug
.PHONY: debug asan release clean

debug:
	@lua make.lua > gen.mk
	@$(MAKE) -f gen.mk

asan:
	@lua make.lua asan > gen.mk
	@$(MAKE) -f gen.mk

release:
	@lua make.lua release > gen.mk
	@$(MAKE) -f gen.mk

clean:
	@lua make.lua debug > gen.mk
	@$(MAKE) -f gen.mk clean
	@lua make.lua asan > gen.mk
	@$(MAKE) -f gen.mk clean
	@rm -f gen.mk

