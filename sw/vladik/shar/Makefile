SHAR_NAME			:= 'Spondoolies Software'
SHAR				:= spon.shar
TAR_FILE			:= spon.tar
TAR_LATEST_FILE			:= spon-latest.tar
SCRIPT				:= ./sw-upgrade.sh

MAKESELF_FLAGS			:= --nocomp

ifeq ($(PRIVATE_KEY),)
$(warning Set PRIVATE_KEY: make PRIVATE_KEY=<priv key file>)
$(warning Example: make PRIVATE_KEY=/mnt/flash/spondoolies.pem)
$(error )
endif

all:				shar


shar:				data/uImage data/spondoolies.dtb
	makeself $(MAKESELF_FLAGS) data $(SHAR) "$(SHAR_NAME)" $(SCRIPT)
	../provisioning/sign-digest.sh --file=$(SHAR) --private=$(PRIVATE_KEY)
	tar cf $(TAR_FILE) $(SHAR) $(SHAR).sign
	$(RM) -f $(SHAR) $(SHAR).sign

.PHONY:	latest
latest:
	makeself $(MAKESELF_FLAGS) latest $(SHAR) "$(SHAR_NAME)" $(SCRIPT)
	../provisioning/sign-digest.sh --file=$(SHAR) --private=$(PRIVATE_KEY)
	tar cf $(TAR_LATEST_FILE) $(SHAR) $(SHAR).sign
	$(RM) -f $(SHAR) $(SHAR).sign
	
