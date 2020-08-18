PROG_NAME= hexo
INSTALL_TOP= /usr/local
INSTALL_BIN= $(INSTALL_TOP)/bin

INSTALL= install -p
INSTALL_EXEC= $(INSTALL) -m 0755

MKDIR= mkdir -p
RM= rm -f

all clean:	
	@cd src && $(MAKE) $@

install: 
	cd src && $(MKDIR) $(INSTALL_BIN)
	cd src && $(INSTALL_EXEC) $(PROG_NAME) $(INSTALL_BIN)

uninstall:
	cd $(INSTALL_BIN) && $(RM) $(PROG_NAME)

local:
	$(MAKE) install INSTALL_TOP=../install

PHONY: all install local
