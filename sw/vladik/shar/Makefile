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


shar:				data/uImage data/spondoolies.dtb ../add-ons/fw_ver
	vi ../add-ons/fw_ver
	cd ../ ; ./make_image
	cp ../image/uImage data/
	cp ../add-ons/fw_ver data/
	@echo -------------------------------
	@echo -n "Making SW upgrade version:"
	@cat data/fw_ver
	@echo -------------------------------
	makeself $(MAKESELF_FLAGS) data $(SHAR) "$(SHAR_NAME)" $(SCRIPT)
	../provisioning/sign-digest.sh --file=$(SHAR) --private=$(PRIVATE_KEY)
	tar cf $(TAR_FILE) $(SHAR) $(SHAR).sign
	$(RM) -f $(SHAR) $(SHAR).sign
	bash VER=`cat ../add-ons/fw_ver`; mv spon.tar spon_$VER.tar

.PHONY:	latest
latest:
	makeself $(MAKESELF_FLAGS) latest $(SHAR) "$(SHAR_NAME)" $(SCRIPT)
	../provisioning/sign-digest.sh --file=$(SHAR) --private=$(PRIVATE_KEY)
	tar cf $(TAR_LATEST_FILE) $(SHAR) $(SHAR).sign
	$(RM) -f $(SHAR) $(SHAR).sign
	
