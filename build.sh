#!/bin/bash

function choose()
{
    local tmp="include/configs/.hb_config.h"
    local target="include/configs/hb_config.h"
    local conftmp=".config_tmp"

    echo "*********************************************************************"
    echo "boot mode= $bootmode"
    echo "*********************************************************************"
    cp .config $conftmp

    echo "#ifndef __HB_CONFIG_H__" > $tmp
    echo "#define __HB_CONFIG_H__" >> $tmp

    if [ "$bootmode" = "nor" ];then
        echo "/* #define CONFIG_HB_NAND_BOOT */" >> $tmp
        echo "#define CONFIG_HB_NOR_BOOT" >> $tmp
        echo "/* #define CONFIG_HB_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    elif [ "$bootmode" = "nand" ];then
        echo "#define CONFIG_HB_NAND_BOOT" >> $tmp
        echo "/* #define CONFIG_HB_NOR_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    elif [ "$bootmode" = "emmc" ];then
        echo "/* #define CONFIG_HB_NAND_BOOT */" >> $tmp
        echo "/* #define CONFIG_HB_NOR_BOOT */" >> $tmp
        echo "#define CONFIG_HB_MMC_BOOT" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    else
        echo "Unknown BOOT_MODE value: $bootmode"
        exit 1
    fi

    echo "#endif /* __HB_CONFIG_H__ */" >> $tmp

    mv $tmp $target
    mv $conftmp .config
}

function change_dts_flash_config()
{
    local dts_file="arch/arm/dts/hobot-x3-soc.dts"
    local key_value="qspi {"

    declare -i nline

    getline()
    {
        cat -n $dts_file|grep "${key_value}"|awk '{print $1}'
    }

    getlinenum()
    {
        awk "BEGIN{a=`getline`;b="1";c=(a+b);print c}";
    }

    nline=`getlinenum`

    if [ x"$bootmode" = x"nor" ] || [ x"$bootmode" = x"nand" ] \
       || [ "$FLASH_ENABLE" = "nor" ] || [ "$FLASH_ENABLE" = "nand" ];then
        sed -i "${nline}s#disabled#okay#g" $dts_file
    else
        sed -i "${nline}s#okay#disabled#g" $dts_file
    fi
}

function build()
{
    local uboot_image="u-boot.bin"
    prefix=$TARGET_UBOOT_DIR
    config=$UBOOT_DEFCONFIG
    echo "uboot config: $config"

    # real build
    ARCH=$ARCH_KERNEL
    make $config || {
        echo "make $config failed"
        exit 1
    }

    choose

    make -j${N} || {
        echo "make failed"
        exit 1
    }

    # put binaries to dest directory
    cpfiles "$uboot_image" "$prefix/"
}

function all()
{
        build
}

function all_32()
{
    CROSS_COMPILE=$CROSS_COMPILE_64
    all
}

function set_uboot_config()
{
    sed -i '/CONFIG_MTDIDS_DEFAULT/d' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
    sed -i '/CONFIG_MTDPARTS_DEFAULT/d' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
    if [[ "$bootmode" = "nand" ]] || [[ "$ifubi" = "true" ]];then
        sed -i 's/# CONFIG_MTD_UBI_FASTMAP is not set/CONFIG_MTD_UBI_FASTMAP=y/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/# CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT is not set/CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT=1/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/CONFIG_ENV_IS_IN_MMC=y/# CONFIG_ENV_IS_IN_MMC is not set/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/# CONFIG_ENV_IS_IN_UBI is not set/CONFIG_ENV_IS_IN_UBI=y/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_CMD_MTDPARTS/a CONFIG_MTDIDS_DEFAULT="spi-nand0=hr_nand"'  $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_MTDIDS_DEFAULT/a CONFIG_MTDPARTS_DEFAULT="mtdparts=hr_nand:6815744@0x0(bootloader),20971520@0x680000(sys),62914560@0x1A80000(rootfs),-@0x5680000(userdata)"' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
    elif [[ "$bootmode" = "nor" ]];then
        sed -i 's/# CONFIG_SPI_FLASH_MTD is not set/CONFIG_SPI_FLASH_MTD=y/g'
        sed -i 's/CONFIG_ENV_IS_IN_UBI=y/# CONFIG_ENV_IS_IN_UBI is not set/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_CMD_MTDPARTS/a CONFIG_MTDIDS_DEFAULT="spi-nor1=hr_nor"'  $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_MTDIDS_DEFAULT/a CONFIG_MTDPARTS_DEFAULT="mtdparts=hr_nor:393216@0x20000(sbl),524288@0x80000(bl31),1048576@0x100000(uboot),131072@0x200000(bpu),131072@0x220000(vbmeta),10485760@0x240000(boot),10485760@0xC40000(recovery),34603008@0x1640000(system),-@0x3740000(app)"' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
    else
        sed -i 's/CONFIG_MTD_UBI_FASTMAP=y/# CONFIG_MTD_UBI_FASTMAP is not set/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT=1/# CONFIG_MTD_UBI_FASTMAP_AUTOCONVERT is not set/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/# CONFIG_ENV_IS_IN_MMC is not set/CONFIG_ENV_IS_IN_MMC=y/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i 's/CONFIG_ENV_IS_IN_UBI=y/# CONFIG_ENV_IS_IN_UBI is not set/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_CMD_MTDPARTS/a CONFIG_MTDIDS_DEFAULT=""'  $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
        sed -i '/CONFIG_MTDIDS_DEFAULT/a CONFIG_MTDPARTS_DEFAULT=""' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
    fi
    sed -i 's/# CONFIG_CMD_MTDPARTS is not set/CONFIG_CMD_MTDPARTS=y/g' $TOPDIR/uboot/configs/$UBOOT_DEFCONFIG
}

function usage()
{
    echo "Usage: build.sh [-o emmc|nor|nand|all ] [-u]"    echo "Options:"
    echo "Options:"
    echo "  -o  boot mode, all or one of uart, emmc, nor, nand, ap"
    echo "  -u  add ubi and ubifs support in uboot"
    echo "  -h  help info"
    echo "Command:"
    echo "  clean clean all the object files along with the executable"
}

bootmode=$BOOT_MODE
ifubi=false

while getopts "uo:h:" opt
do
    case $opt in
        o)
            arg="$OPTARG"
            if [ "$arg" = "all" ];then
                all_boot_mode=true
                echo "compile all boot mode"
            else
                export bootmode="$OPTARG"
                echo "compile boot mode $bootmode"
            fi
            ;;
        u)
            ifubi=true
            ;;
        h)
            usage
            exit 0
            ;;
        \?)
            usage
            exit 1
            ;;
    esac
done

shift $[ $OPTIND - 1 ]

cmd=$1

function clean()
{
    make clean
}

# include
. $INCLUDE_FUNCS
# include end

# config dts
change_dts_flash_config

set_uboot_config

cd $(dirname $0)

buildopt $cmd
