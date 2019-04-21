#!/bin/bash

function choose()
{
    local board=$BOARD_TYPE
    local bootmode=$BOOT_MODE

    if [ "$bootmode" = "ap" ];then
        export UBOOT_DEFCONFIG=hr_x2_ap_defconfig
    elif [ "$bootmode" = "uart" ];then
        export UBOOT_DEFCONFIG=hr_x2_uart_defconfig
    elif [ "$bootmode" = "nor" ];then
        export UBOOT_DEFCONFIG=hr_x2_nor_defconfig
    elif [ "$bootmode" = "emmc" ];then
        export UBOOT_DEFCONFIG=hr_x2_emmc_defconfig
    else
        echo "Unknown BOOT_MODE value: $bootmode"
        exit 1
    fi
}

function set_uart()
{
    local bootmode=$BOOT_MODE
    local board=$BOARD_TYPE

    if [ "$bootmode" = "uart" ];then
        local tmp="include/configs/.x2_config.h"
        local target="include/configs/x2_config.h"
        local conftmp=".config_tmp"

        cp .config $conftmp

        echo "#ifndef __X2_CONFIG_H__" > $tmp
        echo "#define __X2_CONFIG_H__" >> $tmp

        if [ "$board" = "x2som" ];then
            echo "#define CONFIG_X2_SOM_BOARD" >> $tmp
            echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
        elif [ "$board" = "x2svb" ];then
            echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
            echo "/* #define CONFIG_X2_MONO_BOARD */" >> $tmp
        elif [ "$board" = "x2mono" ];then
            echo "/* #define CONFIG_X2_SOM_BOARD */" >> $tmp
            echo "#define CONFIG_X2_MONO_BOARD" >> $tmp
        fi

        echo "#endif /* __X2_CONFIG_H__ */" >> $tmp

        mv $tmp $target
        mv $conftmp .config
    fi
}

function all()
{
    choose

    prefix=$TARGET_UBOOT_DIR
    config=$UBOOT_DEFCONFIG
    echo "uboot config: $config"

    # real build
    ARCH=$ARCH_KERNEL
    make $config || {
        echo "make $config failed"
        exit 1
    }

    set_uart

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
