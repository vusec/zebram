zram-y	:=	zcomp_lzo.o zcomp.o zram_drv.o zsmalloc.o zram_cache.o
ccflags-y +=	-Dzebra_sha -Dzebra_sha2 -DZEBRA_NATIVE

zram-$(CONFIG_ZRAM_LZ4_COMPRESS) += zcomp_lz4.o

obj-$(CONFIG_ZRAM)	+=	zram.o
