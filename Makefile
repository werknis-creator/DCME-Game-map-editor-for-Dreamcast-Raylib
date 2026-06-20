######################################################################
# Makefile dla GTAsaDC - Gra 3D na Dreamcast
# Tworzy: GTAsaDC.elf + 1ST_READ.BIN + obraz CD z plikami .wav
#
# Mapy: dołączane są tylko istniejące pliki mapN.c (N=1..10)
# Dźwięki: wszystkie .wav z katalogu projektu są kopiowane do /rd/ na płycie
######################################################################

# Ścieżki
KOS_BASE = /home/krusz/dc/kos
KOS_PORTS = /home/krusz/dc/kos-ports
RAYLIB_INST = $(KOS_PORTS)/raylib4dc/inst

# Kompilator i narzędzia
CC = $(KOS_CC)
OBJCOPY = $(KOS_OBJCOPY)
CFLAGS = $(KOS_CFLAGS) -I$(RAYLIB_INST)/include
LDFLAGS = $(KOS_LDFLAGS) \
	-L$(RAYLIB_INST)/lib \
	-L$(KOS_PORTS)/lib

# Nazwy plików
TARGET = GTAsaDC.elf
BINARY = 1ST_READ.BIN
CD_IMAGE = GTAsaDC.iso

# Katalog dla danych na płycie
CD_DIR = cd
RD_DIR = $(CD_DIR)/rd

# Pliki źródłowe
MAP_SRCS = $(wildcard map1.c map2.c map3.c map4.c map5.c \
                      map6.c map7.c map8.c map9.c map10.c)
SRCS = main.c $(MAP_SRCS)
OBJS = $(SRCS:.c=.o)

# Pliki dźwiękowe – wszystkie .wav w katalogu projektu
WAV_FILES = $(wildcard *.wav)

######################################################################
# Reguły
######################################################################

all: $(BINARY) $(CD_IMAGE)
	@echo "===================================================="
	@echo "Budowanie zakończone."
	@echo "Obraz CD:  $(CD_IMAGE)"
	@echo "Dolaczone mapy: $(MAP_SRCS)"
	@echo "Dolaczone dzwieki: $(WAV_FILES)"
	@echo "===================================================="

# Główny plik wykonywalny
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(KOS_START) $(OBJS) \
		-Wl,--start-group \
		-lraylib \
		-lGL \
		-lm \
		-lc \
		-lkallisti \
		-lgcc \
		-Wl,--end-group

# Skompilowany plik binarny (1ST_READ.BIN)
$(BINARY): $(TARGET)
	$(OBJCOPY) -O binary $(TARGET) $(BINARY)

# Katalogi na płytę
$(CD_DIR):
	mkdir -p $(CD_DIR)
$(RD_DIR): $(CD_DIR)
	mkdir -p $(RD_DIR)

# Kopiowanie plików dźwiękowych do /rd
$(RD_DIR)/%.wav: %.wav | $(RD_DIR)
	cp $< $@

# Przygotowanie plików na płycie
cd_prepare: $(BINARY) $(CD_DIR) $(RD_DIR) $(addprefix $(RD_DIR)/, $(notdir $(WAV_FILES)))
	cp $(BINARY) $(CD_DIR)/

# Tworzenie obrazu CD (ISO)
$(CD_IMAGE): cd_prepare
	@echo "Tworzenie obrazu ISO..."
	mkisofs -C 0,0 -V GTAsaDC -G /usr/local/dc/kos/ipboot/ip.bin \
		-o $@ $(CD_DIR)

# Czyszczenie
clean:
	rm -f $(OBJS) $(TARGET) $(BINARY)
	rm -rf $(CD_DIR) $(CD_IMAGE)

# Kompilacja plików .c
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all clean cd_prepare