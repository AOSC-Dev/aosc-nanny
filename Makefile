PREFIX ?= /usr/local
DESTDIR ?=
BINDIR ?= $(DESTDIR)$(PREFIX)/bin
DATADIR ?= $(DESTDIR)$(PREFIX)/share
GETTEXT_DOMAIN ?= aosc_launcher

MO_FILES := $(patsubst %.po,%.mo,$(wildcard po/*.po))

.PHONY: all clean

all:
	@echo "Nothing to do."

po/messages.pot: nanny.sh
	xgettext -L Shell -o po/messages.pot nanny.sh

po/%.po: po/messages.pot
	msgmerge --backup off -U $@ $<

po/%.mo: po/%.po
	msgfmt -o $@ $<

install-mo: $(MO_FILES)
	$(foreach f,$(MO_FILES), \
	install -Dm644 $f $(DATADIR)/locale/$(basename $(notdir $f))/LC_MESSAGES/$(GETTEXT_DOMAIN).mo ;\
	)

install: install-mo nanny.sh
	install -d $(BINDIR) $(DATADIR)
	install -Dm755 nanny.sh $(BINDIR)/nanny

clean:
	rm -f po/*.mo po/*~
