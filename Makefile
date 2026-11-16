# GENRA TECHNO: a VCV Rack 2 plugin (slug GenraTechno). Build:
#   make RACK_DIR=/path/to/Rack-SDK dist
RACK_DIR ?= ../..

SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/tg/*.cpp)
# the link hub of the GENRA plugins (Follow GENRA), copied from the GENRA repo
SOURCES += $(wildcard src/link/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/plugin.mk
