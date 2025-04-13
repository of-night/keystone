################################################################################
# 
# ipfs
#
################################################################################

# IPFS_VERSION = <version>
# IPFS_SITE = $(call github,ipfs,go-ipfs,v$(IPFS_VERSION))
# 指定本地 IPFS 源代码路径
ifeq ($(KEYSTONE_IPFS),)
$(error KEYSTONE_IPFS directory not defined)
else
include $(KEYSTONE)/mkutils/pkg-keystone.mk
endif

# IPFS_SITE_METHOD = local
# IPFS_SITE = $(KEYSTONE_IPFS)
# IPFS_LICENSE = MIT
# IPFS_LICENSE_FILES = LICENSE

IPFS_VERSION = 0.22.0
IPFS_SITE = $(call github,ipfs,go-ipfs,v$(IPFS_VERSION))
IPFS_LICENSE = MIT
IPFS_LICENSE_FILES = LICENSE

# # 动态设置 GOARCH
# IPFS_GOARCH = $(BR2_GOARCH)

# IPFS_BUILD_ENV = \
#     GO111MODULE=auto \
#     CGO_ENABLED=1 \
#     GOARCH=$(IPFS_GOARCH)


# # define IPFS_BUILD_CMDS
# #     cd $(@D) && $(IPFS_BUILD_ENV) go mod vendor && $(IPFS_BUILD_ENV) go build -o ipfs
# # endef

# define IPFS_BUILD_CMDS
#     cd $(@D) && $(IPFS_BUILD_ENV) go mod vendor
# endef

# define IPFS_INSTALL_TARGET_CMDS
#     $(INSTALL) -D -m 0755 $(@D)/ipfs $(TARGET_DIR)/usr/bin/ipfs
# endef

$(eval $(keystone-package))
$(eval $(golang-package))

