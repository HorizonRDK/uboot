#!/bin/bash

function choose()
{
    local tmp="include/configs/.x2_config.h"
    local target="include/configs/x2_config.h"
    local board=$BOARD_TYPE
    local bootmode=$BOOT_MODE
    local conftmp=".config_tmp"

    cp .config $conftmp

    echo "#ifndef __X2_CONFIG_H__" > $tmp
    echo "#define __X2_CONFIG_H__" >> $tmp

    if [ "$bootmode" = "ap" ];then
        echo "#define CONFIG_X2_AP_BOOT" >> $tmp
        echo "/* #define CONFIG_X2_YMODEM_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_NOR_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    elif [ "$bootmode" = "uart" ];then
        echo "/* #define CONFIG_X2_AP_BOOT */" >> $tmp
        echo "#define CONFIG_X2_YMODEM_BOOT" >> $tmp
        echo "/* #define CONFIG_X2_NOR_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=y" >> $conftmp
    elif [ "$bootmode" = "nor" ];then
        echo "/* #define CONFIG_X2_AP_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_YMODEM_BOOT */" >> $tmp
        echo "#define CONFIG_X2_NOR_BOOT" >> $tmp
        echo "/* #define CONFIG_X2_MMC_BOOT */" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    elif [ "$bootmode" = "emmc" ];then
        echo "/* #define CONFIG_X2_AP_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_YMODEM_BOOT */" >> $tmp
        echo "/* #define CONFIG_X2_NOR_BOOT */" >> $tmp
        echo "#define CONFIG_X2_MMC_BOOT" >> $tmp
        sed -i "/CONFIG_SPL_YMODEM_SUPPORT/d" $conftmp
        echo "CONFIG_SPL_YMODEM_SUPPORT=n" >> $conftmp
    else
        echo "Unknown BOOT_MODE value: $bootmode"
        exit 1
    fi
    echo "" >> $tmp
    if [ "$board" = "x2som" ];then
        echo "#define CONFIG_X2_SOM_BOARD" >> $tmp
    elif [ "$board" = "x2svb" ];then
        echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
    else
        echo "Unknown BOARD_TYPE value: $board"
        exit 1
    fi

    echo "#endif /* __X2_CONFIG_H__ */" >> $tmp

    mv $tmp $target
    mv $conftmp .config
}

function all()
{
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
    cpfiles "$UBOOT_IMAGE_NAME" "$prefix/"
    cd spl/
    cpfiles "$UBOOT_SPL_NAME" "$prefix/"
}

function clean()
{
    make clean
}

# include
. $INCLUDE_FUNCS
# include end

cd $(dirname $0)

buildopt $1
