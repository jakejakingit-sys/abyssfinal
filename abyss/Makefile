# ABYSS - CEdev build. Produces bin/ABYSS.8xp (program). Assets are separate archived AppVars:
#   make appvars  ->  bin/ABYSSCOR.8xv, bin/ABYSSLV1.8xv ... (sprites/palettes and levels, read in place from flash)
NAME        = ABYSS
DESCRIPTION = "ABYSS"
COMPRESSED  = YES
CFLAGS      = -Wall -Wextra -Oz -Iinclude
CXXFLAGS    = -Wall -Wextra -Oz -Iinclude

include $(shell cedev-config --makefile)

# convbin ships with CEdev. Each data/*.bin becomes an AppVar named after the file.
APPVAR_BINS := $(wildcard data/ABYSS*.bin)
APPVARS     := $(patsubst data/%.bin,bin/%.8xv,$(APPVAR_BINS))
.PHONY: appvars
appvars: $(APPVARS)
bin/%.8xv: data/%.bin
	@mkdir -p bin
	convbin -j bin -k 8xv -n $* -i $< -o $@
