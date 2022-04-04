# add custom modes
ALL_MODES = release debug sanitize
# this is the default mode
MODE = release
ifeq ($(MODE),release)
  CFLAGS = $(CFLAGS_RELEASE)
  D=
else ifeq ($(MODE),debug)
  CFLAGS = $(CFLAGS_DEBUG)
  D=_dbg
else ifeq ($(MODE),sanitize)
  CFLAGS = $(CFLAGS_DEBUG) $(SANITIZE_FLAGS)
  LDFLAGS = $(SANITIZE_FLAGS)
  D=_sanitize
else
  $(error Unknown mode: '$(MODE)'. Accepted modes: $(ALL_MODES); or define your own mode in 'Makefile.inc.in')
endif
