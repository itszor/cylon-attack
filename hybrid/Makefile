# Cylon Attack, hybrid: the game's C compiled natively from ../source, its
# assembler translated from !RunImage (gen/, from ../tools/images.py: make
# regen), linked with libroger (the RISC OS runtime). See game.c.
ROGER   ?= $(HOME)/stuff/libroger
SS      ?= $(HOME)/stuff/superspider
CC      ?= gcc
CFLAGS  ?= -O2 -g
SRC     := ../source
LIBS    := $(shell pkg-config --libs sdl3) -lm -lpthread

# the C of the game and of C3Dlib, as the source's Makefiles link them
# (the game's own list replaces C3Dlib's; sound and the tests aren't linked)
GAME_C  := smoothptinit test scene game list enemy levels title hiscore
C3D_C   := c3d filing intmath object transform vbuf texture2
NATIVE  := $(addprefix build/obj/,$(GAME_C:=.o) $(C3D_C:=.o))
GEN     := $(wildcard gen/*/code*.c) $(wildcard gen/*/entries.c)
OBJ     := $(GEN:.c=.o) build/native.o hyrt.o game.o

# the game's C: the original's headers by their RISC OS names too
# ("C3Dlib:vbuf.h", "C:swis.h"), hybrid.h first
GAMEFLAGS := $(CFLAGS) -std=gnu11 -fno-strict-aliasing -Ibuild/inc -Iinc \
             -include hybrid.h -w

cylon: $(OBJ) $(ROGER)/libroger.a
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(ROGER)/libroger.a $(LIBS)

$(ROGER)/libroger.a: FORCE
	$(MAKE) -C $(ROGER) libroger.a

gen/%.o: gen/%.c $(ROGER)/include/roger_cpu.h
	$(CC) $(CFLAGS) -w -fno-strict-aliasing -I$(ROGER)/include -c -o $@ $<

hyrt.o: hyrt.c hyrt.h inc/swis.h $(ROGER)/include/roger.h
	$(CC) $(CFLAGS) -Wall -fno-strict-aliasing -I$(ROGER)/include -Iinc -c -o $@ $<

game.o: game.c hyrt.h $(ROGER)/include/roger.h
	$(CC) $(CFLAGS) -Wall -I$(ROGER)/include -c -o $@ $<

# the game's C as one object exporting only hy_native, so that its names
# (bank, message...) and libroger's don't meet
build/native.o: $(NATIVE) build/obj/shims.o
	$(LD) -r -o $@ $^
	objcopy -G hy_native $@

build/obj/shims.o: shims.c hyrt.h build/.src hybrid.h
	@mkdir -p build/obj
	$(CC) $(GAMEFLAGS) -c -o $@ $<

# test.c's main is called by the C start-up (shims.c); smoothptinit by a shim
build/obj/test.o: GAMEFLAGS += -Dmain=cylon_main
build/obj/smoothptinit.o: GAMEFLAGS += -Dsmoothptinit=native_smoothptinit

build/obj/%.o: build/.src hybrid.h inc/swis.h inc/kernel.h
	@mkdir -p build/obj
	$(CC) $(GAMEFLAGS) -c -o $@ build/src/$*.c

# the sources, as .c files, with cylon.patch applied; the headers by name
build/.src: cylon.patch $(wildcard $(SRC)/c/* $(SRC)/h/* $(SRC)/C3Dlib/c/* $(SRC)/C3Dlib/h/*)
	rm -rf build/src build/inc
	mkdir -p build/src build/inc
	for f in $(GAME_C); do cp $(SRC)/c/$$f build/src/$$f.c; done
	for f in $(C3D_C); do cp $(SRC)/C3Dlib/c/$$f build/src/$$f.c; done
	patch -s -d build/src -p1 < cylon.patch
	for f in $(SRC)/C3Dlib/h/*; do ln -sf ../../$$f build/inc/$$(basename $$f).h; \
	    ln -sf ../../$$f "build/inc/C3Dlib:$$(basename $$f).h"; done
	for f in $(SRC)/h/*; do ln -sf ../../$$f build/inc/$$(basename $$f).h; done
	ln -sf ../../inc/swis.h "build/inc/C:swis.h"
	ln -sf ../../inc/kernel.h "build/inc/C:kernel.h"
	touch $@

# make regen: analyse and translate the images again
regen:
	SUPERSPIDER=$(SS) python3 ../tools/images.py analyse
	SUPERSPIDER=$(SS) python3 ../tools/images.py gen

clean:
	rm -rf build gen/*/*.o hyrt.o game.o cylon

.PHONY: FORCE regen clean
